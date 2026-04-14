// SPDX-License-Identifier: GPL-2.0-only

#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <asm-generic/syscalls.h>

#define sys_mmap sys_mmap_pgoff

asmlinkage long little64_rt_sigreturn(void);

#define sys_rt_sigreturn little64_rt_sigreturn

#define __ARCH_WANT_NEW_STAT
#define __ARCH_WANT_SET_GET_RLIMIT
#define __ARCH_WANT_SYS_CLONE
#define __ARCH_WANT_MEMFD_SECRET

#include <asm/unistd.h>

#undef __SYSCALL
#define __SYSCALL(nr, call)	[nr] = (void *)(call),

void * const sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls - 1] = (void *)sys_ni_syscall,
#include <uapi/asm-generic/unistd.h>
};