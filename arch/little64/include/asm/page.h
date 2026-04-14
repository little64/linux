/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_PAGE_H
#define __ASM_LITTLE64_PAGE_H

#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#define PAGE_SHIFT	12
#define THREAD_SHIFT	2  /* THREAD_SIZE = PAGE_SIZE * 4 = 16KB */
#define THREAD_SIZE	(PAGE_SIZE << THREAD_SHIFT)

#define HPAGE_SHIFT	21
#define HPAGE_SIZE	(1UL << HPAGE_SHIFT)
#define HPAGE_MASK	(~(HPAGE_SIZE - 1))

#define PAGE_OFFSET	(0xFFFFFFC000000000UL)
#define KERNEL_PHYS_BASE (0x00100000UL)
#define KERNEL_LINK_ADDR (PAGE_OFFSET)

#ifndef __ASSEMBLY__

#include <linux/string.h>

extern phys_addr_t little64_phys_ram_base;

#define clear_page(page)	memset((void *)(page), 0, PAGE_SIZE)
#define copy_page(to, from)	memcpy((to), (from), PAGE_SIZE)

struct page;

typedef struct {
	unsigned long pte;
} pte_t;

typedef struct {
	unsigned long pmd;
} pmd_t;

typedef struct {
	unsigned long pgd;
} pgd_t;

typedef struct {
	unsigned long pgprot;
} pgprot_t;

typedef struct page *pgtable_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) })
#define __pmd(x)	((pmd_t) { (x) })
#define __pgd(x)	((pgd_t) { (x) })
#define __pgprot(x)	((pgprot_t) { (x) })

/*
 * Kernel VMA starts at PAGE_OFFSET while kernel image is physically loaded at
 * KERNEL_PHYS_BASE. Keep helpers consistent with vmlinux.lds load offset.
 */
#define __pa(x)		((unsigned long)(x) - PAGE_OFFSET + KERNEL_PHYS_BASE)
#define __va(x)		((void *)((unsigned long)(x) - KERNEL_PHYS_BASE + PAGE_OFFSET))

#define __pa_symbol(x)	__pa(RELOC_HIDE((unsigned long)(x), 0))
#define __va_symbol(x)	__va(RELOC_HIDE((unsigned long)(x), 0))

#define virt_to_pfn(kaddr)	(__pa((unsigned long)(kaddr)) >> PAGE_SHIFT)
#define virt_to_page(x)		(pfn_to_page(virt_to_pfn(x)))

/* pfn_valid provided by asm-generic/memory_model.h using max_mapnr */
#define ARCH_PFN_OFFSET		(PFN_DOWN(little64_phys_ram_base))
#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>

#define virt_addr_valid(kaddr)	pfn_valid(__pa((unsigned long)(kaddr)) >> PAGE_SHIFT)

/* Zero page for anonymous mappings */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];
#define ZERO_PAGE(vaddr)	virt_to_page(empty_zero_page)

/* Page copy helpers */
static inline void copy_user_page(void *to, const void *from,
				  unsigned long vaddr, struct page *page)
{
	(void)vaddr;
	(void)page;
	copy_page(to, from);
}
#define copy_user_page copy_user_page

static inline void clear_user_page(void *addr, unsigned long vaddr,
				   struct page *page)
{
	(void)vaddr;
	(void)page;
	clear_page(addr);
}
#define clear_user_page clear_user_page

#endif /* __ASSEMBLY__ */

#endif /* __ASM_LITTLE64_PAGE_H */
