/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * arch/little64/lib/memset.S — arch-specific memset for Little-64.
 *
 * Hand-written byte loop that is smaller and tighter than what the
 * LLVM Little-64 backend currently emits for the generic C memset.
 * Since memset is hot (SLUB zeroing, stack init, etc.), the reduced
 * instruction count matters even though the backend's codegen is
 * functionally correct.
 *
 * Calling convention (Little-64 ABI):
 *   R10 = dest (void *s)
 *   R9  = value (int c)
 *   R8  = count (size_t)
 *   R1  = return value (= dest)
 */

#include <linux/linkage.h>

	.text
	.balign 4
	.global memset
	.type memset, @function
memset:
	MOVE	R10, R1			/* return value = dest */
	TEST	R0, R8
	JUMP.Z	@.Lmemset_done

	LDI	#1, R2			/* step = 1 */
	MOVE	R1, R3			/* R3 = current pointer */

.Lmemset_loop:
	BYTE_STORE	[R3], R9	/* *p = (unsigned char)c */
	ADD	R2, R3			/* p++ */
	SUB	R2, R8			/* count-- */
	TEST	R0, R8
	JUMP.Z	@.Lmemset_done
	JUMP	@.Lmemset_loop

.Lmemset_done:
	MOVE	R14, R15		/* return */
