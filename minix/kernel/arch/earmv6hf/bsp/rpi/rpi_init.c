#include "kernel/kernel.h"
#include <sys/types.h>
#include "bsp_init.h"
#include "bsp_padconf.h"
#include "rpi_rtc.h"
#include "bsp_reset.h"

void
bsp_init()
{
	/* map memory for padconf */
    /* bcm2835 does not have pad ctrl */
	bsp_padconf_init();

	/* map memory for rtc */
	rpi_rtc_init();

	/* map memory for reset control */
	bsp_reset_init();

	/* disable watchdog */
	bsp_disable_watchdog();
}
