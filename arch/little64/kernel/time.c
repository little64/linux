// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/clockchips.h>
#include <linux/of.h>

unsigned long lpj_fine;

void __init time_init(void)
{
	/* Conservative delay calibration baseline. */
	lpj_fine = 1000000 / HZ;

	timer_probe();
}
