#ifndef _RPI_TIMER_REGISTERS_H
#define _RPI_TIMER_REGISTERS_H

#define BCM2835_STIMER_HZ 1000000


/* General-purpose timer register map */
#define BCM2835_STIMER_BASE  0x20003000 /* System Timer */ 

/* General-purpose timer registers */
#define BCM2835_STIMER_CLO    0x04  /* System Timer Counter Lower 32 bits  */
#define BCM2835_STIMER_C3     0x18  /* System Timer Compare 3 */

#define RPI_WDT_BASE            0x20100000	/* watchdog timer */
#define RPI_PM_RSTC             0x1c
#define RPI_PM_WDOG             0x24
#define RPI_PM_PASSWORD         0x5a00000
#define RPI_PM_RSTC_RESET       0x00000102
#define RPI_PM_RSTC_CONFIGMASK  0x00000030
#define RPI_PM_RSTC_FULL_RESET  0x00000020
#define RPI_PM_WDOG_TIMEMASK    0x000fffff

#endif /* _RPI_TIMER_REGISTERS_H */
