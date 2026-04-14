// SPDX-License-Identifier: GPL-2.0-only
#include <linux/ptrace.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/sched/task_stack.h>
#include <linux/uaccess.h>

#include <asm/ptrace.h>
#include <asm/processor.h>

long arch_ptrace(struct task_struct *child, long request,
		 unsigned long addr, unsigned long data)
{
	switch (request) {
	case PTRACE_GET_THREAD_AREA:
		return put_user(task_pt_regs(child)->tp, (unsigned long __user *)data);
	default:
		break;
	}

	return ptrace_request(child, request, addr, data);
}

void ptrace_disable(struct task_struct *child)
{
	pr_warn_once("little64: ptrace_disable() is a bring-up stub\n");
}
