/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ASM_LITTLE64_ATOMIC_H
#define __ASM_LITTLE64_ATOMIC_H

#include <linux/types.h>
#include <asm/cmpxchg.h>
#include <asm/barrier.h>

/* 32-bit atomics */

static __always_inline int arch_atomic_read(const atomic_t *v)
{
	return READ_ONCE(v->counter);
}
static __always_inline void arch_atomic_set(atomic_t *v, int i)
{
	WRITE_ONCE(v->counter, i);
}

/*
 * Build arch_atomic_{add,sub,and,or,xor} and their _return/_fetch variants
 * from arch_cmpxchg. In UP the cmpxchg emulation disables IRQs, which is
 * sufficient for atomicity.
 */
#include <asm-generic/atomic.h>

#define arch_atomic_add		generic_atomic_add
#define arch_atomic_sub		generic_atomic_sub
#define arch_atomic_and		generic_atomic_and
#define arch_atomic_or		generic_atomic_or
#define arch_atomic_xor		generic_atomic_xor

#define arch_atomic_add_return		generic_atomic_add_return
#define arch_atomic_sub_return		generic_atomic_sub_return

#define arch_atomic_fetch_add		generic_atomic_fetch_add
#define arch_atomic_fetch_sub		generic_atomic_fetch_sub
#define arch_atomic_fetch_and		generic_atomic_fetch_and
#define arch_atomic_fetch_or		generic_atomic_fetch_or
#define arch_atomic_fetch_xor		generic_atomic_fetch_xor

static __always_inline int arch_atomic_xchg(atomic_t *v, int new)
{
	return (int)arch_xchg(&v->counter, new);
}
static __always_inline int arch_atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return (int)arch_cmpxchg(&v->counter, old, new);
}
#define arch_atomic_xchg	arch_atomic_xchg
#define arch_atomic_cmpxchg	arch_atomic_cmpxchg

/* 64-bit atomics — long is 64-bit on LP64 little64 */

#define ATOMIC64_INIT(i)	{ (i) }

static __always_inline s64 arch_atomic64_read(const atomic64_t *v)
{
	return READ_ONCE(v->counter);
}
static __always_inline void arch_atomic64_set(atomic64_t *v, s64 i)
{
	WRITE_ONCE(v->counter, i);
}
#define arch_atomic64_read	arch_atomic64_read
#define arch_atomic64_set	arch_atomic64_set

/* Implement 64-bit ops directly (same pattern as asm-generic/atomic.h UP) */
#define ATOMIC64_OP(op, c_op)						\
static inline void arch_atomic64_##op(s64 i, atomic64_t *v)		\
{									\
	unsigned long flags;						\
	raw_local_irq_save(flags);					\
	v->counter c_op##= i;						\
	raw_local_irq_restore(flags);					\
}

#define ATOMIC64_OP_RETURN(op, c_op)					\
static inline s64 arch_atomic64_##op##_return(s64 i, atomic64_t *v)	\
{									\
	unsigned long flags;						\
	s64 ret;							\
	raw_local_irq_save(flags);					\
	ret = (v->counter c_op##= i);					\
	raw_local_irq_restore(flags);					\
	return ret;							\
}

#define ATOMIC64_FETCH_OP(op, c_op)					\
static inline s64 arch_atomic64_fetch_##op(s64 i, atomic64_t *v)	\
{									\
	unsigned long flags;						\
	s64 ret;							\
	raw_local_irq_save(flags);					\
	ret = v->counter;						\
	v->counter c_op##= i;						\
	raw_local_irq_restore(flags);					\
	return ret;							\
}

ATOMIC64_OP(add, +)
ATOMIC64_OP(sub, -)
ATOMIC64_OP(and, &)
ATOMIC64_OP(or,  |)
ATOMIC64_OP(xor, ^)

ATOMIC64_OP_RETURN(add, +)
ATOMIC64_OP_RETURN(sub, -)

ATOMIC64_FETCH_OP(add, +)
ATOMIC64_FETCH_OP(sub, -)
ATOMIC64_FETCH_OP(and, &)
ATOMIC64_FETCH_OP(or,  |)
ATOMIC64_FETCH_OP(xor, ^)

#undef ATOMIC64_FETCH_OP
#undef ATOMIC64_OP_RETURN
#undef ATOMIC64_OP

#define arch_atomic64_add		arch_atomic64_add
#define arch_atomic64_sub		arch_atomic64_sub
#define arch_atomic64_and		arch_atomic64_and
#define arch_atomic64_or		arch_atomic64_or
#define arch_atomic64_xor		arch_atomic64_xor
#define arch_atomic64_add_return	arch_atomic64_add_return
#define arch_atomic64_sub_return	arch_atomic64_sub_return
#define arch_atomic64_fetch_add		arch_atomic64_fetch_add
#define arch_atomic64_fetch_sub		arch_atomic64_fetch_sub
#define arch_atomic64_fetch_and		arch_atomic64_fetch_and
#define arch_atomic64_fetch_or		arch_atomic64_fetch_or
#define arch_atomic64_fetch_xor		arch_atomic64_fetch_xor

static __always_inline s64 arch_atomic64_xchg(atomic64_t *v, s64 new)
{
	return (s64)arch_xchg(&v->counter, (unsigned long)new);
}
static __always_inline s64 arch_atomic64_cmpxchg(atomic64_t *v, s64 old, s64 new)
{
	return (s64)arch_cmpxchg(&v->counter, (unsigned long)old, (unsigned long)new);
}
#define arch_atomic64_xchg	arch_atomic64_xchg
#define arch_atomic64_cmpxchg	arch_atomic64_cmpxchg

#endif /* __ASM_LITTLE64_ATOMIC_H */
