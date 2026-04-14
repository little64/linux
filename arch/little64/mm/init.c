/* SPDX-License-Identifier: GPL-2.0-only */
/* arch/little64/mm/init.c - Memory initialization */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/mman.h>
#include <linux/pgtable.h>
#include <asm/fixmap.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

void __init paging_init(void);

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_bss;
pmd_t little64_fixmap_pmd[PTRS_PER_PMD] __page_aligned_bss;
pte_t little64_fixmap_pte[PTRS_PER_PTE] __page_aligned_bss;
unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)] __page_aligned_bss;

static const pgprot_t protection_map[16] = {
	[VM_NONE] = PAGE_NONE,
	[VM_READ] = PAGE_READ,
	[VM_WRITE] = PAGE_WRITE,
	[VM_WRITE | VM_READ] = PAGE_WRITE,
	[VM_EXEC] = PAGE_EXEC,
	[VM_EXEC | VM_READ] = PAGE_READ_EXEC,
	[VM_EXEC | VM_WRITE] = PAGE_WRITE_EXEC,
	[VM_EXEC | VM_WRITE | VM_READ] = PAGE_WRITE_EXEC,
	[VM_SHARED] = PAGE_NONE,
	[VM_SHARED | VM_READ] = PAGE_READ,
	[VM_SHARED | VM_WRITE] = PAGE_WRITE,
	[VM_SHARED | VM_WRITE | VM_READ] = PAGE_WRITE,
	[VM_SHARED | VM_EXEC] = PAGE_EXEC,
	[VM_SHARED | VM_EXEC | VM_READ] = PAGE_READ_EXEC,
	[VM_SHARED | VM_EXEC | VM_WRITE] = PAGE_WRITE_EXEC,
	[VM_SHARED | VM_EXEC | VM_WRITE | VM_READ] = PAGE_WRITE_EXEC,
};

DECLARE_VM_GET_PAGE_PROT

void __set_fixmap(enum fixed_addresses idx, phys_addr_t phys, pgprot_t prot)
{
	unsigned long addr = __fix_to_virt(idx);
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	BUG_ON(idx >= __end_of_fixed_addresses);

	pud = pud_offset(p4d_offset(pgd_offset_k(addr), addr), addr);
	if (pud_none(*pud))
		pud_populate(&init_mm, pud, little64_fixmap_pmd);

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd))
		pmd_populate_kernel(&init_mm, pmd, little64_fixmap_pte);

	pte = pte_offset_kernel(pmd, addr);
	if (pgprot_val(prot))
		set_pte(pte, pfn_pte(PFN_DOWN(phys), prot));
	else
		pte_clear(&init_mm, addr, pte);

	flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
}

void __init arch_zone_limits_init(unsigned long *max_zone_pfn)
{
	max_zone_pfn[ZONE_NORMAL] = max_low_pfn;
}

void __init paging_init(void)
{
	/* Page tables set up in head.S. Nothing more to do in Phase 1. */
}

void __init mem_init(void)
{
	/* Memory stats printed by generic code */
}
