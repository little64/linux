/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_CURRENT_H
#define __ASM_LITTLE64_CURRENT_H

#include <linux/compiler.h>

#ifndef __ASSEMBLY__

struct task_struct;

extern struct task_struct *little64_current_task;

/*
 * Early Little64 Linux bring-up keeps current in a plain global task pointer.
 *
 * The Little64 LLVM backend reserves R12 as internal scratch, so the kernel
 * must not model current as a fixed-register variable until the toolchain and
 * arch ABI agree on a dedicated preserved register contract.
 */
static __always_inline struct task_struct *get_current(void)
{
	return READ_ONCE(little64_current_task);
}

#define current get_current()

register unsigned long current_stack_pointer __asm__("r13");

#endif /* __ASSEMBLY__ */

#endif /* __ASM_LITTLE64_CURRENT_H */
