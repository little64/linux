// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/delay.h>

void __delay(unsigned long loops)
{
	while (loops--)
		barrier();
}

void __const_udelay(unsigned long xloops)
{
	__delay(xloops);
}

void __udelay(unsigned long usecs)
{
	__const_udelay(usecs);
}

void __ndelay(unsigned long nsecs)
{
	__const_udelay(nsecs);
}

void __init calibrate_delay(void)
{
	if (!loops_per_jiffy)
		loops_per_jiffy = 100000;
}
