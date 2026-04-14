/* SPDX-License-Identifier: GPL-2.0-only */
/* arch/little64/kernel/head.S - Little-64 boot entry point */

#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/page.h>
#include <asm/special_registers.h>
#include <asm/pgtable.h>

/*
 * Little-64 instruction quick reference (all instructions are 2 bytes):
 *   LDI #imm, Rd       - load immediate (signed)
 *   MOVE Rs, Rd        - register copy
 *   ADD Rs, Rd         - Rd += Rs
 *   SUB Rs, Rd         - Rd -= Rs
 *   AND Rs, Rd         - Rd &= Rs
 *   OR  Rs, Rd         - Rd |= Rs
 *   SLLI #n, Rd        - Rd <<= n
 *   SRL  Rs, Rd        - Rd >>= Rs (logical, zero-filled)
 *   STORE [addr], Rs   - 64-bit store
 *   LOAD  [addr], Rd   - 64-bit load
 *   LOAD  @offset, Rd  - PC-relative 64-bit load (offset in 2-byte units)
 *   JUMP  @label       - unconditional jump
 *   JUMP.Z @label      - jump if zero flag
 *   JUMP.C @label      - jump if carry (unsigned >=, "no borrow")
 *   JUMP.GT @label     - jump if greater than (signed)
 *   JUMP.LT @label     - jump if less than (signed)
 *   SSR Rs1, Rd        - SR[Rs1] = Rd
 *   LSR Rs1, Rd        - Rd = SR[Rs1]
 *
 * Large 64-bit constants: JUMP @+4; .quad val; LOAD @-5, Rd
 * (JUMP skips 8 bytes = 4 instructions; LOAD reads 10 bytes back from
 *  after the LOAD instruction)
 *
 * Boot contract from emulator (direct mode, paging OFF):
 *   R1  = physical address of DTB
 *   R13 = physical RAM top (stack pointer, already aligned)
 *   R15 = physical address of _start
 */

__HEAD
SYM_CODE_START(_start)
	/* Save DTB physical address before we clobber registers */
	MOVE	R1, R10

	/* Debug breadcrumbs: UART THR @ 0x08000000 */
	JUMP	@+4
	.quad	0x08000000
	LOAD	@-5, R12
	LDI	#'0', R2
	BYTE_STORE	[R12], R2
	LDI	#10, R2
	BYTE_STORE	[R12], R2

	/* Align stack to 16 bytes */
	LDI	#-16, R2
	AND	R2, R13

	/*
	 * Get physical address of BSS start/end.
	 * __bss_start/__bss_stop are linked at virtual PAGE_OFFSET + offset.
	 * Physical = virtual - PAGE_OFFSET + KERNEL_PHYS_BASE
	 * PAGE_OFFSET = 0xFFFFFFC000000000, KERNEL_PHYS_BASE = 0x100000
	 * So: phys = virtual - 0xFFFFFFC000000000 + 0x100000
	 *          = virtual + 0x100000 - 0xFFFFFFC000000000
	 *          = virtual + (0xFFFFFFC000100000 + 0x400000000)... no
	 * Simpler: virtual - PAGE_OFFSET gives offset from kernel base,
	 * then add KERNEL_PHYS_BASE.
	 *
	 * Since PAGE_OFFSET - KERNEL_PHYS_BASE = 0xFFFFFFC000000000 - 0x100000
	 * = 0xFFFFFFBFFFF00000, use: phys = virtual - (PAGE_OFFSET - KERNEL_PHYS_BASE)
	 *                              phys = virtual + (KERNEL_PHYS_BASE - PAGE_OFFSET)
	 *
	 * We can compute this as:  phys = virtual + DELTA
	 * where DELTA = KERNEL_PHYS_BASE - PAGE_OFFSET = 0x100000 - 0xFFFFFFC000000000
	 *             = 0x4000000100000 (unsigned wrap) -- complicated.
	 *
	 * Alternative: assemble __bss_start physical directly using .org trick.
	 * Actually the assembler evaluates __bss_start - PAGE_OFFSET + KERNEL_PHYS_BASE
	 * as a link-time constant. This is fine.
	 */

	/* R4 = physical address of __bss_start */
	JUMP	@+4
	.quad	__bss_start - PAGE_OFFSET + 0x100000
	LOAD	@-5, R4

	/* R5 = physical address of __bss_stop */
	JUMP	@+4
	.quad	__bss_stop - PAGE_OFFSET + 0x100000
	LOAD	@-5, R5

	/* Clear BSS loop: zero from R4 to R5 */
clear_bss:
	MOVE	R4, R6
	SUB	R5, R6		/* R6 = R4 - R5 */
	JUMP.C	@clear_bss_store	/* emulator C flag for SUB is set on borrow (R4 < R5) */
	JUMP	@bss_done
clear_bss_store:
	STORE	[R4], R0	/* *R4 = 0 */
	LDI	#8, R2
	ADD	R2, R4
	JUMP	@clear_bss
bss_done:
	LDI	#'B', R2
	BYTE_STORE	[R12], R2
	LDI	#10, R2
	BYTE_STORE	[R12], R2

	/*
	 * Build initial 3-level page tables.
	 * Allocate two 4KB pages after BSS (4KB-aligned):
	 *   R9 = L2 table (root)
	 *   R8 = L1 table (kernel mapping)
	 *   R11 = L1b table (identity map for transition)
	 */

	/* R9 = first 4KB-aligned page after __bss_stop physical */
	JUMP	@+4
	.quad	__bss_stop - PAGE_OFFSET + 0x100000
	LOAD	@-5, R9
	JUMP	@+4
	.quad	4095
	LOAD	@-5, R2
	ADD	R2, R9
	JUMP	@+4
	.quad	-4096
	LOAD	@-5, R2
	AND	R2, R9

	/* R8 = second page (R9 + 4KB) */
	MOVE	R9, R8
	JUMP	@+4
	.quad	4096
	LOAD	@-5, R2
	ADD	R2, R8

	/* R11 = third page (R8 + 4KB) — identity map L1 */
	MOVE	R8, R11
	JUMP	@+4
	.quad	4096
	LOAD	@-5, R2
	ADD	R2, R11

	/* R7 = end of page table region (R11 + 4KB) */
	MOVE	R11, R7
	JUMP	@+4
	.quad	4096
	LOAD	@-5, R2
	ADD	R2, R7

	/* Zero all three pages (3 * 4096 = 12288 bytes, 8 bytes at a time) */
	MOVE	R9, R4
zero_pt:
	MOVE	R4, R6
	SUB	R7, R6
	JUMP.C	@zero_pt_store
	JUMP	@pt_zeroed
zero_pt_store:
	STORE	[R4], R0
	LDI	#8, R2
	ADD	R2, R4
	JUMP	@zero_pt
pt_zeroed:
	LDI	#'P', R2
	BYTE_STORE	[R12], R2
	LDI	#10, R2
	BYTE_STORE	[R12], R2

	/*
	 * L2[0x100] -> 1GiB leaf for kernel virtual window.
	 * VA 0xFFFFFFC000000000 -> L2 index 0x100.
	 * Physical base is 0x100000 so VA base maps to loaded kernel base.
	 */
	JUMP	@+4
	.quad	0x100
	LOAD	@-5, R4	/* PPN = 256 (for PA 0x100000) */
	SLLI	#10, R4		/* R4 = PPN << 10 = 0x40000 */
	LDI	#0x2F, R2	/* V|R|W|X|G */
	OR	R2, R4		/* R4 = 0x4002F */
	MOVE	R9, R3
	JUMP	@+4
	.quad	2048
	LOAD	@-5, R2
	ADD	R2, R3		/* R3 = &L2[0x100] */
	STORE	[R3], R4

	/*
	 * L2[0] -> 1GiB superpage identity map at PA=0.
	 * This keeps low physical execution alive across the paging-enable transition.
	 */
	LDI	#0x2F, R2	/* V|R|W|X|G, PPN=0 */
	MOVE	R2, R4
	STORE	[R9], R4	/* L2[0] leaf */

	/*
	 * Enable paging:
	 *   SR11 = L2 table physical address (page_table_root_physical)
	 *   SR0 bit 16 = PagingEnable
	 */
	LDI	#LITTLE64_SR_PAGE_TABLE_ROOT_PHYSICAL, R2
	SSR	R2, R9		/* SR11 = R9 (L2 root) */

	LDI	#0, R2
	LSR	R2, R3		/* R3 = SR0 (cpu_control) */

	JUMP	@+4
	.quad	0x10000
	LOAD	@-5, R2
	OR	R2, R3		/* R3 |= PagingEnable (bit 16) */

	LDI	#0, R2
	SSR	R2, R3		/* SR0 = R3 (paging now on) */

	/*
	 * Jump to virtual address.
	 * The identity map at L2[0] keeps instruction fetch alive.
	 * We load the absolute virtual address from a constant pool.
	 */
	JUMP	@+4
	.quad	_start_virtual
	LOAD	@-5, R3
	MOVE	R3, R15		/* R15 = PC: indirect jump */

SYM_CODE_END(_start)

	/*
	 * _start_virtual: post-paging entry point.
	 * Now running at virtual addresses. Identity map still active.
	 */
_start_virtual:
	/* Set up virtual kernel stack: init_thread_union + THREAD_SIZE */
	JUMP	@+4
	.quad	init_thread_union
	LOAD	@-5, R13
	JUMP	@+4
	.quad	THREAD_SIZE
	LOAD	@-5, R2
	ADD	R2, R13

	/* Restore DTB physical address for setup_arch() */
	MOVE	R10, R1

	/* Persist DTB physical address for setup_arch() early FDT scan. */
	JUMP	@+4
	.quad	little64_boot_fdt_pa
	LOAD	@-5, R2
	STORE	[R2], R10

	/* Jump to start_kernel (does not return). */
	/* Use a literal-pool hop to avoid PCREL13 branch-range limits. */
	JUMP	@+4
	.quad	start_kernel
	LOAD	@-5, R3
	MOVE	R3, R15
