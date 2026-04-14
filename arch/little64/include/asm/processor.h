/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_PROCESSOR_H
#define __ASM_LITTLE64_PROCESSOR_H

#include <linux/const.h>
#include <linux/compiler.h>
#include <asm/page.h>

#define TASK_SIZE		(1UL << 38)
#define STACK_TOP		TASK_SIZE
#define STACK_TOP_MAX		TASK_SIZE

#define TASK_UNMAPPED_BASE	(TASK_SIZE / 3)

#define LITTLE64_THREAD_STRUCT_SP	0x00
#define LITTLE64_THREAD_STRUCT_R11	0x08
#define LITTLE64_THREAD_STRUCT_R14	0x10
#define LITTLE64_THREAD_STRUCT_TP	0x18
#define LITTLE64_THREAD_STRUCT_FN	0x20
#define LITTLE64_THREAD_STRUCT_FN_ARG	0x28
#define LITTLE64_THREAD_STRUCT_SIZE	0x30

#ifndef __ASSEMBLY__

#include <asm/ptrace.h>

struct thread_struct {
	unsigned long sp;	/* R13 — kernel stack pointer */
	unsigned long r11;	/* R11 — frame pointer (callee-saved) */
	unsigned long r14;	/* R14 — link register (callee-saved) */
	unsigned long tp;	/* user-bank TP special register */
	unsigned long fn;	/* kernel thread function (for ret_from_fork) */
	unsigned long fn_arg;	/* kernel thread argument (for ret_from_fork) */
};

struct task_struct;
extern unsigned long __get_wchan(struct task_struct *p);

#define INIT_THREAD { .sp = 0, .r11 = 0, .r14 = 0, .tp = 0, .fn = 0, .fn_arg = 0, }

typedef struct user_regs_struct {
	unsigned long gpr[16];
	unsigned long epc;
	unsigned long cpu_ctl;
	unsigned long tp;
} elf_gregset_t;

static inline void cpu_relax(void)
{
	barrier();
}

/* task_pt_regs: find the saved pt_regs at the top of the kernel stack */
#define task_pt_regs(task) \
	((struct pt_regs *)(task_stack_page(task) + THREAD_SIZE) - 1)

/* start_thread: set up a new user-mode thread context */
static inline void start_thread(struct pt_regs *regs,
				unsigned long pc, unsigned long sp)
{
	regs->epc = pc;
	regs->regs[13] = sp;  /* R13 = stack pointer */
	regs->tp = 0;
	regs->cpu_ctl = LITTLE64_CPU_CTL_IRQ_ENABLE |
			 LITTLE64_CPU_CTL_PAGING_ENABLE |
			 LITTLE64_CPU_CTL_USER_MODE;
}

/* Return the user-space stack pointer for core dumps / proc maps */
#define KSTK_ESP(task)	(task_pt_regs(task)->regs[13])
#define KSTK_EIP(task)	(task_pt_regs(task)->epc)

#endif /* __ASSEMBLY__ */
#endif /* __ASM_LITTLE64_PROCESSOR_H */
