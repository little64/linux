// SPDX-License-Identifier: GPL-2.0-only

/*
 * Minimal 64-bit multiply helper used when the backend lowers to __muldi3.
 * Implemented with shift-add so it does not depend on wider runtime helpers.
 */
long long __muldi3(long long a, long long b);

long long __muldi3(long long a, long long b)
{
	unsigned long long ua;
	unsigned long long ub;
	unsigned long long res = 0;
	int negate = 0;

	if (a < 0) {
		ua = (unsigned long long)(-a);
		negate ^= 1;
	} else {
		ua = (unsigned long long)a;
	}

	if (b < 0) {
		ub = (unsigned long long)(-b);
		negate ^= 1;
	} else {
		ub = (unsigned long long)b;
	}

	while (ub) {
		if (ub & 1ULL)
			res += ua;
		ua <<= 1;
		ub >>= 1;
	}

	return negate ? -(long long)res : (long long)res;
}
