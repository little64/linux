/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * TLB flush operations for Little-64.
 * The emulator has no real TLB, so flushes are no-ops.
 * However, we still need to rewrite SR11 to signal the hardware
 * when the page table root changes (handled by switch_mm in mmu_context.h).
 */
#ifndef __ASM_LITTLE64_TLBFLUSH_H
#define __ASM_LITTLE64_TLBFLUSH_H

#include <linux/mm_types.h>
#include <asm/page.h>
#include <asm/pgtable.h>

static inline void little64_reload_page_table_root(unsigned long root_phys)
{
	unsigned long idx = 11;

	asm volatile("SSR %0, %1" : : "r"(idx), "r"(root_phys) : "memory");
}

static inline void little64_flush_kernel_mappings(void)
{
	little64_reload_page_table_root(__pa(init_mm.pgd));
}

static inline void flush_tlb_all(void)
{
	little64_flush_kernel_mappings();
}

static inline void flush_tlb_mm(struct mm_struct *mm)
{
	if (mm && mm->pgd)
		little64_reload_page_table_root(__pa(mm->pgd));
	else
		little64_flush_kernel_mappings();
}

static inline void flush_tlb_page(struct vm_area_struct *vma,
				  unsigned long addr)
{
	flush_tlb_mm(vma ? vma->vm_mm : NULL);
}

static inline void flush_tlb_range(struct vm_area_struct *vma,
				   unsigned long start, unsigned long end)
{
	flush_tlb_mm(vma ? vma->vm_mm : NULL);
}

static inline void flush_tlb_kernel_range(unsigned long start,
					  unsigned long end)
{
	little64_flush_kernel_mappings();
}

#endif /* __ASM_LITTLE64_TLBFLUSH_H */
