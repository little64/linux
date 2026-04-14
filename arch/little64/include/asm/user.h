/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_USER_H
#define __ASM_LITTLE64_USER_H

#include <asm/processor.h>
#include <asm/page.h>

/*
 * elf_gregset_t (which is 'struct user_regs_struct') is defined in
 * asm/processor.h.  We reuse it here to avoid a redefinition.
 */

struct user {
	elf_gregset_t regs;
	unsigned long u_tsize;
	unsigned long u_dsize;
	unsigned long u_ssize;
	unsigned long start_code;
	unsigned long start_stack;
	long int signal;
	unsigned long u_ar0;
	unsigned long magic;
	char u_comm[32];
};

#define NBPG			PAGE_SIZE
#define UPAGES			1
#define HOST_TEXT_START_ADDR	(u.start_code)
#define HOST_DATA_START_ADDR	(u.start_data)
#define HOST_STACK_END_ADDR	(u.start_stack + u.u_ssize * NBPG)

#endif /* __ASM_LITTLE64_USER_H */
