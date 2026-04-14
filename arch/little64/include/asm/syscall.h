/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_SYSCALL_H
#define __ASM_LITTLE64_SYSCALL_H

#include <linux/audit.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <asm/elf.h>
#include <asm/ptrace.h>

/* Audit arch ID for Little-64: 64-bit little-endian custom ISA. */
#ifndef AUDIT_ARCH_LITTLE64
#define AUDIT_ARCH_LITTLE64  (EM_LITTLE64 | __AUDIT_ARCH_64BIT | __AUDIT_ARCH_LE)
#endif

#define NO_SYSCALL (-1)

typedef long (*syscall_fn_t)(unsigned long, unsigned long, unsigned long,
			     unsigned long, unsigned long, unsigned long);

extern void * const sys_call_table[];

static inline int syscall_get_nr(struct task_struct *task,
				  struct pt_regs *regs)
{
	return (int)regs->regs[4];  /* R4 = syscall number */
}

static inline void syscall_set_return_value(struct task_struct *task,
				     struct pt_regs *regs,
				     int error, long val)
{
	regs->regs[1] = (long)error ?: val;
}

static inline void syscall_set_nr(struct task_struct *task,
				   struct pt_regs *regs, int nr)
{
	regs->regs[4] = (unsigned long)nr;
	if (nr == NO_SYSCALL)
		syscall_set_return_value(task, regs, -ENOSYS, 0);
}

static inline void syscall_rollback(struct task_struct *task,
				    struct pt_regs *regs)
{
	/*
	 * Little64 keeps the syscall number and all six arguments outside the
	 * return-value register, so rollback only needs to leave the input bank
	 * intact. The pre-syscall R1 scratch value is not preserved in pt_regs.
	 */
}

static inline long syscall_get_error(struct task_struct *task,
				     struct pt_regs *regs)
{
	unsigned long error = regs->regs[1];

	return IS_ERR_VALUE(error) ? error : 0;
}

static inline long syscall_get_return_value(struct task_struct *task,
					     struct pt_regs *regs)
{
	return regs->regs[1];  /* R1 = return value */
}

static inline void syscall_get_arguments(struct task_struct *task,
					  struct pt_regs *regs,
					  unsigned long *args)
{
	/* R10, R9, R8, R7, R6, R5 = syscall arguments 0..5 */
	args[0] = regs->regs[10];
	args[1] = regs->regs[9];
	args[2] = regs->regs[8];
	args[3] = regs->regs[7];
	args[4] = regs->regs[6];
	args[5] = regs->regs[5];
}

static inline void syscall_set_arguments(struct task_struct *task,
					  struct pt_regs *regs,
					  const unsigned long *args)
{
	regs->regs[10] = args[0];
	regs->regs[9] = args[1];
	regs->regs[8] = args[2];
	regs->regs[7] = args[3];
	regs->regs[6] = args[4];
	regs->regs[5] = args[5];
}

static inline int syscall_get_arch(struct task_struct *task)
{
	return AUDIT_ARCH_LITTLE64;
}

static inline bool arch_syscall_is_vdso_sigreturn(struct pt_regs *regs)
{
	return false;
}

#endif /* __ASM_LITTLE64_SYSCALL_H */
