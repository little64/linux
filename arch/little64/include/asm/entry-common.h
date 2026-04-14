/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_ENTRY_COMMON_H
#define __ASM_LITTLE64_ENTRY_COMMON_H

#include <linux/sched/task_stack.h>

#include <asm/current.h>
#include <asm/page.h>

/* Little64 currently returns from traps on the task's normal thread stack. */
static __always_inline bool on_thread_stack(void)
{
	return !(((unsigned long)task_stack_page(current) ^ current_stack_pointer) &
		 ~(THREAD_SIZE - 1));
}

#define on_thread_stack on_thread_stack

#endif /* __ASM_LITTLE64_ENTRY_COMMON_H */