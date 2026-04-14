/* SPDX-License-Identifier: GPL-2.0-only */
/* Page table bits for Little-64 SV39 page tables */

#ifndef __ASM_LITTLE64_PGTABLE_BITS_H
#define __ASM_LITTLE64_PGTABLE_BITS_H

/* PTE layout (SV39-compatible):
 *  Bit  0: V    (Valid)
 *  Bit  1: R    (Readable)
 *  Bit  2: W    (Writable)
 *  Bit  3: X    (Executable)
 *  Bit  4: U    (User)
 *  Bit  5: G    (Global)
 *  Bit  6: A    (Accessed - used for swapping)
 *  Bit  7: D    (Dirty - used for swapping)
 *  Bits [53:10]: PPN (Physical Page Number)
 */

#define _PAGE_V		(1UL << 0)	/* Valid */
#define _PAGE_R		(1UL << 1)	/* Read */
#define _PAGE_W		(1UL << 2)	/* Write */
#define _PAGE_X		(1UL << 3)	/* Execute */
#define _PAGE_U		(1UL << 4)	/* User */
#define _PAGE_G		(1UL << 5)	/* Global */
#define _PAGE_A		(1UL << 6)	/* Accessed */
#define _PAGE_D		(1UL << 7)	/* Dirty */

#define _PAGE_PRESENT	(_PAGE_V)
#define _PAGE_READ	(_PAGE_R)
#define _PAGE_WRITE	(_PAGE_W)
#define _PAGE_EXEC	(_PAGE_X)
#define _PAGE_USER	(_PAGE_U)
#define _PAGE_GLOBAL	(_PAGE_G)
#define _PAGE_SOFT	(_PAGE_A | _PAGE_D)
#define _PAGE_DIRTY	(_PAGE_D)
#define _PAGE_ACCESSED	(_PAGE_A)

#define _PAGE_CHG_MASK	(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | \
			 _PAGE_USER | _PAGE_GLOBAL | _PAGE_ACCESSED | _PAGE_DIRTY)

/* Page table entry combinations */
#define _PAGE_BASE	(_PAGE_V | _PAGE_A | _PAGE_U)

#define PAGE_NONE	__pgprot(0)
#define PAGE_READ	__pgprot(_PAGE_BASE | _PAGE_R)
#define PAGE_WRITE	__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_W | _PAGE_D)
#define PAGE_EXEC	__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_X)
#define PAGE_READ_EXEC	__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_X)
#define PAGE_WRITE_EXEC	__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_W | _PAGE_X | _PAGE_D)

#define PAGE_COPY	PAGE_READ
#define PAGE_COPY_EXEC	PAGE_READ_EXEC

#define _PAGE_KERNEL	(_PAGE_V | _PAGE_R | _PAGE_W | _PAGE_X | _PAGE_G)
#define PAGE_KERNEL	__pgprot(_PAGE_KERNEL)
#define PAGE_KERNEL_EXEC	__pgprot(_PAGE_KERNEL | _PAGE_X)

/* ioremap: emulator has no cache coherency issues, use kernel mapping flags */
#define _PAGE_IOREMAP	_PAGE_KERNEL

#define PTE_PFN_SHIFT	10
#define _PFN_MASK	(((1ULL << 54) - 1) >> PTE_PFN_SHIFT)

#endif /* __ASM_LITTLE64_PGTABLE_BITS_H */
