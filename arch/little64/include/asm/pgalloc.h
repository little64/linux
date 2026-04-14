/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_PGALLOC_H
#define __ASM_LITTLE64_PGALLOC_H

#include <linux/mm.h>
#include <linux/string.h>
#include <asm/pgtable.h>
#include <asm-generic/pgalloc.h>

static inline void little64_sync_kernel_mappings(pgd_t *pgd)
{
	memcpy(pgd + USER_PTRS_PER_PGD,
	       init_mm.pgd + USER_PTRS_PER_PGD,
	       (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
}

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *pgd;

	pgd = __pgd_alloc(mm, 0);
	if (likely(pgd != NULL))
		little64_sync_kernel_mappings(pgd);
	return pgd;
}

static inline void pmd_populate_kernel(struct mm_struct *mm,
				       pmd_t *pmd, pte_t *pte)
{
	unsigned long pfn = virt_to_pfn(pte);

	set_pmd(pmd, __pmd((pfn << PTE_PFN_SHIFT) | _PAGE_PRESENT));
}

static inline void pmd_populate(struct mm_struct *mm,
				pmd_t *pmd, pgtable_t pte)
{
	unsigned long pfn = page_to_pfn(pte);

	set_pmd(pmd, __pmd((pfn << PTE_PFN_SHIFT) | _PAGE_PRESENT));
}

static inline void pud_populate(struct mm_struct *mm,
				pud_t *pud, pmd_t *pmd)
{
	unsigned long pfn = virt_to_pfn(pmd);

	set_pud(pud, __pud((pfn << PTE_PFN_SHIFT) | _PAGE_PRESENT));
}

#define __pte_free_tlb(tlb, pte, addr)	pte_free((tlb)->mm, (pte))

#ifndef __PAGETABLE_PMD_FOLDED
#define __pmd_free_tlb(tlb, pmd, addr)	pmd_free((tlb)->mm, (pmd))
#endif

#endif /* __ASM_LITTLE64_PGALLOC_H */
