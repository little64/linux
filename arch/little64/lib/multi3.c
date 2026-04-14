// SPDX-License-Identifier: GPL-2.0-only

/*
 * 128-bit multiply helper (__multi3).
 *
 * The LLVM Little64 backend emits calls to __multi3 for 128-bit
 * multiplications (e.g. when computing the high half of a 64×64 product
 * used by the generic div64 helpers or 128-bit arithmetic).
 *
 * Signature:  __int128 __multi3(__int128 a, __int128 b)
 *
 * On Little64 the calling convention passes/returns 128-bit values as
 * a pair of 64-bit registers (lo, hi).  The implementation uses only
 * 64-bit multiplies (__muldi3) to stay independent of wider helpers.
 */

typedef          long long di_int;
typedef unsigned long long du_int;

typedef union {
	__int128 all;
	struct {
		du_int lo;
		di_int hi;
	} s;
} twords;

extern di_int __muldi3(di_int a, di_int b);

__int128 __multi3(__int128 a, __int128 b)
{
	twords x, y, r;

	x.all = a;
	y.all = b;

	r.s.lo = (du_int)__muldi3((di_int)x.s.lo, (di_int)y.s.lo);
	r.s.hi = __muldi3((di_int)x.s.hi, (di_int)y.s.lo) +
		 __muldi3((di_int)x.s.lo, (di_int)y.s.hi);

	/* Add the upper 64 bits of (x.lo * y.lo). */
	{
		du_int a0 = x.s.lo;
		du_int b0 = y.s.lo;
		du_int a0_lo = a0 & 0xFFFFFFFFULL;
		du_int a0_hi = a0 >> 32;
		du_int b0_lo = b0 & 0xFFFFFFFFULL;
		du_int b0_hi = b0 >> 32;

		du_int cross1 = (du_int)__muldi3((di_int)a0_hi, (di_int)b0_lo);
		du_int cross2 = (du_int)__muldi3((di_int)a0_lo, (di_int)b0_hi);
		du_int lo_lo  = (du_int)__muldi3((di_int)a0_lo, (di_int)b0_lo);
		du_int hi_hi  = (du_int)__muldi3((di_int)a0_hi, (di_int)b0_hi);

		du_int mid    = cross1 + (lo_lo >> 32);
		du_int carry  = (du_int)((mid & 0xFFFFFFFFULL) + (cross2 & 0xFFFFFFFFULL));

		r.s.hi += (di_int)(hi_hi + (mid >> 32) + (cross2 >> 32) + (carry >> 32));
	}

	return r.all;
}
