/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_CMPXCHG_H
#define __ASM_LITTLE64_CMPXCHG_H

/* Use the generic UP (uniprocessor) implementation for Phase 1.
 * LLR/SCR-based SMP cmpxchg can be added when SMP support is introduced. */
#include <asm-generic/cmpxchg.h>

#endif /* __ASM_LITTLE64_CMPXCHG_H */
