#ifndef _RPI_SERIAL_H
#define _RPI_SERIAL_H

#define RPI_UART_BASE 0x20201000 /* UART1 physical address */

#define UART011_FR_TXFF         0x020   /* Transmit-hold-register empty */
#define UART011_FR_RXFE         0x010
#define RPI_UART_MSR_CTS        0x10    /* Current Clear to Send */


#define UART011_DR              0x00    /* Data read or written from the interface. */
#define UART011_FR              0x18    /* Flag register (Read only). */
#define UART010_IIR             0x1C    /* Interrupt indentification register (Read). */
#define UART011_IBRD            0x24    /* Integer baud rate divisor register. */
#define UART011_FBRD            0x28    /* Fractional baud rate divisor register. */
#define UART011_LCRH            0x2c    /* Line control register. */
#define UART011_CR              0x30    /* Control register. */
#define UART011_IFLS            0x34    /* Interrupt fifo level select. */
#define UART011_IMSC            0x38    /* Interrupt mask. */
#define UART011_MIS             0x40    /* Masked interrupt status. */
#define UART011_ICR             0x44    /* Interrupt clear register. */

#define UART011_DR_OE           (1 << 11)

#define UART011_LCRH_SPS        0x80
#define UART011_LCRH_WLEN_8     0x60
#define UART011_LCRH_WLEN_7     0x40
#define UART011_LCRH_WLEN_6     0x20
#define UART011_LCRH_WLEN_5     0x00
#define UART011_LCRH_FEN        0x10
#define UART011_LCRH_STP2       0x08
#define UART011_LCRH_EPS        0x04
#define UART011_LCRH_PEN        0x02
#define UART011_LCRH_BRK        0x01


#define UART011_CR_LBE          0x0080  /* loopback enable */
#define UART011_CR_UARTEN       0x0001  /* UART enable */
#define UART011_CR_TXE          0x0100  /* transmit enable */
#define UART011_CR_RXE          0x0200  /* receive enable */
#define UART011_CR_DTR          0x0400  /* DTR */
#define UART011_CR_RTS          0x0800  /* RTS */
#define UART011_CR_RTSEN        0x4000  /* RTS hardware flow control */
#define UART011_CR_CTSEN        0x8000  /* CTS hardware flow control */
#define UART011_CR_OUT2         0x2000  /* OUT2 */
#define UART011_CR_OUT1         0x1000  /* OUT1 */

#define UART011_FR_BUSY         (1 << 3)        /* set to 1 when UART is transmitting data */
#define UART011_TXIM            (1 << 5)        /* transmit interrupt mask */
#define UART011_RXIM            (1 << 4)        /* receive interrupt mask */
#define UART011_RTIM            (1 << 6)        /* receive timeout interrupt mask */

#define UART011_OEIS            (1 << 10)       /* overrun error interrupt status */
#define UART011_BEIS            (1 << 9)        /* break error interrupt status */
#define UART011_PEIS            (1 << 8)        /* parity error interrupt status */
#define UART011_FEIS            (1 << 7)        /* framing error interrupt status */
#define UART011_RTIS            (1 << 6)        /* receive timeout interrupt status */
#define UART011_TXIS            (1 << 5)        /* transmit interrupt status */
#define UART011_RXIS            (1 << 4)        /* receive interrupt status */
#define UART011_DSRMIS          (1 << 3)        /* DSR interrupt status */
#define UART011_DCDMIS          (1 << 2)        /* DCD interrupt status */
#define UART011_CTSMIS          (1 << 1)        /* CTS interrupt status */
#define UART011_RIMIS           (1 << 0)        /* RI interrupt status */

#define UART011_IFLS_RX4_8      (2 << 3)
#define UART011_IFLS_TX4_8      (2 << 0)

#define UART011_FR_CTS          0x001
#define UART011_FR_DSR          0x002
#define UART011_FR_DCD          0x004

#define UART011_FR_MODEM_ANY    (UART011_FR_DCD|UART011_FR_DSR|UART011_FR_CTS)

#endif /* _OMAP_SERIAL_H */
