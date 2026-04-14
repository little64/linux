/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_ELF_H
#define __ASM_LITTLE64_ELF_H

#include <linux/types.h>
#include <linux/elf-em.h>
#include <asm/processor.h>

/* Little-64 ELF machine type */
#define EM_LITTLE64	0x4C36

/* elf_greg_t/elf_gregset_t come from processor.h (user_regs_struct) */
typedef unsigned long elf_greg_t;
#define ELF_NGREG	(sizeof(elf_gregset_t) / sizeof(elf_greg_t))

typedef unsigned long elf_fpregset_t;  /* no FPU */

#define elf_check_arch(x) ((x)->e_machine == EM_LITTLE64)

#define ELF_CLASS	ELFCLASS64
#define ELF_DATA	ELFDATA2LSB
#define ELF_ARCH	EM_LITTLE64

#define ELF_EXEC_PAGESIZE	PAGE_SIZE

#define ELF_PLATFORM		(NULL)
#define ELF_HWCAP		(0)

#define ELF_ET_DYN_BASE		(TASK_SIZE / 3 * 2)

#define SET_PERSONALITY(ex)		do { } while (0)

#endif /* __ASM_LITTLE64_ELF_H */
