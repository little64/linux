// SPDX-License-Identifier: GPL-2.0-only

unsigned long long __udivdi3(unsigned long long n, unsigned long long d);
unsigned long long __umoddi3(unsigned long long n, unsigned long long d);
long long __divdi3(long long n, long long d);

static unsigned long long __udivmoddi4(unsigned long long n,
				       unsigned long long d,
				       unsigned long long *r)
{
	unsigned long long q = 0;
	unsigned long long rem = 0;
	int i;

	if (!d) {
		if (r)
			*r = 0;
		return 0;
	}

	for (i = 63; i >= 0; --i) {
		rem = (rem << 1) | ((n >> i) & 1ULL);
		if (rem >= d) {
			rem -= d;
			q |= (1ULL << i);
		}
	}

	if (r)
		*r = rem;
	return q;
}

unsigned long long __udivdi3(unsigned long long n, unsigned long long d)
{
	return __udivmoddi4(n, d, 0);
}

unsigned long long __umoddi3(unsigned long long n, unsigned long long d)
{
	unsigned long long r;

	(void)__udivmoddi4(n, d, &r);
	return r;
}

long long __divdi3(long long n, long long d)
{
	unsigned long long un;
	unsigned long long ud;
	unsigned long long q;
	int neg = 0;

	if (n < 0) {
		un = (unsigned long long)(-n);
		neg ^= 1;
	} else {
		un = (unsigned long long)n;
	}

	if (d < 0) {
		ud = (unsigned long long)(-d);
		neg ^= 1;
	} else {
		ud = (unsigned long long)d;
	}

	q = __udivdi3(un, ud);
	return neg ? -(long long)q : (long long)q;
}
