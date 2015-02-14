#include <assert.h>
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
#include "bsp_reset.h"

#include "rpi_timer_registers.h"
#include "rpi_rtc.h"

void bsp_reset_init(void)
{
}

void
bsp_reset(void)
{
}

void
bsp_poweroff(void)
{

/*
 * The am335x can signal an external power management chip to cut the power
 * by toggling the PMIC_POWER_EN pin. It might fail if there isn't an
 * external PMIC or if the PMIC hasn't been configured to respond to toggles.
 * The only way to pull the pin low is via ALARM2 (see TRM 20.3.3.8).
 * At this point PM should have already signaled readclock to set the alarm.
 */
	if (BOARD_IS_RPI(machine.board_id)) {
		/* rtc was frozen to prevent premature power-off, unfreeze it
		 * now */
		rpi_rtc_run();

		/* wait for the alarm to go off and PMIC to disable power to
		 * SoC */
		while (1);
	}
}

void bsp_disable_watchdog(void)
{
    if(BOARD_IS_RPI(machine.board_id)) 
    {
        u32_t tmp, wdog, rstc;
        tmp = mmio_read(RPI_WDT_BASE+RPI_PM_RSTC);
        rstc = wdog = RPI_PM_PASSWORD;
        rstc |= tmp &~ RPI_PM_RSTC_CONFIGMASK;
        rstc |= RPI_PM_RSTC_FULL_RESET;

        wdog |= ( 10 & RPI_PM_WDOG_TIMEMASK );
        mmio_write( ( RPI_WDT_BASE + RPI_PM_WDOG ), wdog );
        mmio_write( ( RPI_WDT_BASE + RPI_PM_RSTC ), rstc );
	}
}

