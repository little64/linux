/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_THREAD_INFO_H
#define __ASM_LITTLE64_THREAD_INFO_H

#include <linux/const.h>
#include <asm/page.h>

#define THREAD_SIZE_ORDER	2
/* THREAD_SIZE is defined in asm/page.h via THREAD_SHIFT */

#ifndef __ASSEMBLY__

#include <asm/types.h>

struct task_struct;

struct thread_info {
	struct task_struct	*task;
	unsigned long		flags;
	int			preempt_count;
	__u32			cpu;
};

#define INIT_THREAD_INFO(tsk)		\
{					\
	.task		= &tsk,		\
	.flags		= 0,		\
	.preempt_count	= INIT_PREEMPT_COUNT, \
	.cpu		= 0,		\
}

/* With THREAD_INFO_IN_TASK, current_thread_info() comes from linux/thread_info.h */

#endif /* __ASSEMBLY__ */

/*
 * Thread flags — bits in thread_info.flags
 */
#define TIF_SIGPENDING		0	/* signal pending */
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_NEED_RESCHED	2	/* rescheduling necessary */
#define TIF_SYSCALL_TRACE	3	/* syscall trace active */
#define TIF_NOTIFY_SIGNAL	4	/* signal notifications exist */
#define TIF_MEMDIE		5	/* is dying */
#define TIF_UPROBE		6	/* uprobe breakpoint or singlestep */
#define TIF_POLLING_NRFLAG	16	/* poll_idle() is polling TIF_NEED_RESCHED */

#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_NOTIFY_SIGNAL	(1 << TIF_NOTIFY_SIGNAL)
#define _TIF_UPROBE		(1 << TIF_UPROBE)
#define _TIF_POLLING_NRFLAG	(1 << TIF_POLLING_NRFLAG)

#define _TIF_WORK_MASK		(_TIF_SIGPENDING | _TIF_NOTIFY_RESUME | \
				 _TIF_NEED_RESCHED | _TIF_NOTIFY_SIGNAL)

#endif /* __ASM_LITTLE64_THREAD_INFO_H */
