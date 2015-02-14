#ifndef _RPI_SERIAL_H
#define _RPI_SERIAL_H

/* UART register map */
#define RPI_BCM2835_DEBUG_UART_BASE 0x20201000 /* miniUART - uses PL011 module */

/* UART registers */
#define RPI_UART_DR         0 // data register
#define RPI_UART_FR         0x18 // flag register

/* Line status register fields */
#define RPI_UARTFR_TXFF     (1 << 5)        // tramit FIFO full

#endif /* _RPI_SERIAL_H */
