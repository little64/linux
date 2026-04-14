/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_IO_H
#define __ASM_LITTLE64_IO_H

/* Little64 currently has no legacy I/O port space; use MMIO only. */
#ifdef CONFIG_HAS_IOPORT_MAP
#ifndef ioport_map
#define ioport_map ioport_map
static inline void __iomem *ioport_map(unsigned long port, unsigned int nr)
{
	return NULL;
}
#endif

#ifndef ioport_unmap
#define ioport_unmap ioport_unmap
static inline void ioport_unmap(void __iomem *p)
{
}
#endif

#define ARCH_HAS_GENERIC_IOPORT_MAP
#endif

#include <asm-generic/io.h>

#endif /* __ASM_LITTLE64_IO_H */
