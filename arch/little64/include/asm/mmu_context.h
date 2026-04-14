/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_MMU_CONTEXT_H
#define __ASM_LITTLE64_MMU_CONTEXT_H

#include <linux/mm_types.h>
#include <asm/mmu.h>
#include <asm/page.h>
#include <asm/special_registers.h>

static inline void switch_mm(struct mm_struct *prev,
			     struct mm_struct *next,
			     struct task_struct *tsk)
{
	unsigned long page_table_root;
	unsigned long idx;

	if (unlikely(prev == next))
		return;

	/* Write new page table root to SR11 (page_table_root_physical) */
	page_table_root = __pa(next->pgd);
	idx = LITTLE64_SR_PAGE_TABLE_ROOT_PHYSICAL;
	asm volatile("SSR %0, %1" : : "r"(idx), "r"(page_table_root) : "memory");
}

static inline void activate_mm(struct mm_struct *prev,
				struct mm_struct *next)
{
	switch_mm(prev, next, NULL);
}

static inline int init_new_context(struct task_struct *tsk,
				   struct mm_struct *mm)
{
	return 0;
}

static inline void destroy_context(struct mm_struct *mm) {}
static inline void enter_lazy_tlb(struct mm_struct *mm,
				  struct task_struct *tsk) {}
static inline void deactivate_mm(struct task_struct *tsk,
				 struct mm_struct *mm) {}
static inline void arch_exit_mmap(struct mm_struct *mm) {}
static inline int arch_dup_mmap(struct mm_struct *oldmm,
				struct mm_struct *mm)
{
	return 0;
}
static inline bool arch_vma_access_permitted(struct vm_area_struct *vma,
					     bool write,
					     bool execute,
					     bool foreign)
{
	return true;
}
static inline void arch_bprm_mm_init(struct mm_struct *mm,
				     struct vm_area_struct *vma) {}
static inline void arch_unmap(struct mm_struct *mm,
			      unsigned long start, unsigned long end) {}

#endif /* __ASM_LITTLE64_MMU_CONTEXT_H */
