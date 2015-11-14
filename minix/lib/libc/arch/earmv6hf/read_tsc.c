#include <sys/types.h>
#include <minix/minlib.h>

void
read_tsc(u32_t *hi, u32_t *lo)
{
/* Read Clock Cycle Counter (CCNT). Intel calls it Time Stamp Counter (TSC) */
	u32_t ccnt;
	/* Setup for ArmV6*/
	asm volatile ("MRC p15, 0, %0, c15, c12, 1\t\n" : "=r" (ccnt) : : "%0");

	/* The ARMv7-A clock cycle counter is only 32-bits, but read_tsc is
	 * expected to return a 64-bit value. hi is therefore always 0.
	 */
	*hi = 0;
	*lo = ccnt;
}
