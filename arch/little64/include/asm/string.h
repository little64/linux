/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_STRING_H
#define __ASM_LITTLE64_STRING_H

/*
 * Arch-specific memset: hand-written byte loop that is smaller and
 * tighter than what the LLVM backend currently emits for the generic
 * C memset.
 */
#define __HAVE_ARCH_MEMSET
extern void *memset(void *s, int c, size_t count);

#include <asm-generic/string.h>

#endif /* __ASM_LITTLE64_STRING_H */
