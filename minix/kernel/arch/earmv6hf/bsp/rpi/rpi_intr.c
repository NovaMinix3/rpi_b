#include <sys/types.h>
#include <machine/cpu.h>
#include <minix/type.h>
#include <minix/board.h>
#include <io.h>

#include "kernel/kernel.h"
#include "kernel/proc.h"
#include "kernel/vm.h"
#include "kernel/proto.h"
#include "arch_proto.h"
#include "hw_intr.h"

#include "rpi_intr_registers.h"

#define NR_BANKS 3

static struct rpi_intr
{
    vir_bytes base;
    int size;
} rpi_intr;

/* Bitwise table of IRQs that have been enabled on the ARM. */
static u32_t             bcm2835_enabled_irqs[3];
static kern_phys_map     intr_phys_map;

int intr_init(const int auto_eoi)
{
    if (BOARD_IS_RPI(machine.board_id)) 
    {
        rpi_intr.base = RPI_INTR_BASE;
    } 
    else 
    {
        panic("Can not do the interrupt setup. machine (0x%08x) is unknown\n",machine.board_id);
    };
    rpi_intr.size = 0x1000;	/* 4K */

    kern_phys_map_ptr(rpi_intr.base, rpi_intr.size,
        VMMF_UNCACHED | VMMF_WRITE,
        &intr_phys_map, (vir_bytes) & rpi_intr.base);
    return 0;
}

/* 
 * Check if the pending bit for an IRQ line is set; if so, call the handler
 * function. 
 */ 
void check_irq_pending(u32_t irq_num)
{
    u32_t handle = FALSE;

    /* Check the appropriate hardware register, depending on the IRQ number.  */
    if ( irq_num >= 0x40 )
    {
        u32_t irqPending = mmio_read( rpi_intr.base + RPI_INTCPS_PENDING_IRQ_BASIC );
        if ( irqPending & ( 1 << ( irq_num - 0x40 ) ) )
        {
            handle = TRUE;
        }
    }
    else if ( irq_num >= 0x20 )
    {
        u32_t irqPending1 = mmio_read( rpi_intr.base + RPI_INTCPS_PENDING_IRQ1 );
        if ( irqPending1 & ( 1 << ( irq_num - 0x20 ) ) )
        {
            handle = TRUE;
        }
    }
    else
    {
        u32_t irqPending0 = mmio_read( rpi_intr.base + RPI_INTCPS_PENDING_IRQ0 );
        if ( irqPending0 & ( 1 << irq_num ) )
        {
            handle = TRUE;
        }
    }
    if (handle)
    {
        irq_handle(irq_num);
        /* The pending bit should have been cleared in a device-specific way by
         * the handler function.  As far as we can tell, it cannot be cleared
         * directly through the interrupt controller.  */
    }
}

/*
 * Processes all pending interrupt requests.
 *
 * On the BCM2835 (Raspberry Pi), this done by iterating through all registered
 * interrupts on the ARM and checking whether each one is pending.  This is not
 * necessarily the fastest way to do it, but this should minimize problems with
 * the poorly-documented hardware and conflicts with the GPU.
 */

/* Find index of first set bit in a nonzero word. */
static inline ulong first_set_bit(ulong word)
{
    return 31 - __builtin_clz(word);
}

void bsp_irq_handle(void)
{
    u32_t bank;

    for ( bank = 0; bank < NR_BANKS; bank++ )
    {
        u32_t mask = bcm2835_enabled_irqs[bank];
        while ( mask != 0 )
        {
            u32_t bit = first_set_bit( mask );
            mask ^= ( 1 << bit );
            check_irq_pending( bit + ( bank << 5 ) );
        }
    }
}

void bsp_irq_unmask(int irq)
{
    if ( irq < 0x20 ) //gpu0: 0-31
    {
        mmio_write( rpi_intr.base + RPI_INTCPS_MIR_CLEAR0, ( 1 << irq ) );
        bcm2835_enabled_irqs[0] |= ( 1 << irq );
    }
    else if ( irq < 0x40 ) //gpu1: 32-63
    {
        mmio_write( rpi_intr.base + RPI_INTCPS_MIR_CLEAR1, ( 1 << ( irq - 0x20 ) ) );
        bcm2835_enabled_irqs[1] |= ( 1 << ( irq - 0x20 ) );
    }
    else //arm: 64-95 (in fact 71)
    {
        mmio_write( rpi_intr.base + RPI_INTCPS_MIR_CLEAR_BASIC, ( 1 << ( irq - 0x40 ) ) );
        bcm2835_enabled_irqs[2] |= ( 1 << ( irq - 0x40 ) );
    }
}

void bsp_irq_mask(const int irq)
{
    if ( irq < 0x20 )
    {
        mmio_write( ( rpi_intr.base + RPI_INTCPS_MIR_SET0 ), ( 1 << irq ) );
        bcm2835_enabled_irqs[0] &= ~( 1 << irq );
    }
    else if ( irq < 0x40 )
    {
        mmio_write( ( rpi_intr.base + RPI_INTCPS_MIR_SET1 ), ( 1 << ( irq - 0x20 ) ) );
        bcm2835_enabled_irqs[1] &= ~( 1 << ( irq - 0x20 ) );
    }
    else
    {
        mmio_write( ( rpi_intr.base + RPI_INTCPS_MIR_SET_BASIC ), ( 1 << ( irq - 0x40 ) ) );
        bcm2835_enabled_irqs[2] &= ~( 1 << ( irq - 0x40 ) );
    }
}

