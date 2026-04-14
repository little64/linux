/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm-generic/vmlinux.lds.h>

#define L64_KERNEL_PHYS_BASE 0x100000
#define L64_PAGE_OFFSET 0xFFFFFFC000000000
#define L64_LOAD_OFFSET (L64_PAGE_OFFSET - L64_KERNEL_PHYS_BASE)
#define LOAD_OFFSET L64_LOAD_OFFSET
#define PAGE_SIZE 4096
#define L64_THREAD_SIZE 16384
#define THREAD_SIZE L64_THREAD_SIZE

ENTRY(_start)

jiffies = jiffies_64;

SECTIONS
{
	/*
	 * All sections are linked at virtual addresses in the kernel VA space.
	 * The AT() directive specifies the physical load address for each section.
	 * head.text is at VMA=PAGE_OFFSET, LMA=KERNEL_PHYS_BASE.
	 * This means e_entry resolves to 0xFFFFFFC000000000 (virtual), which the
	 * emulator maps back to physical 0x100000 via the offset difference.
	 */
	. = L64_PAGE_OFFSET;
	_start = .;

	.head.text : AT(ADDR(.head.text) - LOAD_OFFSET) {
		HEAD_TEXT
	}

	_stext = .;

	.text : AT(ADDR(.text) - LOAD_OFFSET) {
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		*(.fixup)
		*(.gnu.warning)
	}

	_etext = .;

	/*
	 * All init sections are contiguous so free_initmem() can reclaim the
	 * entire [__init_begin, __init_end) range as a single region.
	 */
	__init_begin = .;

	.init.text : AT(ADDR(.init.text) - LOAD_OFFSET) {
		_sinittext = .;
		INIT_TEXT
		_einittext = .;
	}

	.init.data : AT(ADDR(.init.data) - LOAD_OFFSET) {
		INIT_DATA
		INIT_SETUP(8)
		INIT_CALLS
		CON_INITCALL
		INIT_RAM_FS
	}

	. = ALIGN(L64_THREAD_SIZE);
	.data.init_task : AT(ADDR(.data.init_task) - LOAD_OFFSET) {
		INIT_TASK_DATA(L64_THREAD_SIZE)
	}

	PERCPU_SECTION(64)

	__init_end = .;

	/*
	 * Read-only data and read-write data follow init.  RO_DATA uses the
	 * LOAD_OFFSET defined above and places all rodata sub-sections
	 * (__ksymtab, SCHED_DATA, NOTES, RO_EXCEPTION_TABLE, etc.) properly.
	 */
	_sdata = .;

	RO_DATA(PAGE_SIZE)

	/*
	 * lld does not emit symbol assignments inside an empty output section.
	 * If __ex_table has no entries the BOUNDED_SECTION_BY symbols inside
	 * EXCEPTION_TABLE (called from RO_DATA) are never defined.  These
	 * PROVIDE stubs activate only in the empty-table case and present a
	 * valid zero-length range to the extable search routines.
	 */
	PROVIDE(__start___ex_table = .);
	PROVIDE(__stop___ex_table = .);

	.data : AT(ADDR(.data) - LOAD_OFFSET) {
		DATA_DATA
		*(.data.*)
	}

	_edata = .;

	.bss : AT(ADDR(.bss) - LOAD_OFFSET) {
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		__bss_stop = .;
	}

	_end = .;

	STABS_DEBUG
	DWARF_DEBUG
	MODINFO
	ELF_DETAILS

	DISCARDS
}
