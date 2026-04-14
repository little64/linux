// SPDX-License-Identifier: GPL-2.0-only
/*
 * arch/little64/kernel/irq.c — Interrupt controller driver
 *
 * The Little-64 interrupt controller is CPU-internal:
 *   SR17/SR18 hold the low/high IRQ enable masks (bit set = unmasked)
 *   SR19/SR20 hold the low/high IRQ pending bits (set by HW, cleared by SW)
 *
 * Hardware IRQ vectors currently occupy 65..127, exposed through a 1-cell DT
 * binding (#interrupt-cells = <1>).
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/printk.h>

#include <asm/interrupt_vectors.h>
#include <asm/special_registers.h>

/* ------------------------------------------------------------------ */
/*  SR helpers                                                         */
/* ------------------------------------------------------------------ */

static inline unsigned long read_sr(unsigned long idx)
{
	unsigned long val;
	asm volatile("LSR %1, %0" : "=r"(val) : "r"(idx));
	return val;
}

static inline void write_sr(unsigned long idx, unsigned long val)
{
	asm volatile("SSR %0, %1" : : "r"(idx), "r"(val) : "memory");
}

int little64_handle_irq(unsigned int hwirq);

static inline unsigned long little64_irq_mask_sr(unsigned long vector)
{
	return little64_irq_bank(vector) == 0 ? LITTLE64_SR_INTERRUPT_MASK : LITTLE64_SR_INTERRUPT_MASK_HIGH;
}

static inline unsigned long little64_irq_state_sr(unsigned long vector)
{
	return little64_irq_bank(vector) == 0 ? LITTLE64_SR_INTERRUPT_STATES : LITTLE64_SR_INTERRUPT_STATES_HIGH;
}

static inline unsigned long little64_irq_valid_bits(unsigned long vector)
{
	return little64_irq_bank(vector) == 0 ? 0UL : LITTLE64_IRQ_MASK_HIGH_VALID_BITS;
}

/* ------------------------------------------------------------------ */
/*  irq_chip callbacks                                                 */
/* ------------------------------------------------------------------ */

static void little64_irq_mask(struct irq_data *d)
{
	unsigned long mask;
	unsigned long bit;

	if (!little64_is_irq_vector(d->hwirq))
		return;

	bit = little64_irq_bit(d->hwirq);
	mask = read_sr(little64_irq_mask_sr(d->hwirq));
	mask &= ~bit;
	mask &= little64_irq_valid_bits(d->hwirq);
	write_sr(little64_irq_mask_sr(d->hwirq), mask);
}

static void little64_irq_unmask(struct irq_data *d)
{
	unsigned long mask;
	unsigned long bit;

	if (!little64_is_irq_vector(d->hwirq))
		return;

	bit = little64_irq_bit(d->hwirq);
	mask = read_sr(little64_irq_mask_sr(d->hwirq));
	mask |= bit;
	mask &= little64_irq_valid_bits(d->hwirq);
	write_sr(little64_irq_mask_sr(d->hwirq), mask);
}

static void little64_irq_ack(struct irq_data *d)
{
	unsigned long states;
	unsigned long bit;

	if (!little64_is_irq_vector(d->hwirq))
		return;

	bit = little64_irq_bit(d->hwirq);
	states = read_sr(little64_irq_state_sr(d->hwirq));
	states &= ~bit;
	states &= little64_irq_valid_bits(d->hwirq);
	write_sr(little64_irq_state_sr(d->hwirq), states);
}

static struct irq_chip little64_irq_chip = {
	.name		= "little64-intc",
	.irq_mask	= little64_irq_mask,
	.irq_unmask	= little64_irq_unmask,
	.irq_ack	= little64_irq_ack,
};

/* ------------------------------------------------------------------ */
/*  IRQ domain                                                         */
/* ------------------------------------------------------------------ */

static struct irq_domain *little64_irq_domain;

int little64_handle_irq(unsigned int hwirq)
{
	if (!little64_irq_domain)
		return -EINVAL;
	if (!little64_is_irq_vector(hwirq))
		return -EINVAL;
	return generic_handle_domain_irq(little64_irq_domain, hwirq);
}

static int little64_irq_domain_map(struct irq_domain *d, unsigned int virq,
				    irq_hw_number_t hwirq)
{
	if (!little64_is_irq_vector(hwirq))
		return -EINVAL;

	irq_set_chip_and_handler(virq, &little64_irq_chip, handle_edge_irq);
	irq_set_noprobe(virq);
	return 0;
}

static const struct irq_domain_ops little64_irq_domain_ops = {
	.map		= little64_irq_domain_map,
	.xlate		= irq_domain_xlate_onecell,
};

/* ------------------------------------------------------------------ */
/*  OF match + init                                                   */
/* ------------------------------------------------------------------ */

static int __init little64_intc_of_init(struct device_node *node,
					 struct device_node *parent)
{
	/* Start with all IRQs masked */
	write_sr(LITTLE64_SR_INTERRUPT_MASK, 0);
	write_sr(LITTLE64_SR_INTERRUPT_MASK_HIGH, 0);

	little64_irq_domain = irq_domain_add_linear(node, LITTLE64_VECTOR_COUNT,
						      &little64_irq_domain_ops,
						      NULL);
	if (!little64_irq_domain) {
		pr_err("little64-intc: failed to create IRQ domain\n");
		return -ENOMEM;
	}

	pr_info("little64-intc: IRQ vectors %u-%u initialised\n",
		(unsigned int)LITTLE64_IRQ_VECTOR_BASE,
		(unsigned int)LITTLE64_MAX_VECTOR);
	return 0;
}

IRQCHIP_DECLARE(little64_intc, "little64,intc", little64_intc_of_init);

/* ------------------------------------------------------------------ */
/*  init_IRQ — called from start_kernel()                              */
/* ------------------------------------------------------------------ */

void __init init_IRQ(void)
{
	irqchip_init();
}
