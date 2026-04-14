/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_SWITCH_TO_H
#define __ASM_LITTLE64_SWITCH_TO_H

#include <linux/sched.h>

extern struct task_struct *__switch_to(struct task_struct *prev,
				       struct task_struct *next);

#define switch_to(prev, next, last)				\
	do {							\
		(last) = __switch_to((prev), (next));		\
	} while (0)

#endif /* __ASM_LITTLE64_SWITCH_TO_H */
