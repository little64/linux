/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __ASM_LITTLE64_SIGCONTEXT_H
#define __ASM_LITTLE64_SIGCONTEXT_H

#include <asm/processor.h>

/* Signal context for Little-64 */
struct sigcontext {
	struct user_regs_struct sc_regs;
};

#endif /* __ASM_LITTLE64_SIGCONTEXT_H */
