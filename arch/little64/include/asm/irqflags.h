/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_IRQFLAGS_H
#define __ASM_LITTLE64_IRQFLAGS_H

/*
 * cpu_control (SR0) bit layout:
 *   bit 0 = interrupt enable (1 = enabled)
 *   bit 1 = in interrupt
 *   bit 16 = paging enable
 *   bit 17 = user mode
 */

static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	unsigned long idx = 0;	/* SR index for cpu_control (SR0) */
	__asm__ __volatile__("LSR %1, %0" : "=r"(flags) : "r"(idx));
	return flags;
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	unsigned long idx = 0;
	__asm__ __volatile__("SSR %0, %1" : : "r"(idx), "r"(flags) : "memory");
}

static inline void arch_local_irq_enable(void)
{
	unsigned long flags = arch_local_save_flags();
	arch_local_irq_restore(flags | 1UL);
}

static inline void arch_local_irq_disable(void)
{
	unsigned long flags = arch_local_save_flags();
	arch_local_irq_restore(flags & ~1UL);
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags = arch_local_save_flags();
	arch_local_irq_restore(flags & ~1UL);
	return flags;
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !(flags & 1UL);
}

static inline int arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

#endif /* __ASM_LITTLE64_IRQFLAGS_H */
