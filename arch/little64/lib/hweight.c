// SPDX-License-Identifier: GPL-2.0-only

unsigned int __sw_hweight8(unsigned int w);

unsigned int __sw_hweight32(unsigned int w)
{
	unsigned int c = 0;

	while (w) {
		w &= (w - 1);
		c++;
	}
	return c;
}

unsigned int __sw_hweight8(unsigned int w)
{
	return __sw_hweight32(w & 0xffU);
}

unsigned long __sw_hweight64(unsigned long w)
{
	unsigned long c = 0;

	while (w) {
		w &= (w - 1);
		c++;
	}
	return c;
}
