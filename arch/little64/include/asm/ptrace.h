/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_PTRACE_H
#define __ASM_LITTLE64_PTRACE_H

#define LITTLE64_GPR_COUNT		16

#define LITTLE64_PT_REGS_R0		0x00
#define LITTLE64_PT_REGS_R1		0x08
#define LITTLE64_PT_REGS_R2		0x10
#define LITTLE64_PT_REGS_R3		0x18
#define LITTLE64_PT_REGS_R4		0x20
#define LITTLE64_PT_REGS_R5		0x28
#define LITTLE64_PT_REGS_R6		0x30
#define LITTLE64_PT_REGS_R7		0x38
#define LITTLE64_PT_REGS_R8		0x40
#define LITTLE64_PT_REGS_R9		0x48
#define LITTLE64_PT_REGS_R10		0x50
#define LITTLE64_PT_REGS_R11		0x58
#define LITTLE64_PT_REGS_R12		0x60
#define LITTLE64_PT_REGS_R13		0x68
#define LITTLE64_PT_REGS_R14		0x70
#define LITTLE64_PT_REGS_R15		0x78
#define LITTLE64_PT_REGS_EPC		0x80
#define LITTLE64_PT_REGS_CPU_CTL	0x88
#define LITTLE64_PT_REGS_TP		0x90
#define LITTLE64_PT_REGS_SIZE		0x98

#define PTRACE_GET_THREAD_AREA	25

#ifndef __ASSEMBLY__

struct pt_regs {
	unsigned long regs[LITTLE64_GPR_COUNT];  /* R0-R15 */
	unsigned long epc;       /* SR21: interrupt EPC */
	unsigned long cpu_ctl;   /* SR23: saved cpu_control */
	unsigned long tp;        /* user-bank TP special register */
};

#define LITTLE64_CPU_CTL_IRQ_ENABLE	(1UL << 0)
#define LITTLE64_CPU_CTL_IN_INTERRUPT	(1UL << 1)
#define LITTLE64_CPU_CTL_CURRENT_IRQ_MASK	(0x7FUL << 2)
#define LITTLE64_CPU_CTL_PAGING_ENABLE	(1UL << 16)
#define LITTLE64_CPU_CTL_USER_MODE	(1UL << 17)

#define user_mode(regs) (((regs)->cpu_ctl & LITTLE64_CPU_CTL_USER_MODE) != 0)

static __always_inline bool regs_irqs_disabled(struct pt_regs *regs)
{
	return ((regs->cpu_ctl & LITTLE64_CPU_CTL_IRQ_ENABLE) == 0);
}


static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
	return regs->regs[13];
}

static inline void user_stack_pointer_set(struct pt_regs *regs,
					  unsigned long val)
{
	regs->regs[13] = val;
}

static inline unsigned long user_thread_pointer(struct pt_regs *regs)
{
	return regs->tp;
}

static inline void user_thread_pointer_set(struct pt_regs *regs,
				   unsigned long val)
{
	regs->tp = val;
}

#define instruction_pointer(regs)  ((regs)->epc)
#define profile_pc(regs)           instruction_pointer(regs)

#endif /* __ASSEMBLY__ */
#endif /* __ASM_LITTLE64_PTRACE_H */
