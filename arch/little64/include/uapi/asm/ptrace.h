/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __ASM_LITTLE64_PTRACE_H
#define __ASM_LITTLE64_PTRACE_H

/* Register layout for ptrace - visible to userspace */
struct pt_regs {
	unsigned long gpr[16];
	unsigned long epc;
	unsigned long cpu_ctl;
	unsigned long tp;
};

#define PTRACE_GET_THREAD_AREA 25

#endif /* __ASM_LITTLE64_PTRACE_H */
