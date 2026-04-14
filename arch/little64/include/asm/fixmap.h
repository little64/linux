/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_FIXMAP_H
#define __ASM_LITTLE64_FIXMAP_H

#include <linux/bug.h>
#include <asm/page.h>
#include <asm/pgtable.h>

enum fixed_addresses {
	FIX_EARLYCON_MEM_BASE,
	FIX_TEXT_POKE0,
	__end_of_fixed_addresses
};

#define FIXADDR_TOP	((unsigned long)(-2 * PAGE_SIZE))
#define FIXADDR_SIZE	(__end_of_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_START	(FIXADDR_TOP - FIXADDR_SIZE)
#define FIXMAP_PAGE_IO	__pgprot(_PAGE_IOREMAP)

extern void __set_fixmap(enum fixed_addresses idx, phys_addr_t phys,
			 pgprot_t prot);

#include <asm-generic/fixmap.h>

#endif /* __ASM_LITTLE64_FIXMAP_H */