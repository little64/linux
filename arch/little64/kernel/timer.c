// SPDX-License-Identifier: GPL-2.0-only
/*
 * arch/little64/kernel/timer.c — Little-64 timer clocksource + clockevent
 *
 * Timer MMIO (base 0x08001000, 32 bytes):
 *   +0  (RO, 8B): cycle counter
 *   +8  (RO, 8B): nanosecond counter
 *   +16 (RW, 8B): cycle interval (0 = disabled)
 *   +24 (RW, 8B): nanosecond interval (0 = disabled)
 *
 * When the nanosecond counter exceeds the armed threshold, the device
 * asserts IRQ vector 66 and rearms at (now + interval). The pending bit
 * in the high IRQ-state register bank must be cleared by the IRQ handler.
 */

#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/printk.h>

static void __iomem *timer_base;

#define TIMER_REG_CYCLES	0x00
#define TIMER_REG_NS		0x08
#define TIMER_REG_CYCLE_INTVL	0x10
#define TIMER_REG_NS_INTVL	0x18

/* ------------------------------------------------------------------ */
/*  Clocksource: free-running nanosecond counter                      */
/* ------------------------------------------------------------------ */

static u64 little64_clocksource_read(struct clocksource *cs)
{
	return readq(timer_base + TIMER_REG_NS);
}

static struct clocksource little64_clocksource = {
	.name	= "little64-ns",
	.rating	= 400,
	.read	= little64_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(64),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

/* ------------------------------------------------------------------ */
/*  Clockevent: one-shot nanosecond interval                          */
/* ------------------------------------------------------------------ */

static int little64_set_next_event(unsigned long delta,
				    struct clock_event_device *evt)
{
	u64 now = readq(timer_base + TIMER_REG_NS);
	/* Arm: write absolute target as interval from now */
	writeq(delta, timer_base + TIMER_REG_NS_INTVL);
	pr_info_once("little64-timer: set_next_event delta=%lu now=%llu\n",
		     delta, now);
	(void)now;
	return 0;
}

static int little64_set_state_shutdown(struct clock_event_device *evt)
{
	/* Disarm both interval sources */
	writeq(0, timer_base + TIMER_REG_NS_INTVL);
	writeq(0, timer_base + TIMER_REG_CYCLE_INTVL);
	return 0;
}

static int little64_set_state_oneshot(struct clock_event_device *evt)
{
	/* Nothing special; next_event will arm */
	return 0;
}

static struct clock_event_device little64_clockevent = {
	.name			= "little64-timer",
	.features		= CLOCK_EVT_FEAT_ONESHOT,
	.rating			= 400,
	.set_next_event		= little64_set_next_event,
	.set_state_shutdown	= little64_set_state_shutdown,
	.set_state_oneshot	= little64_set_state_oneshot,
};

/* ------------------------------------------------------------------ */
/*  IRQ handler                                                       */
/* ------------------------------------------------------------------ */

static irqreturn_t little64_timer_irq(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;

	/* Disarm so we don't re-fire immediately */
	writeq(0, timer_base + TIMER_REG_NS_INTVL);

	evt->event_handler(evt);
	return IRQ_HANDLED;
}

/* ------------------------------------------------------------------ */
/*  Init (called from time_init via OF)                                */
/* ------------------------------------------------------------------ */

static int __init little64_timer_init(struct device_node *node)
{
	int irq, ret;

	timer_base = of_iomap(node, 0);
	if (!timer_base) {
		pr_err("little64-timer: failed to map MMIO\n");
		return -ENXIO;
	}

	irq = irq_of_parse_and_map(node, 0);
	if (irq <= 0) {
		pr_err("little64-timer: no IRQ in DT\n");
		return -EINVAL;
	}

	/* Disarm timer */
	writeq(0, timer_base + TIMER_REG_NS_INTVL);
	writeq(0, timer_base + TIMER_REG_CYCLE_INTVL);

	/* Register clocksource — 1 GHz ns counter. */
	ret = clocksource_register_hz(&little64_clocksource, 1000000000UL);
	if (ret) {
		pr_err("little64-timer: clocksource register failed (%d)\n", ret);
		return ret;
	}

	/* Register clockevent */
	little64_clockevent.cpumask = cpumask_of(0);
	clockevents_config_and_register(&little64_clockevent,
					1000000000UL,	/* 1 GHz = 1 ns/tick */
					1,		/* min delta */
					ULONG_MAX);	/* max delta */

	ret = request_irq(irq, little64_timer_irq, IRQF_TIMER,
			  "little64-timer", &little64_clockevent);
	if (ret) {
		pr_err("little64-timer: failed to request IRQ %d (%d)\n",
		       irq, ret);
		return ret;
	}

	pr_info("little64-timer: clocksource + clockevent @ 1 GHz, IRQ %d\n",
		irq);
	return 0;
}

TIMER_OF_DECLARE(little64_timer, "little64,timer", little64_timer_init);
