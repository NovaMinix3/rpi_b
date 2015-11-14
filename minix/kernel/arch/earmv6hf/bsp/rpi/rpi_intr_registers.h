#ifndef _RPI_INTR_H
#define _RPI_INTR_H

/* Interrupt controller memory map */
#define RPI_INTR_BASE 0x2000B000 /* INTCPS physical address */
#define RPI_INTCPS_PENDING_IRQ_BASIC    0x200
#define RPI_INTCPS_PENDING_IRQ0         0x204
#define RPI_INTCPS_PENDING_IRQ1         0x208
#define RPI_INTCPS_MIR_SET0             0x21C /* Set interrupt mask bits */
#define RPI_INTCPS_MIR_SET1             0x220 /* Set interrupt mask bits */
#define RPI_INTCPS_MIR_SET_BASIC        0x224 /* Set interrupt mask bits */
#define RPI_INTCPS_MIR_CLEAR0           0x210 /* Clear interrupt mask bits */
#define RPI_INTCPS_MIR_CLEAR1           0x214 /* Clear interrupt mask bits */
#define RPI_INTCPS_MIR_CLEAR_BASIC      0x218 /* Clear interrupt mask bits */
#define BCM2835_INT_TIMER3  3

#endif /* _RPI_INTR_H */
