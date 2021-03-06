#include <minix/config.h>
#include <minix/drivers.h>
#include <minix/vm.h>
#include <minix/type.h>
#include <minix/board.h>
#include <sys/mman.h>
#include <assert.h>
#include <signal.h>
#include <termios.h>
#include "rpi_serial.h"
#include "tty.h"

#if NR_RS_LINES > 0

#define UART_FREQ       3000000L /* timer frequency */
#if 0
#define DFLT_BAUD    TSPEED_DEF        /* default baud rate */
#else
#define DFLT_BAUD	B115200		/* default baud rate */
#endif


#define RS_IBUFSIZE          40960	/* RS232 input buffer size */
#define RS_OBUFSIZE          40960	/* RS232 output buffer size */

/* Input buffer watermarks.
 * The external device is asked to stop sending when the buffer
 * exactly reaches high water, or when TTY requests it.  Sending restarts
 * when the input buffer empties below the low watermark.
 */
#define RS_ILOWWATER   (1 * RS_IBUFSIZE / 4)
#define RS_IHIGHWATER  (3 * RS_IBUFSIZE / 4)

/* Output buffer low watermark.
 * TTY is notified when the output buffer empties below the low watermark, so
 * it may continue filling the buffer if doing a large write.
 */
#define RS_OLOWWATER   (1 * RS_OBUFSIZE / 4)

/* Macros to handle flow control.
 * Interrupts must be off when they are used.
 * Time is critical - already the function call for outb() is annoying.
 * If outb() can be done in-line, tests to avoid it can be dropped.
 * istart() tells external device we are ready by raising RTS.
 * istop() tells external device we are not ready by dropping RTS.
 * DTR is kept high all the time (it probably should be raised by open and
 * dropped by close of the device).
 * OUT2 is also kept high all the time.
 */
#define istart(rs) \
	(serial_out((rs), UART011_CR, UART011_CR_UARTEN|UART011_CR_RXE|UART011_CR_TXE),\
		(rs)->idevready = TRUE)
#define istop(rs) \
	(serial_out((rs), UART011_CR, UART011_CR_OUT2|UART011_CR_DTR), \
		(rs)->idevready = FALSE)
/* Macro to tell if device is ready.  The rs->cts field is set to UART011_FR_CTS
 * if CLOCAL is in effect for a line without a CTS wire.
 */
#define devready(rs) (((serial_in(rs, UART011_FR) | rs->cts) & UART011_FR_CTS) ? ODEVREADY : 0)

/* Macro to tell if transmitter is ready. */
#define txready(rs) (serial_in(rs, UART011_FR) & UART011_FR_TXFE)

/* RS232 device structure, one per device. */
typedef struct rs232 {
  tty_t *tty;			/* associated TTY structure */

  int icount;			/* number of bytes in the input buffer */
  char *ihead;			/* next free spot in input buffer */
  char *itail;			/* first byte to give to TTY */
  char idevready;		/* nonzero if we are ready to receive (RTS) */
  char cts;			/* normally 0, but MS_CTS if CLOCAL is set */

  unsigned char ostate;		/* combination of flags: */
#define ODONE          1	/* output completed (< output enable bits) */
#define ORAW           2	/* raw mode for xoff disable (< enab. bits) */
#define OWAKEUP        4	/* tty_wakeup() pending (asm code only) */
#define ODEVREADY   0x10	/* external device hardware ready (CTS) */
#define OQUEUED     0x20	/* output buffer not empty */
#define OSWREADY    0x40	/* external device software ready (no xoff) */
#define ODEVHUP     0x80	/* external device has dropped carrier */
#define OSOFTBITS  (ODONE | ORAW | OWAKEUP | OQUEUED | OSWREADY)
				/* user-defined bits */
#if (OSOFTBITS | ODEVREADY | ODEVHUP) == OSOFTBITS
				/* a weak sanity check */
#error				/* bits are not unique */
#endif
  unsigned char oxoff;		/* char to stop output */
  char inhibited;		/* output inhibited? (follows tty_inhibited) */
  char drain;			/* if set drain output and reconfigure line */
  int ocount;			/* number of bytes in the output buffer */
  char *ohead;			/* next free spot in output buffer */
  char *otail;			/* next char to output */

  phys_bytes phys_base;		/* UART physical base address (I/O map) */
  unsigned int reg_offset;	/* UART register offset */
  unsigned int im;		/* copy of im register */
  unsigned int scr;		/* copy of scr register */
  unsigned int fcr;		/* copy of fcr register */
  unsigned int uartclk;		/* UART clock rate */
  unsigned int old_status;

  unsigned char lstatus;	/* last line status */
  int rx_overrun_events;

  int irq;			/* irq for this line */
  int irq_hook_id;		/* interrupt hook */
  int irq_hook_kernel_id;	/* id as returned from sys_irqsetpolicy */

  char ibuf[RS_IBUFSIZE];	/* input buffer */
  char obuf[RS_OBUFSIZE];	/* output buffer */
} rs232_t;

static rs232_t rs_lines[NR_RS_LINES];

typedef struct uart_port {
	phys_bytes base_addr;
	int irq;
} uart_port_t;

static uart_port_t rpi_ports[] = {
  {  RPI_UART_BASE , 57 },	/* UART0 */
  { 0, 0 },
  { 0, 0 },
  { 0, 0 }
};


static int rs_write(tty_t *tp, int try);
static void rs_echo(tty_t *tp, int c);
static int rs_ioctl(tty_t *tp, int try);
static void rs_config(rs232_t *rs);
static int rs_read(tty_t *tp, int try);
static int rs_icancel(tty_t *tp, int try);
static int rs_ocancel(tty_t *tp, int try);
static void rs_ostart(rs232_t *rs);
static int rs_break_on(tty_t *tp, int try);
static int rs_break_off(tty_t *tp, int try);
static int rs_close(tty_t *tp, int try);
static int rs_open(tty_t *tp, int try);
static void rs232_handler(rs232_t *rs);
static void rs_reset(rs232_t *rs);
static unsigned int check_modem_status(rs232_t *rs);
static int termios_baud_rate(struct termios *term);

static inline unsigned int readw(vir_bytes addr);
static inline unsigned int serial_in(rs232_t *rs, int offset);
static inline void serial_out(rs232_t *rs, int offset, int val);
static inline void writew(vir_bytes addr, int val);
static void write_chars(rs232_t *rs);
static void read_chars(rs232_t *rs, unsigned int status);

static inline unsigned int
readw(vir_bytes addr)
{
	return *((volatile unsigned int *) addr);
}

static inline void
writew(vir_bytes addr, int val)
{
	*((volatile unsigned int *) addr) = val;
}

static inline unsigned int
serial_in(rs232_t *rs, int offset)
{
	offset <<= rs->reg_offset;
	return readw(rs->phys_base + offset);
}

static inline void
serial_out(rs232_t *rs, int offset, int val)
{
	offset <<= rs->reg_offset;
	writew(rs->phys_base + offset, val);
}

static void
rs_reset(rs232_t *rs)
{
    unsigned int lcr;
    while( serial_in( rs, UART011_FR ) & UART011_FR_BUSY )
    {
    }
    lcr = serial_in( rs, UART011_LCRH );
    lcr &= ~(UART011_LCRH_BRK | UART011_LCRH_FEN);
    serial_out( rs, UART011_LCRH, lcr );
}

static int
rs_write(register tty_t *tp, int try)
{
/* (*devwrite)() routine for RS232. */

	rs232_t *rs = tp->tty_priv;
	int r, count, ocount;

	if (rs->inhibited != tp->tty_inhibited) {
		/* Inhibition state has changed. */
		rs->ostate |= OSWREADY;
		if (tp->tty_inhibited) rs->ostate &= ~OSWREADY;
		rs->inhibited = tp->tty_inhibited;
	}

	if (rs->drain) {
		/* Wait for the line to drain then reconfigure and continue
		 * output. */
		if (rs->ocount > 0) return 0;
		rs->drain = FALSE;
		rs_config(rs);
	}
	/* While there is something to do. */
	for (;;) {
		ocount = buflen(rs->obuf) - rs->ocount;
		count = bufend(rs->obuf) - rs->ohead;
		if (count > ocount) count = ocount;
		if (count > tp->tty_outleft) count = tp->tty_outleft;
		if (count == 0 || tp->tty_inhibited) {
			if (try) return 0;
			break;
		}

		if (try) return 1;

		/* Copy from user space to the RS232 output buffer. */
		if (tp->tty_outcaller == KERNEL) {
			/* We're trying to print on kernel's behalf */
			memcpy(rs->ohead,
				(char *) tp->tty_outgrant + tp->tty_outcum,
				count);
		} else {
			if ((r = sys_safecopyfrom(tp->tty_outcaller,
				tp->tty_outgrant, tp->tty_outcum,
				(vir_bytes) rs->ohead, count)) != OK) {
				return 0;
			}
		}

		/* Perform output processing on the output buffer. */
		out_process(tp, rs->obuf, rs->ohead, bufend(rs->obuf), &count,
		    &ocount);
		if (count == 0) {
			break;
		}

		/* Assume echoing messed up by output. */
		tp->tty_reprint = TRUE;

		/* Bookkeeping. */
		rs->ocount += ocount;
		rs_ostart(rs);
		if ((rs->ohead += ocount) >= bufend(rs->obuf))
			rs->ohead -= buflen(rs->obuf);
		tp->tty_outcum += count;
		if ((tp->tty_outleft -= count) == 0) {
			/* Output is finished, reply to the writer. */
			if (tp->tty_outcaller != KERNEL)
				chardriver_reply_task(tp->tty_outcaller,
					tp->tty_outid, tp->tty_outcum);
			tp->tty_outcum = 0;
			tp->tty_outcaller = NONE;
		}

	}
	if (tp->tty_outleft > 0 && tp->tty_termios.c_ospeed == B0) {
		/* Oops, the line has hung up. */
		if (tp->tty_outcaller != KERNEL)
			chardriver_reply_task(tp->tty_outcaller, tp->tty_outid,
				EIO);
		tp->tty_outleft = tp->tty_outcum = 0;
		tp->tty_outcaller = NONE;
	}

	return 1;
}

static void
rs_echo(tty_t *tp, int character)
{
/* Echo one character.  (Like rs_write, but only one character, optionally.) */

	rs232_t *rs = tp->tty_priv;
	int count, ocount;

	ocount = buflen(rs->obuf) - rs->ocount;
	if (ocount == 0) return;		/* output buffer full */
	count = 1;
	*rs->ohead = character;			/* add one character */

	out_process(tp, rs->obuf, rs->ohead, bufend(rs->obuf), &count, &ocount);
	if (count == 0) return;

	rs->ocount += ocount;
	rs_ostart(rs);
	if ((rs->ohead += ocount) >= bufend(rs->obuf))
		rs->ohead -= buflen(rs->obuf);
}

static int
rs_ioctl(tty_t *tp, int UNUSED(dummy))
{
/* Reconfigure the line as soon as the output has drained. */
	rs232_t *rs = tp->tty_priv;

	rs->drain = TRUE;
	return 0;	/* dummy */
}

static int
termios_baud_rate(struct termios *term)
{
	int baud, denom, div, rem, ibrd, fbrd;
	switch(term->c_ospeed) {
	case B300: baud = 300; break;
	case B600: baud = 600; break;
	case B1200: baud = 1200; break;
	case B2400: baud = 2400; break;
	case B4800: baud = 4800; break;
	case B9600: baud = 9600; break;
	case B38400: baud = 38400; break;
	case B57600: baud = 57600; break;
	case B115200: baud = 115200; break;
	case B0:
	default:
		/* Reset the speed to the default speed, then call ourselves
		 * to convert the default speed to a baudrate. This call will
		 * always return a value without inducing another recursive
		 * call. */
		term->c_ospeed = DFLT_BAUD;
		baud = termios_baud_rate(term);
	}

	denom = 16 * baud;
	div = UART_FREQ / denom;
	rem = UART_FREQ % denom;

	ibrd = div << 6;
	fbrd = (((8 * rem) / baud) + 1) / 2;

	/* Tolerance? */
	return ibrd | fbrd;
}
static void rs_config(rs232_t *rs)
{
/* Set various line control parameters for RS232 I/O.  */
	tty_t *tp = rs->tty;
	unsigned int divisor, efr, lcr, mcr, rate, old_cr;
    
    /* Clear pending error and receive interrupts */
    serial_out( rs, UART011_ICR, (UART011_OEIS | UART011_BEIS | UART011_PEIS | UART011_FEIS | UART011_RTIS | UART011_RXIS) );
    serial_out( rs, UART011_IFLS, ( UART011_IFLS_RX4_8 | UART011_IFLS_TX4_8 ) );
/*
* Provoke TX FIFO interrupt into asserting.
*/
    serial_out( rs, UART011_CR, 0 );
    while( serial_in( rs, UART011_FR ) & UART011_FR_BUSY );

    rs->old_status = serial_in( rs, UART011_FR ) & UART011_FR_MODEM_ANY;
    
    /* RS232 needs to know the xoff character, and if CTS works. */
    rs->oxoff = tp->tty_termios.c_cc[VSTOP];
    rs->cts = (tp->tty_termios.c_cflag & CLOCAL) ? UART011_FR_CTS : 0;


    rate = termios_baud_rate(&tp->tty_termios);

    /* Compute line control flag bits. */
    lcr = 0;
    if (tp->tty_termios.c_cflag & PARENB) {
	lcr |= UART011_LCRH_PEN;
	if (!(tp->tty_termios.c_cflag & PARODD)) lcr |= UART011_LCRH_EPS;
    }
    if (tp->tty_termios.c_cflag & CSTOPB) lcr |= UART011_LCRH_STP2;
    switch(tp->tty_termios.c_cflag & CSIZE) {
  	case CS5:
  		lcr |= UART011_LCRH_WLEN_5;
  		break;
  	case CS6:
  		lcr |= UART011_LCRH_WLEN_6;
  		break;
  	case CS7:
  		lcr |= UART011_LCRH_WLEN_7;
  		break;
  	default:
  	case CS8:
  		lcr |= UART011_LCRH_WLEN_8;
  		break;
    }

	/* Lock out interrupts while setting the speed. The receiver register
	 * is going to be hidden by the div_low register, but the input
	 * interrupt handler relies on reading it to clear the interrupt and
	 * avoid looping forever.
	 */

	if (sys_irqdisable(&rs->irq_hook_kernel_id) != OK)
		panic("unable to disable interrupts");

    /* first disable everything */
    serial_out(rs, UART011_CR, 0 );

 /* Set baud rate */
        serial_out(rs, UART011_FBRD, (rate & ((1 << 6) - 1)) );
        serial_out(rs, UART011_IBRD, (rate >> 6) );
        /*
         * ----------v----------v----------v----------v-----
         * NOTE: MUST BE WRITTEN AFTER UARTLCR_M & UARTLCR_L
         * ----------^----------^----------^----------^-----
         */
        serial_out(rs, UART011_LCRH, (lcr | UART011_LCRH_FEN) );
        serial_out(rs, UART011_IMSC , rs->im );
		lcr = UART011_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE;
		serial_out( rs, UART011_CR, lcr );

    rs->ostate = devready(rs) | ORAW | OSWREADY;	/* reads MSR */
	if ((tp->tty_termios.c_lflag & IXON) && rs->oxoff != _POSIX_VDISABLE)
		rs->ostate &= ~ORAW;
        
	if (sys_irqenable(&rs->irq_hook_kernel_id) != OK)
		panic("unable to enable interrupts");
}

void
rs_init(tty_t *tp)
{
/* Initialize RS232 for one line. */
	register rs232_t *rs;
	int line;
	uart_port_t this_rpi;
	char l[10];
	struct minix_mem_range mr;
	struct machine machine;

	/* Associate RS232 and TTY structures. */
	line = tp - &tty_table[NR_CONS];

	/* See if kernel debugging is enabled; if so, don't initialize this
	 * serial line, making tty not look at the irq and returning ENXIO
	 * for all requests on it from userland. (The kernel will use it.)
	 */
	rs = tp->tty_priv = &rs_lines[line];
	rs->tty = tp;

	/* Set up input queue. */
	rs->ihead = rs->itail = rs->ibuf;

	sys_getmachine(&machine);
	
	if (BOARD_IS_RPI(machine.board_id)){
		this_rpi = rpi_ports[line];
	} else {
		return;
	}
	if (this_rpi.base_addr == 0) return;

	/* Configure memory access */
	mr.mr_base = rs->phys_base;
	mr.mr_limit = rs->phys_base + 0x100;
	if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != OK) {
		panic("Unable to request access to UART memory");
	}
	rs->phys_base = (vir_bytes) vm_map_phys(SELF,
					(void *) this_rpi.base_addr, 0x1000);
	
	if (rs->phys_base ==  (vir_bytes) MAP_FAILED) {
		panic("Unable to request access to UART memory");
	}
	rs->reg_offset = 0;

	rs->uartclk = UART_FREQ;
	rs->ohead = rs->otail = rs->obuf;

	/* Override system default baud rate. We do this because u-boot
	 * configures the UART for a baud rate of 115200 b/s and the kernel
	 * directly sends data over serial out upon boot up. If we then
	 * suddenly change the settings, the output will be garbled during
	 * booting.
	 */
	tp->tty_termios.c_ospeed = DFLT_BAUD;

	/* Configure IRQ */
	rs->irq = this_rpi.irq;

	/* callback with irq line number + 1 because using line number 0 
	   fails eslewhere */
	rs->irq_hook_kernel_id = rs->irq_hook_id = line + 1;	

	/* sys_irqsetpolicy modifies irq_hook_kernel_id. this modified id
	 * needs to be used in sys_irqenable and similar calls.
	 */
	if (sys_irqsetpolicy(rs->irq, 0, &rs->irq_hook_kernel_id) != OK) {
		printf("RS232: Couldn't obtain hook for irq %d\n", rs->irq);
	} else {
		if (sys_irqenable(&rs->irq_hook_kernel_id) != OK)  {
			printf("RS232: Couldn't enable irq %d (hooked)\n",
				rs->irq);
		}
	}

	/* When we get called back we get called back using the original 
	 * hook_id bit set. e.g. if we register with hook_id 5 the callback
	 * calls us with the 5 th bit set */
	rs_irq_set |= (1 << (rs->irq_hook_id ));

	/* Enable interrupts */
	rs_reset(rs);
	rs->im = UART011_RXIM | UART011_RTIM;
	rs_config(rs);

	/* Fill in TTY function hooks. */
	tp->tty_devread = rs_read;
	tp->tty_devwrite = rs_write;
	tp->tty_echo = rs_echo;
	tp->tty_icancel = rs_icancel;
	tp->tty_ocancel = rs_ocancel;
	tp->tty_ioctl = rs_ioctl;
	tp->tty_break_on = rs_break_on;
	tp->tty_break_off = rs_break_off;
	tp->tty_open = rs_open;
	tp->tty_close = rs_close;
printf("rs_init done\n");
	/* Tell external device we are ready. */
	istart(rs);
}

void
rs_interrupt(message *m)
{
	unsigned long irq_set;
	int line;
	rs232_t *rs;

	irq_set = m->m_notify.interrupts;
	for (line = 0, rs = rs_lines; line < NR_RS_LINES; line++, rs++) {
		if (irq_set & (1 << rs->irq_hook_id)) {
			rs232_handler(rs);
			if (sys_irqenable(&rs->irq_hook_kernel_id) != OK)
				panic("unable to enable interrupts");
		}
	}
}

static int
rs_icancel(tty_t *tp, int UNUSED(dummy))
{
/* Cancel waiting input. */
	rs232_t *rs = tp->tty_priv;

	rs->icount = 0;
	rs->itail = rs->ihead;
	istart(rs);
	return 0;	/* dummy */
}

static int
rs_ocancel(tty_t *tp, int UNUSED(dummy))
{
/* Cancel pending output. */
	rs232_t *rs = tp->tty_priv;

	rs->ostate &= ~(ODONE | OQUEUED);
	rs->ocount = 0;
	rs->otail = rs->ohead;

	return 0;	/* dummy */
}

static int
rs_read(tty_t *tp, int try)
{
/* Process characters from the circular input buffer. */

	rs232_t *rs = tp->tty_priv;
	int icount, count, ostate;

	if (!(tp->tty_termios.c_cflag & CLOCAL)) {
		if (try) return 1;

		/* Send a SIGHUP if hangup detected. */
		ostate = rs->ostate;
		rs->ostate &= ~ODEVHUP;		/* save ostate, clear DEVHUP */
		if (ostate & ODEVHUP) {
			sigchar(tp, SIGHUP, 1);
			tp->tty_termios.c_ospeed = B0;/* Disable further I/O.*/
			return 0;
		}
	}

	if (try) {
		return(rs->icount > 0);
	}

	while ((count = rs->icount) > 0) {
		icount = bufend(rs->ibuf) - rs->itail;
		if (count > icount) count = icount;

		/* Perform input processing on (part of) the input buffer. */
		if ((count = in_process(tp, rs->itail, count)) == 0) break;
		rs->icount -= count;
		if (!rs->idevready && rs->icount < RS_ILOWWATER) istart(rs);
		if ((rs->itail += count) == bufend(rs->ibuf))
			rs->itail = rs->ibuf;
	}

	return 0;
}

static void
rs_ostart(rs232_t *rs)
{
/* Tell RS232 there is something waiting in the output buffer. */

	rs->ostate |= OQUEUED;
	if (txready(rs)) write_chars(rs);
}

static int
rs_break_on(tty_t *tp, int UNUSED(dummy))
{
/* Raise break condition */
	rs232_t *rs = tp->tty_priv;
	unsigned int lsr;

    lsr = serial_in(rs, UART011_LCRH);
	serial_out(rs, UART011_LCRH, lsr | UART011_LCRH_BRK);
	return 0;	/* dummy */
}

static int
rs_break_off(tty_t *tp, int UNUSED(dummy))
{
/* Clear break condition */
	rs232_t *rs = tp->tty_priv;
	unsigned int lsr;

	lsr = serial_in(rs, UART011_LCRH);
	serial_out(rs, UART011_LCRH, lsr & ~UART011_LCRH_BRK);
	return 0;	/* dummy */
}

static int
rs_open(tty_t *tp, int UNUSED(dummy))
{
	/* Set the speed to 115200 by default */
	tp->tty_termios.c_ospeed = DFLT_BAUD;
	return 0;
}

static int
rs_close(tty_t *tp, int UNUSED(dummy))
{
/* The line is closed; optionally hang up. */
	rs232_t *rs = tp->tty_priv;
	if (tp->tty_termios.c_cflag & HUPCL) {
        unsigned int lcr;
		lcr = UART011_CR_RXE | UART011_CR_TXE;
		serial_out(rs, UART011_CR, lcr);
		serial_out(rs, UART011_IMSC, (UART011_RTIM | UART011_RXIM));
	}
	return 0;	/* dummy */
}

/* Low level (interrupt) routines. */

static void
rs232_handler(struct rs232 *rs)
{
/* Handle interrupt of a UART port */
	unsigned int iir, lsr;

    iir = serial_in(rs, UART011_MIS);
    if ( iir )
    {
        do
        {
            serial_out(rs, UART011_ICR, (iir & ~(UART011_TXIS|UART011_RTIS|UART011_RXIS)));
            if ( iir & (UART011_RTIS|UART011_RXIS))
            {
                read_chars(rs, 0);
            }
            if ( iir & (UART011_DSRMIS|UART011_DCDMIS|UART011_CTSMIS|UART011_RIMIS))
            {
                check_modem_status(rs);
            }
            if ( iir & (UART011_TXIS))
            {
                write_chars(rs);
            }
            iir = serial_in(rs, UART011_MIS);
        } while ( iir != 0 );
    }
}

static void
read_chars(rs232_t *rs, unsigned int status)
{
	unsigned char c;

	/* check the line status to know if there are more chars */
	while ( (serial_in(rs, UART011_FR) &  UART011_FR_RXFE) == 0 ) {
		c = serial_in(rs, UART011_DR);
        if ( c & UART011_DR_OE )
        {
            rs->rx_overrun_events++;
        }
		if (!(rs->ostate & ORAW)) {
			if (c == rs->oxoff) {
				rs->ostate &= ~OSWREADY;
			} else if (!(rs->ostate & OSWREADY)) {
				rs->ostate = OSWREADY;
			}
		}

		if (rs->icount == buflen(rs->ibuf)) {
			/* no buffer space? keep reading */
			continue;
		}

		if (++rs->icount == RS_IHIGHWATER && rs->idevready) {
			 istop(rs);
		}

		*rs->ihead = c;
		if (++rs->ihead == bufend(rs->ibuf)) {
			rs->ihead = rs->ibuf;
		}

		if (rs->icount == 1) {
			rs->tty->tty_events = 1;
		}
	}
}

static void
write_chars(rs232_t *rs)
{
/* If there is output to do and everything is ready, do it (local device is
 * known ready).
 * Notify TTY when the buffer goes empty.
 */

	if (rs->ostate >= (ODEVREADY | OQUEUED | OSWREADY)) {
		/* Bit test allows ORAW and requires the others. */
		serial_out(rs, UART011_DR, *rs->otail);
		if (++rs->otail == bufend(rs->obuf))
			rs->otail = rs->obuf;
		if (--rs->ocount == 0) {
			/* Turn on ODONE flag, turn off OQUEUED */
			rs->ostate ^= (ODONE | OQUEUED); 
			rs->tty->tty_events = 1;
			if (rs->im & UART011_TXIM) {
				rs->im &= ~UART011_TXIM;
				serial_out(rs, UART011_IMSC, rs->im);
			}
		} else  {
			if (rs->icount == RS_OLOWWATER)
				rs->tty->tty_events = 1;
			if (!(rs->im & UART011_TXIM)) {
				rs->im |= UART011_TXIM;
				serial_out(rs, UART011_IMSC, rs->im);
			}
		}
	}
}

static unsigned int
check_modem_status(rs232_t *rs)
{
/* Check modem status */

	unsigned int msr;

	msr = serial_in(rs, UART011_FR); /* Resets modem interrupt */
	if ((msr & (UART011_FR_DCD|UART011_FR_DSR)) == UART011_FR_DSR) {
		rs->ostate |= ODEVHUP;
		rs->tty->tty_events = 1;
	}

	if (!devready(rs))
		rs->ostate &= ~ODEVREADY;
	else
		rs->ostate |= ODEVREADY;

	return msr;
}

#endif /* NR_RS_LINES > 0 */

