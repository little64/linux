/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * arch/little64/kernel/entry.S — Trap entry/exit, context switch,
 *                                  and new-task entry for Little-64.
 *
 * CPU interrupt-entry contract (hardware actions, all GPRs untouched):
 *   SR21 (interrupt_epc)         = saved PC
 *   SR22 (interrupt_eflags)      = saved flags
 *   SR23 (interrupt_cpu_control) = saved cpu_control
 *   SR0  (cpu_control): forced supervisor, IRQs off, in-interrupt,
 *         bits [8:2] = current interrupt/trap number (7-bit, 0-127)
 *   PC   = handler from table:  MEM64[SR16 + irq_num * 8]
 */

#include <linux/linkage.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/special_registers.h>

#define LITTLE64_STACK_SLOT_SIZE		8
#define LITTLE64_TRAP_TEMP_SAVE_R3_OFFSET	8
#define LITTLE64_TRAP_TEMP_SAVE_R2_OFFSET	16
#define LITTLE64_TRAP_TEMP_SAVE_R1_OFFSET	24
#define LITTLE64_TRAP_ENTRY_ORIG_SP_DELTA	64
#define LITTLE64_CALL_LITERAL_RETURN_DELTA	6
#define LITTLE64_TRAP_EXIT_R15_TO_R2_DELTA	0x68
#define LITTLE64_TRAP_EXIT_R15_TO_R1_DELTA	0x70

/*
 * pt_regs layout (19 x 8 = 152 bytes, low to high address):
 *   +0x00  regs[0]  (R0, always 0)      +0x08  regs[1]
 *   +0x10  regs[2]                       +0x18  regs[3]
 *   +0x20  regs[4]                       +0x28  regs[5]
 *   +0x30  regs[6]                       +0x38  regs[7]
 *   +0x40  regs[8]                       +0x48  regs[9]
 *   +0x50  regs[10]                      +0x58  regs[11]
 *   +0x60  regs[12]                      +0x68  regs[13] (SP)
 *   +0x70  regs[14]                      +0x78  regs[15]
 *   +0x80  epc   (LITTLE64_PT_REGS_EPC)     +0x88  cpu_ctl  (LITTLE64_PT_REGS_CPU_CTL)
 *   +0x90  tp    (LITTLE64_PT_REGS_TP)
 */

/* --------------------------------------------------------------- */
/*  Vector table                                                    */
/*  128 entries, all pointing to little64_trap_entry.               */
/*  CPU reads handler = MEM64[interrupt_table_base + irq * 8].      */
/* --------------------------------------------------------------- */
	.section .data
	.balign 8
	.global little64_trap_table
little64_trap_table:
	.rept 128
	.quad	little64_trap_entry
	.endr

/* Per-CPU data (uniprocessor) */
	.balign 8
	.global little64_thread_offset
little64_thread_offset:
	.quad	0

/* --------------------------------------------------------------- */
/*  TRAP ENTRY                                                      */
/* --------------------------------------------------------------- */
/*
 * On entry all 16 GPRs are intact from the interrupted context.
 * R13 is a valid kernel stack (Phase 1: kernel-only).
 *
 * We build a pt_regs frame (152 bytes) on the stack using PUSH.
 * Three scratch registers (R1, R2, R3) are temp-saved above the frame
 * via PUSH before we use them; originals are recovered before the
 * frame is finalised.
 */
	.text
	.balign 4
	.global little64_trap_entry
	.type little64_trap_entry, @function
little64_trap_entry:
	/* ---- temp-save R3, R2 and R1 above the frame ---- */
	PUSH	R3			/* [origSP -  8] = orig R3 */
	PUSH	R2			/* [origSP - 16] = orig R2 */
	PUSH	R1			/* [origSP - 24] = orig R1 */

	/* ---- read special registers while they are stable ---- */
	LDI64	#LITTLE64_SR_USER_THREAD_POINTER, R3
	LSR	R3, R3			/* R3 = user-bank TP selector */
	LDI	#LITTLE64_SR_INTERRUPT_CPU_CONTROL, R1
	LSR	R1, R1			/* R1 = SR23 (interrupt_cpu_control) */
	LDI	#LITTLE64_SR_INTERRUPT_EPC, R2
	LSR	R2, R2			/* R2 = SR21 (interrupt_epc)         */

	/* ---- push tp, cpu_ctl, epc, R15, R14 ---- */
	PUSH	R3			/* tp       → LITTLE64_PT_REGS_TP */
	PUSH	R1			/* cpu_ctl  → LITTLE64_PT_REGS_CPU_CTL */
	PUSH	R2			/* epc      → LITTLE64_PT_REGS_EPC */
	PUSH	R15			/* regs[15] → LITTLE64_PT_REGS_R15 */
	PUSH	R14			/* regs[14] → LITTLE64_PT_REGS_R14 */

	/* ---- push original R13 ----
	 * 8 PUSHes so far, each -8 ⇒ origSP = R13 + LITTLE64_TRAP_ENTRY_ORIG_SP_DELTA. */
	MOVE	R13, R1
	LDI	#LITTLE64_TRAP_ENTRY_ORIG_SP_DELTA, R2
	ADD	R2, R1			/* R1 = original SP */
	PUSH	R1			/* regs[13] → LITTLE64_PT_REGS_R13 */

	/* ---- push R12 .. R4 ---- */
	PUSH	R12
	PUSH	R11
	PUSH	R10
	PUSH	R9
	PUSH	R8
	PUSH	R7
	PUSH	R6
	PUSH	R5
	PUSH	R4

	/* ---- recover and push original R3 ----
	 * orig R3 is at [origSP - 8].  R1 still = origSP. */
	MOVE	R1, R2
	LDI	#LITTLE64_TRAP_TEMP_SAVE_R3_OFFSET, R3
	SUB	R3, R2			/* R2 = origSP - 8 */
	LOAD	[R2], R2		/* R2 = original R3 */
	PUSH	R2			/* regs[3] → LITTLE64_PT_REGS_R3 */

	/* ---- recover and push original R2 ----
	 * orig R2 is at [origSP - 16].  R1 still = origSP. */
	MOVE	R1, R2
	LDI	#LITTLE64_TRAP_TEMP_SAVE_R2_OFFSET, R3
	SUB	R3, R2			/* R2 = origSP - 16 */
	LOAD	[R2], R2		/* R2 = original R2 */
	PUSH	R2			/* regs[2] → LITTLE64_PT_REGS_R2 */

	/* ---- recover and push original R1 ----
	 * orig R1 is at [origSP - 24].  R1 still = origSP. */
	MOVE	R1, R2
	LDI	#LITTLE64_TRAP_TEMP_SAVE_R1_OFFSET, R3
	SUB	R3, R2			/* R2 = origSP - 24 */
	LOAD	[R2], R2		/* R2 = original R1 */
	PUSH	R2			/* regs[1] → LITTLE64_PT_REGS_R1 */

	/* ---- R0 is hardwired zero ---- */
	PUSH	R0			/* regs[0] → LITTLE64_PT_REGS_R0 */

	/* ===== pt_regs frame complete.  R13 = frame base.
	 *       LITTLE64_TRAP_TEMP_SAVE_R1_OFFSET-byte temp area sits above at [origSP-16 .. origSP-1],
	 *       reclaimed when we restore origSP on exit. ===== */

	/* ---- call do_trap(struct pt_regs *regs) ----
	 * arg0 in R10 per Little64 calling convention. */
	MOVE	R13, R10
	JUMP	@+4
	.quad	do_trap
	LOAD	@-5, R1
	MOVE	R15, R14
	LDI	#LITTLE64_CALL_LITERAL_RETURN_DELTA, R2
	ADD	R2, R14			/* LR = return address */
	MOVE	R1, R15			/* call do_trap */

	/* fall through to trap exit */

/* --------------------------------------------------------------- */
/*  TRAP EXIT                                                       */
/* --------------------------------------------------------------- */
/*
 * R13 = pt_regs base (C callee preserved it).
 *
 * Strategy: stash restored-R3 in SR28 and keep saved-R13 in R1 scratch so
 * we can temporarily retarget R13 back into the pt_regs frame to recover
 * the last caller-saved registers before restoring the original stack.
 *
 * Restore order:
	 *   1. TP, SR21, SR23 (using R1-R3 as scratch — they're restored later)
	 *   2. R3-R12 via SPOP
	 *   3. Stash restored R3 into SR28
	 *   4. Stash saved R13 slot into R1 scratch
	 *   5. Skip saved R13 slot, restore R14 via SPOP
	 *   6. Restore R2 from frame  (using R3 as address scratch)
	 *   7. Temporarily retarget R13 to frame + LITTLE64_PT_REGS_R1, restore R1 into R3 scratch
	 *   8. Restore R13 from R1 scratch
	 *   9. Restore R1 from R3 scratch
	 *   10. Restore R3 from SR28
	 *   11. IRET
 */
	.global little64_trap_exit
	.type little64_trap_exit, @function
little64_trap_exit:
	/*
	 * ret_from_fork's direct branch can arrive one instruction past the
	 * symbol entry on the current LLVM/emulator combination. Keep a
	 * harmless 2-byte landing pad here so both fallthrough and branch
	 * callers reach the real restore sequence safely.
	 */
	ADD	R0, R0
	/* ---- Phase 1: restore TP from frame + LITTLE64_PT_REGS_TP ---- */
	MOVE	R13, R1
	LDI	#LITTLE64_PT_REGS_TP, R2
	ADD	R2, R1			/* R1 = frame + LITTLE64_PT_REGS_TP */
	LOAD	[R1], R2		/* R2 = saved tp */
	LDI64	#LITTLE64_SR_USER_THREAD_POINTER, R3
	SSR	R3, R2			/* user TP = saved tp */

	/* ---- Phase 2: restore SR21 (epc) from frame + LITTLE64_PT_REGS_EPC ---- */
	MOVE	R13, R1
	LDI	#LITTLE64_PT_REGS_EPC, R2
	ADD	R2, R1			/* R1 = frame + LITTLE64_PT_REGS_EPC */
	LOAD	[R1], R2		/* R2 = saved epc */
	LDI	#LITTLE64_SR_INTERRUPT_EPC, R3
	SSR	R3, R2			/* SR21 = epc */

	/* ---- restore SR23 (cpu_ctl) from frame + LITTLE64_PT_REGS_CPU_CTL ---- */
	LDI	#LITTLE64_STACK_SLOT_SIZE, R3
	ADD	R3, R1			/* R1 = frame + LITTLE64_PT_REGS_CPU_CTL */
	LOAD	[R1], R2		/* R2 = saved cpu_ctl */
	LDI	#LITTLE64_SR_INTERRUPT_CPU_CONTROL, R3
	SSR	R3, R2			/* SR23 = cpu_ctl */

	/* ---- Phase 3: restore R3 .. R12 via POP ---- */
	MOVE	R13, R1
	LDI	#LITTLE64_PT_REGS_R3, R2
	ADD	R2, R1
	MOVE	R1, R13			/* SP → regs[3] at frame + LITTLE64_PT_REGS_R3 */
	POP	R3
	POP	R4
	POP	R5
	POP	R6
	POP	R7
	POP	R8
	POP	R9
	POP	R10
	POP	R11
	POP	R12
	/* SP is now at regs[13] (frame + LITTLE64_PT_REGS_R13) */

	/* ---- Phase 4: stash restored R3 in SR28 ---- */
	LDI	#LITTLE64_SR_TRAP_AUX, R1	/* SR28 index (R1 not yet restored) */
	SSR	R1, R3			/* SR28 = restored R3 */

	/* ---- Phase 5: stash saved R13 in R1 scratch ---- */
	LOAD	[R13], R1		/* R1 = saved R13; do not touch R15 (PC) */

	/* ---- Phase 6: skip saved R13, restore R14 ---- */
	LDI	#LITTLE64_STACK_SLOT_SIZE, R3
	ADD	R3, R13			/* skip regs[13] → SP at regs[14] */
	POP	R14
	/* SP now at regs[15] = frame + LITTLE64_PT_REGS_R15 */

	/* ---- Phase 7: restore original R2 (frame + LITTLE64_PT_REGS_R2) ---- */
	MOVE	R13, R3
	LDI	#LITTLE64_TRAP_EXIT_R15_TO_R2_DELTA, R2
	SUB	R2, R3			/* R3 = &regs[2] = frame + LITTLE64_PT_REGS_R2 */
	LOAD	[R3], R2		/* R2 = original R2 */

	/* ---- Phase 8: restore original R1 into R3 scratch ---- */
	LDI	#LITTLE64_TRAP_EXIT_R15_TO_R1_DELTA, R3
	SUB	R3, R13			/* R13 = &regs[1] = frame + LITTLE64_PT_REGS_R1 */
	LOAD	[R13], R3		/* R3 = original R1; original R3 is in SR28 */

	/* ---- Phase 9: restore R13 from R1 scratch ---- */
	MOVE	R1, R13			/* R13 = original SP */

	/* ---- Phase 10: restore R1 from R3 scratch ---- */
	MOVE	R3, R1			/* R1 = original R1 */

	/* ---- Phase 11: restore R3 from SR28 ---- */
	LDI	#LITTLE64_SR_TRAP_AUX, R3	/* SR28 index (R3 = 28 momentarily) */
	LSR	R3, R3			/* R3 = SR28 = original R3 */

	IRET


/* --------------------------------------------------------------- */
/*  ret_from_fork — first scheduling of a new task                  */
/* --------------------------------------------------------------- */
/*
 * Entered via __switch_to "returning" to this address (thread.r14).
 *
 * R1  = prev (return value of __switch_to)
 * R3  = fn   (from thread.fn, 0 for user fork)
 * R4  = arg  (from thread.fn_arg)
 * R13 = new task kernel stack pointing at pt_regs frame
 */
	.text
	.balign 4
	.global ret_from_fork
	.type ret_from_fork, @function
ret_from_fork:
	/* schedule_tail(prev) — prev in R1, move to arg0 (R10) */
	MOVE	R1, R10
	JUMP	@+4
	.quad	schedule_tail
	LOAD	@-5, R1
	MOVE	R15, R14
	LDI	#LITTLE64_CALL_LITERAL_RETURN_DELTA, R2
	ADD	R2, R14
	MOVE	R1, R15			/* call schedule_tail */

	/* Let C handle kernel-thread dispatch.  Assembly LOAD from
	 * thread.fn returns 0 for unknown reasons; C reads it fine.
	 * do_kernel_thread_entry() calls fn(arg) + do_exit() for
	 * kernel threads and returns 1 for user forks.              */
	JUMP	@+4
	.quad	do_kernel_thread_entry
	LOAD	@-5, R1
	MOVE	R15, R14
	LDI	#LITTLE64_CALL_LITERAL_RETURN_DELTA, R2
	ADD	R2, R14
	MOVE	R1, R15			/* call do_kernel_thread_entry */

	/* If we reach here, it was a user fork → trap_exit */
	JUMP	@little64_trap_exit


/* --------------------------------------------------------------- */
/*  __switch_to(prev, next)                                         */
/* --------------------------------------------------------------- */
/*
 * R10 = prev (arg0),  R9 = next (arg1)
 * Returns prev in R1.
 *
 * Saves / restores only callee-saved registers per LLVM ABI:
 *   R13 (SP), R11 (FP), R14 (LR)
 *
 * thread_struct:  { sp, r11, r14, tp, fn, fn_arg } at LITTLE64_THREAD_STRUCT_* offsets.
 * Offset of thread in task_struct is in little64_thread_offset (set by C).
 *
 * After restore, R3 = next->thread.fn, R4 = next->thread.fn_arg.
 * ret_from_fork uses these directly (avoids stack-based pt_regs issues).
 */
	.text
	.balign 4
	.global __switch_to
	.type __switch_to, @function
__switch_to:
	/* R1 = thread_struct offset in task_struct */
	JUMP	@+4
	.quad	little64_thread_offset
	LOAD	@-5, R1
	LOAD	[R1], R1

	/* R2 = &prev->thread */
	MOVE	R10, R2
	ADD	R1, R2

	/* Save prev callee-saved state */
	STORE	[R2], R13		/* prev->thread.sp  */
	MOVE	R2, R3
	LDI	#LITTLE64_THREAD_STRUCT_R11, R4
	ADD	R4, R3
	STORE	[R3], R11		/* prev->thread.r11 */
	MOVE	R2, R3
	LDI	#LITTLE64_THREAD_STRUCT_R14, R4
	ADD	R4, R3
	STORE	[R3], R14		/* prev->thread.r14 */
	MOVE	R2, R3
	LDI	#LITTLE64_THREAD_STRUCT_TP, R4
	ADD	R4, R3
	LDI64	#LITTLE64_SR_USER_THREAD_POINTER, R4
	LSR	R4, R4			/* R4 = current user TP */
	STORE	[R3], R4		/* prev->thread.tp */

	/* R2 = &next->thread */
	MOVE	R9, R2
	ADD	R1, R2

	/* Restore next callee-saved state */
	LOAD	[R2], R13		/* SP  ← next->thread.sp  */
	MOVE	R2, R3
	LDI	#LITTLE64_THREAD_STRUCT_R11, R4
	ADD	R4, R3
	LOAD	[R3], R11		/* FP  ← next->thread.r11 */
	MOVE	R2, R3
	LDI	#LITTLE64_THREAD_STRUCT_R14, R4
	ADD	R4, R3
	LOAD	[R3], R14		/* LR  ← next->thread.r14 */
	MOVE	R2, R3
	LDI	#LITTLE64_THREAD_STRUCT_TP, R4
	ADD	R4, R3
	LOAD	[R3], R4		/* user TP ← next->thread.tp */
	LDI64	#LITTLE64_SR_USER_THREAD_POINTER, R3
	SSR	R3, R4

	/* Update current task pointer */
	JUMP	@+4
	.quad	little64_current_task
	LOAD	@-5, R1
	STORE	[R1], R9		/* little64_current_task = next */

	/* Return prev */
	MOVE	R10, R1
	MOVE	R14, R15		/* return via restored LR */
