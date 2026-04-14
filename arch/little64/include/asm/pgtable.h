/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_PGTABLE_H
#define __ASM_LITTLE64_PGTABLE_H

#include <linux/const.h>
#include <linux/sizes.h>
#include <asm/pgtable-bits.h>
#include <asm/page.h>

/*
 * SV39 3-level page table (PGD → PMD → PTE):
 *   PGD = L2: 512 entries, PGDIR_SHIFT = 30
 *   PMD = L1: 512 entries, PMD_SHIFT   = 21
 *   PTE = L0: 512 entries, PAGE_SHIFT  = 12
 *
 * P4D and PUD are folded into PGD (PGTABLE_LEVELS=3).
 * pgtable-nopud.h (which includes pgtable-nop4d.h) provides:
 *   - p4d_t = struct { pgd_t }  (P4D = PGD alias)
 *   - pud_t = struct { p4d_t }  (PUD = PGD alias)
 *   - p4d_* is folded into pgd_t by pgtable-nop4d.h
 *   - pud_* wraps the real top-level pgd entry operations below
 *
 * The REAL top-level entry operations are the pud_* functions below.
 */
#define PGDIR_SHIFT	30
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE - 1))
#define PTRS_PER_PGD	512

#define PMD_SHIFT	21
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE - 1))
#define PTRS_PER_PMD	512

#define PTRS_PER_PTE	512

#define KERN_VIRT_START	PAGE_OFFSET
#define USER_PTRS_PER_PGD	(((PAGE_OFFSET) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

/*
 * vmalloc range — must stay within SV39 canonical kernel VA space.
 * Canonical kernel range: [0xFFFFFFC000000000, 0xFFFFFFFFFFFFFFFF]
 * (bit 38 = 1, bits 63-39 = all 1s).
 *
 * Place vmalloc well above the kernel linear mapping (which occupies
 * PAGE_OFFSET + phys_ram, currently 64 MiB).
 */
#define VMALLOC_START	(0xFFFFFFC800000000UL)
#define VMALLOC_END	(0xFFFFFFD000000000UL)
#define VMALLOC_SIZE	(VMALLOC_END - VMALLOC_START)

/* PFN_PTE_SHIFT: used by linux/pgtable.h pte_next_pfn() */
#define PFN_PTE_SHIFT	PTE_PFN_SHIFT

#ifndef __ASSEMBLY__

extern pgd_t swapper_pg_dir[];

#include <asm/mmu_context.h>

/*
 * Little64 uses fixed 3-level paging. Linux still exposes pgd/p4d/pud, but
 * only p4d is folded; pud is the real top-level hardware entry type.
 */
#define __PAGETABLE_P4D_FOLDED 1
#define __PAGETABLE_PUD_FOLDED 1
typedef struct { unsigned long p4d; } p4d_t;
typedef struct { unsigned long pud; } pud_t;

#define p4d_val(x)			((x).p4d)
#define __p4d(x)			((p4d_t) { (x) })
#define pud_val(x)			((x).pud)
#define __pud(x)			((pud_t) { (x) })

#define P4D_SHIFT		PGDIR_SHIFT
#define PTRS_PER_P4D		1
#define P4D_SIZE		(1UL << P4D_SHIFT)
#define P4D_MASK		(~(P4D_SIZE - 1))

#define PUD_SHIFT		P4D_SHIFT
#define PTRS_PER_PUD		1
#define PUD_SIZE		(1UL << PUD_SHIFT)
#define PUD_MASK		(~(PUD_SIZE - 1))

static inline int p4d_none(p4d_t p4d)		{ return 0; }
static inline int p4d_bad(p4d_t p4d)		{ return 0; }
static inline int p4d_present(p4d_t p4d)	{ return 1; }
static inline void p4d_clear(p4d_t *p4d)	{ }

#define p4d_ERROR(e) \
	pr_err("%s:%d: bad p4d %016lx\n", __FILE__, __LINE__, p4d_val(e))
#define pud_ERROR(e) \
	pr_err("%s:%d: bad pud %016lx\n", __FILE__, __LINE__, pud_val(e))
#define pgd_populate(mm, pgd, p4d)		do { } while (0)
#define pgd_populate_safe(mm, pgd, p4d)	do { } while (0)
#define p4d_populate(mm, p4d, pud)		do { } while (0)
#define p4d_populate_safe(mm, p4d, pud)	do { } while (0)

static inline void set_pud(pud_t *pudp, pud_t pud) { *pudp = pud; }
static inline void pud_clear(pud_t *pudp) { set_pud(pudp, __pud(0)); }
static inline void set_p4d(p4d_t *p4dp, p4d_t p4d)
{
	set_pud((pud_t *)p4dp, (pud_t) { p4d_val(p4d) });
}

static inline p4d_t *p4d_offset(pgd_t *pgd, unsigned long address)
{
	return (p4d_t *)pgd;
}

static inline pud_t *pud_offset(p4d_t *p4d, unsigned long address)
{
	return (pud_t *)p4d;
}
#define pud_offset pud_offset

#define p4d_alloc_one(mm, address)		NULL
#define p4d_free(mm, x)			do { } while (0)
#define p4d_free_tlb(tlb, x, a)		do { } while (0)
#define pud_alloc_one(mm, address)		NULL
#define pud_free(mm, x)			do { } while (0)
#define pud_free_tlb(tlb, x, a)		do { } while (0)

#undef p4d_addr_end
#define p4d_addr_end(addr, end)		(end)
#undef pud_addr_end
#define pud_addr_end(addr, end)		(end)

static inline int pgd_none(pgd_t pgd)		{ return 0; }
static inline int pgd_present(pgd_t pgd)	{ return 1; }
static inline int pgd_bad(pgd_t pgd)		{ return 0; }
static inline void pgd_clear(pgd_t *pgdp)	{ }
static inline void set_pgd(pgd_t *pgdp, pgd_t pgd)
{
	set_p4d((p4d_t *)pgdp, (p4d_t) { pgd_val(pgd) });
}

static inline unsigned long pgd_pfn(pgd_t pgd)
{
	return pgd_val(pgd) >> PTE_PFN_SHIFT;
}

static inline int pud_present(pud_t pud) { return pud_val(pud) & _PAGE_PRESENT; }
static inline int pud_none(pud_t pud)    { return pud_val(pud) == 0; }

#define pud_leaf pud_leaf
static inline bool pud_leaf(pud_t pud)
{
	return pud_present(pud) && (pud_val(pud) & (_PAGE_READ | _PAGE_WRITE | _PAGE_EXEC));
}

static inline int pud_bad(pud_t pud)	   { return !pud_present(pud) || pud_leaf(pud); }

static inline unsigned long pud_pfn(pud_t pud)
{
	return pud_val(pud) >> PTE_PFN_SHIFT;
}

static inline pmd_t *pud_pgtable(pud_t pud)
{
	return (pmd_t *)__va(pud_pfn(pud) << PAGE_SHIFT);
}
#define pud_pgtable pud_pgtable

#define p4d_page(p4d)			(pud_page((pud_t) { p4d_val(p4d) }))
#define p4d_pgtable(p4d)		((pud_t *)(pud_pgtable((pud_t) { p4d_val(p4d) })))

static inline p4d_t *pgd_pgtable(pgd_t pgd)
{
	return (p4d_t *)p4d_pgtable((p4d_t) { pgd_val(pgd) });
}

#define pgd_page(pgd)		pfn_to_page(pgd_pfn(pgd))
#define pgd_page_vaddr(pgd)	((unsigned long)pgd_pgtable(pgd))

#define pud_page(pud)		pfn_to_page(pud_pfn(pud))

/*
 * pmd_offset: given a pud entry (= PGD entry in 3-level), compute the
 * address of the PMD entry for `addr`.
 */
static inline pmd_t *pmd_offset(pud_t *pud, unsigned long addr)
{
	return pud_pgtable(*pud) + ((addr >> PMD_SHIFT) & (PTRS_PER_PMD - 1));
}
#define pmd_offset pmd_offset

/* PMD operations */
static inline int pmd_none(pmd_t pmd)    { return pmd_val(pmd) == 0; }
static inline int pmd_present(pmd_t pmd) { return pmd_val(pmd) & _PAGE_PRESENT; }
static inline int pmd_bad(pmd_t pmd)     { return !pmd_present(pmd); }

static inline pte_t *pmd_page_vaddr(pmd_t pmd)
{
	return (pte_t *)__va((pmd_val(pmd) >> PTE_PFN_SHIFT) << PAGE_SHIFT);
}

static inline unsigned long pmd_pfn(pmd_t pmd)
{
	return pmd_val(pmd) >> PTE_PFN_SHIFT;
}

#define pmd_page(pmd)		pfn_to_page(pmd_pfn(pmd))

static inline void set_pmd(pmd_t *pmdp, pmd_t pmd) { *pmdp = pmd; }
static inline void pmd_clear(pmd_t *pmdp) { set_pmd(pmdp, __pmd(0)); }

/* PTE operations */
#define pte_pfn(pte)		(pte_val(pte) >> PTE_PFN_SHIFT)
#define pfn_pte(pfn, prot)	__pte(((pfn) << PTE_PFN_SHIFT) | pgprot_val(prot))

static inline int pte_none(pte_t pte)    { return pte_val(pte) == 0; }
static inline int pte_present(pte_t pte) { return pte_val(pte) & _PAGE_PRESENT; }
static inline int pte_valid(pte_t pte)   { return pte_present(pte); }
static inline int pte_read(pte_t pte)    { return pte_val(pte) & _PAGE_READ; }
static inline int pte_write(pte_t pte)   { return pte_val(pte) & _PAGE_WRITE; }
static inline int pte_exec(pte_t pte)    { return pte_val(pte) & _PAGE_EXEC; }
static inline int pte_user(pte_t pte)    { return pte_val(pte) & _PAGE_USER; }
static inline int pte_dirty(pte_t pte)   { return pte_val(pte) & _PAGE_DIRTY; }
static inline int pte_young(pte_t pte)   { return pte_val(pte) & _PAGE_ACCESSED; }

#define pte_page(pte)		pfn_to_page(pte_pfn(pte))
#define pte_modify(pte, newprot)	__pte((pte_val(pte) & ~_PAGE_CHG_MASK) | pgprot_val(newprot))

static inline pte_t pte_wrprotect(pte_t pte)     { return __pte(pte_val(pte) & ~_PAGE_WRITE); }
static inline pte_t pte_mkwrite_novma(pte_t pte) { return __pte(pte_val(pte) | _PAGE_WRITE); }
static inline pte_t pte_mkread(pte_t pte)        { return __pte(pte_val(pte) | _PAGE_READ); }
static inline pte_t pte_mkexec(pte_t pte)        { return __pte(pte_val(pte) | _PAGE_EXEC); }
static inline pte_t pte_mkuser(pte_t pte)        { return __pte(pte_val(pte) | _PAGE_USER); }
static inline pte_t pte_mkglobal(pte_t pte)      { return __pte(pte_val(pte) | _PAGE_GLOBAL); }
static inline pte_t pte_mkdirty(pte_t pte)       { return __pte(pte_val(pte) | _PAGE_DIRTY); }
static inline pte_t pte_mkyoung(pte_t pte)       { return __pte(pte_val(pte) | _PAGE_ACCESSED); }
static inline pte_t pte_mkclean(pte_t pte)       { return __pte(pte_val(pte) & ~_PAGE_DIRTY); }
static inline pte_t pte_mkold(pte_t pte)         { return __pte(pte_val(pte) & ~_PAGE_ACCESSED); }

static inline void set_pte(pte_t *ptep, pte_t pte) { *ptep = pte; }

#define pte_clear(mm, addr, ptep)  set_pte(ptep, __pte(0))

#define pte_ERROR(e) \
	pr_err("%s:%d: bad pte %016lx\n", __FILE__, __LINE__, pte_val(e))
#define pmd_ERROR(e) \
	pr_err("%s:%d: bad pmd %016lx\n", __FILE__, __LINE__, pmd_val(e))
#define pgd_ERROR(e) \
	pr_err("%s:%d: bad pgd %016lx\n", __FILE__, __LINE__, pgd_val(e))

/*
 * Swap PTE encoding:
 *   Bit 0:      0 (not present)
 *   Bit 1:      SWP_EXCLUSIVE
 *   Bits [7:2]: swap type (6 bits)
 *   Bits [63:10]: swap offset (page number)
 */
#define _PAGE_SWP_EXCLUSIVE	(1UL << 1)

#define __swp_type(x)		(((x).val >> 2) & 0x3f)
#define __swp_offset(x)		((x).val >> 10)
#define __swp_entry(type, offset) \
	((swp_entry_t){ ((unsigned long)(type) << 2) | ((unsigned long)(offset) << 10) })
#define __pte_to_swp_entry(pte)	((swp_entry_t){ pte_val(pte) })
#define __swp_entry_to_pte(swp)	(__pte((swp).val))

static inline int pte_swp_exclusive(pte_t pte)
{
	return pte_val(pte) & _PAGE_SWP_EXCLUSIVE;
}
static inline pte_t pte_swp_mkexclusive(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SWP_EXCLUSIVE);
}
static inline pte_t pte_swp_clear_exclusive(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_SWP_EXCLUSIVE);
}

static inline void update_mmu_cache(struct vm_area_struct *vma,
				    unsigned long address,
				    pte_t *ptep)
{
}

#define update_mmu_cache_range(vmf, vma, addr, ptep, nr) do { } while (0)

#endif /* __ASSEMBLY__ */

#endif /* __ASM_LITTLE64_PGTABLE_H */
