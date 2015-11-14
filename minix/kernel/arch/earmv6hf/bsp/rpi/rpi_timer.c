#include "kernel/kernel.h"
#include "kernel/clock.h"
#include <sys/types.h>
#include <machine/cpu.h>
#include <minix/board.h>
#include <minix/mmio.h>
#include <assert.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "arch_proto.h"
#include "bsp_timer.h"
#include "rpi_timer_registers.h"
#include "rpi_intr_registers.h"
#include "bsp_intr.h"

/* interrupt handler hook */
static irq_hook_t rpi_timer_hook;

static u32_t counts_per_hz = ~0;
static u64_t high_frc;

struct rpi_timer_registers;

struct bcm2835_timer
{
	vir_bytes base;
	int irq_nr;
	struct rpi_timer_registers *regs;
};

struct rpi_timer_registers
{
	vir_bytes STCLO;
	vir_bytes STC3;
};

static struct rpi_timer_registers regs_v1 = 
{
	.STCLO = BCM2835_STIMER_CLO,
	.STC3 =  BCM2835_STIMER_C3,
};
static struct bcm2835_timer rpi_timer = {
	.base = BCM2835_STIMER_BASE,
	.irq_nr = BCM2835_INT_TIMER3,
	.regs = &regs_v1
};

static struct bcm2835_timer *timer;

int
bsp_register_timer_handler(const irq_handler_t handler)
{
	/* Initialize the CLOCK's interrupt hook. */
	rpi_timer_hook.proc_nr_e = NONE;
	rpi_timer_hook.irq = timer->irq_nr;

	put_irq_handler(&rpi_timer_hook, timer->irq_nr, handler);
	/* only unmask interrupts after registering */
	bsp_irq_unmask(timer->irq_nr);

	return 0;
}

/* meta data for remapping */
static kern_phys_map timer_phys_map;
void
bsp_timer_init(unsigned freq)
{
    if ( BOARD_IS_RPI(machine.board_id)) 
	{
		u32_t stclo;
		timer = &rpi_timer;
		counts_per_hz = BCM2835_STIMER_HZ / freq;
		kern_phys_map_ptr(timer->base, ARM_PAGE_SIZE,
		    VMMF_UNCACHED | VMMF_WRITE,
		    &timer_phys_map, (vir_bytes) & timer->base);
		stclo = mmio_read( timer->base + timer->regs->STCLO );
		stclo += counts_per_hz;
		mmio_write( timer->base + timer->regs->STC3, stclo );
		frclock_init();
	}
}

void
bsp_timer_stop()
{

}

void
bsp_timer_int_handler()
{
	u32_t stclo;
	stclo = mmio_read( timer->base + timer->regs->STCLO );
	stclo += counts_per_hz;
	mmio_write( timer->base + timer->regs->STC3, stclo );
}

