// SPDX-License-Identifier: GPL-2.0-only
#include <linux/kernel.h>
#include <linux/cpu.h>
#include <linux/sched/task_stack.h>
#include <linux/ptrace.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/elfcore.h>
#include <linux/init_task.h>
#include <linux/cpuidle.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/processor.h>
#include <asm/irqflags.h>

void show_regs(struct pt_regs *regs);
void show_stack(struct task_struct *task, unsigned long *sp,
		const char *loglvl);
int do_kernel_thread_entry(void);

/* ret_from_fork is in entry.S */
extern void ret_from_fork(void);

struct task_struct *little64_current_task = &init_task;

/*
 * Called from ret_from_fork (entry.S) after schedule_tail().
 * If current->thread.fn is set (kernel thread), call it.
 * A successful kernel_execve() clears PF_KTHREAD and returns here so the
 * architecture can resume through the user-mode trap-exit path instead of
 * killing PID 1 on the first real init handoff.
 * Returns 1 when ret_from_fork should continue through trap_exit.
 */
int do_kernel_thread_entry(void)
{
	unsigned long fn = current->thread.fn;
	unsigned long fn_arg = current->thread.fn_arg;

	if (fn) {
		int ret = ((int (*)(void *))fn)((void *)fn_arg);

		if (!(current->flags & PF_KTHREAD))
			return 1;

		do_exit(ret);
	}
	return 1;	/* user fork — go to trap_exit */
}

void __cpuidle arch_cpu_idle(void)
{
	raw_local_irq_enable();
}

void show_regs(struct pt_regs *regs)
{
	pr_info("epc=%016lx sp=%016lx tp=%016lx r0=%016lx\n",
		regs->epc, regs->regs[13], regs->tp, regs->regs[0]);
}

void show_stack(struct task_struct *task, unsigned long *sp,
		const char *loglvl)
{
	pr_info("stack: task=%px sp=%px\n", task, sp);
}

/*
 * copy_thread — set up the kernel stack and thread_struct for a new task.
 *
 * For kernel threads (args->fn != NULL):
 *   pt_regs.regs[10] = fn   (loaded by ret_from_fork)
 *   pt_regs.regs[9]  = arg  (loaded by ret_from_fork)
 *   thread.r14        = ret_from_fork (so __switch_to "returns" there)
 *   thread.sp         = childregs (top of frame that ret_from_fork expects)
 *
 * For user forks (args->fn == NULL):
 *   pt_regs is a copy of the parent's, with SP/return value overridden.
 *   thread.r14 = ret_from_fork (goes through trap_exit path)
 */
int copy_thread(struct task_struct *p, const struct kernel_clone_args *args)
{
	struct pt_regs *childregs = task_pt_regs(p);

	memset(childregs, 0, sizeof(*childregs));

	if (args->fn) {
		/* Kernel thread — store fn/arg in thread_struct so that
		 * __switch_to can pass them in registers to ret_from_fork
		 * without relying on stack-based pt_regs surviving across
		 * schedule_tail().  */
		p->thread.fn     = (unsigned long)args->fn;
		p->thread.fn_arg = (unsigned long)args->fn_arg;
	} else {
		/* User fork — not yet supported, but wire the basics */
		/* Parent regs would be copied here */
		childregs->epc = 0;
		if (args->stack)
			childregs->regs[13] = args->stack;
		childregs->tp = args->tls;
		childregs->cpu_ctl = LITTLE64_CPU_CTL_IRQ_ENABLE |
					 LITTLE64_CPU_CTL_PAGING_ENABLE |
					 LITTLE64_CPU_CTL_USER_MODE;
	}

	p->thread.sp  = (unsigned long)childregs;
	p->thread.r11 = 0;
	p->thread.r14 = (unsigned long)ret_from_fork;
	p->thread.tp  = childregs->tp;

	return 0;
}

void flush_thread(void)
{
}

int elf_core_copy_task_fpregs(struct task_struct *t, elf_fpregset_t *fpu)
{
	pr_warn_once("little64: FP register core-dump copy is a bring-up stub\n");
	return 0;
}
