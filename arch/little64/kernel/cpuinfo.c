// SPDX-License-Identifier: GPL-2.0-only
#include <linux/seq_file.h>

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return (*pos == 0) ? (void *)1 : 0;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return 0;
}

static void c_stop(struct seq_file *m, void *v)
{
}

static int c_show(struct seq_file *m, void *v)
{
	seq_puts(m, "processor\t: 0\n");
	seq_puts(m, "model name\t: Little64\n");
	return 0;
}

const struct seq_operations cpuinfo_op = {
	.start = c_start,
	.next = c_next,
	.stop = c_stop,
	.show = c_show,
};
