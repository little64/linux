/* SPDX-License-Identifier: GPL-2.0-only */
/* arch/little64/kernel/asm-offsets.c - Generate asm offsets for assembly code */

#define COMPILE_OFFSETS

#include <linux/kbuild.h>
#include <linux/thread_info.h>

int main(void)
{
	/* Thread info offsets — used by entry.S in Phase 2 */
	DEFINE(THREAD_INFO_FLAGS,	offsetof(struct thread_info, flags));
	DEFINE(THREAD_INFO_PREEMPT,	offsetof(struct thread_info, preempt_count));
	BLANK();
	return 0;
}
