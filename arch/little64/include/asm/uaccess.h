/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_UACCESS_H
#define __ASM_LITTLE64_UACCESS_H

#include <asm/page.h>
#include <asm/extable.h>
#include <asm-generic/access_ok.h>

/* Phase 1: minimal stubs — no real user/kernel memory distinction yet */

static inline unsigned long raw_copy_to_user(void __user *to,
					     const void *from, unsigned long n)
{
	memcpy((void __force *)to, from, n);
	return 0;
}

static inline unsigned long raw_copy_from_user(void *to,
					       const void __user *from,
					       unsigned long n)
{
	memcpy(to, (const void __force *)from, n);
	return 0;
}

#define INLINE_COPY_TO_USER
#define INLINE_COPY_FROM_USER

#include <asm-generic/uaccess.h>

#endif /* __ASM_LITTLE64_UACCESS_H */
