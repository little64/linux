// SPDX-License-Identifier: GPL-2.0-only
#include <linux/sched/task_stack.h>

unsigned long __get_wchan(struct task_struct *p)
{
	/* Early bring-up: unwind support is not implemented yet. */
	return 0;
}
