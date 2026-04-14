// SPDX-License-Identifier: GPL-2.0-only

long long __moddi3(long long n, long long d);
extern long long __divdi3(long long n, long long d);

long long __moddi3(long long n, long long d)
{
	long long q;

	if (!d)
		return 0;
	q = __divdi3(n, d);
	return n - q * d;
}
