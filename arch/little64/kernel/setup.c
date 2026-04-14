/* SPDX-License-Identifier: GPL-2.0-only */
/* arch/little64/kernel/setup.c - Architecture-specific setup */

#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/libfdt.h>
#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/panic.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <asm/page.h>
#include <asm/sections.h>

extern char _start[];

void machine_restart(char *cmd);
void machine_halt(void);
void machine_power_off(void);

/* Populated by head.S before entering start_kernel(). */
u64 little64_boot_fdt_pa;
phys_addr_t little64_phys_ram_base;

static bool __init little64_early_scan_fdt(void)
{
	void *dt_virt = NULL;
	phys_addr_t dt_phys = 0;

	if (little64_boot_fdt_pa) {
		dt_phys = (phys_addr_t)little64_boot_fdt_pa;
		dt_virt = phys_to_virt(dt_phys);
		if (!early_init_dt_scan(dt_virt, dt_phys)) {
			pr_warn("Ignoring invalid bootloader DTB at PA 0x%llx\n",
				(unsigned long long)dt_phys);
			dt_virt = NULL;
			dt_phys = 0;
		}
	}

#ifdef CONFIG_BUILTIN_DTB
	if (!dt_virt && !fdt_check_header(__dtb_start)) {
		dt_virt = __dtb_start;
		dt_phys = __pa(__dtb_start);
		if (!early_init_dt_scan(dt_virt, dt_phys)) {
			pr_warn("Ignoring invalid built-in DTB at PA 0x%llx\n",
				(unsigned long long)dt_phys);
			dt_virt = NULL;
			dt_phys = 0;
		}
	}
#endif

	return dt_virt != NULL;
}

static void __init little64_setup_memory(void)
{
	const phys_addr_t dram_start = memblock_start_of_DRAM();
	const phys_addr_t dram_end = memblock_end_of_DRAM();

	if (!dram_end || dram_end <= dram_start)
		panic("little64: no usable RAM described by DT\n");

	little64_phys_ram_base = dram_start;
	min_low_pfn = PFN_UP(dram_start);
	max_low_pfn = PFN_DOWN(dram_end);
	max_pfn = max_low_pfn;
}

static void __init little64_reserve_early_memory(void)
{
	const phys_addr_t early_pt_phys = PAGE_ALIGN(__pa_symbol(_end));
	const size_t early_pt_bytes = 3 * PAGE_SIZE;

	init_mm.pgd = (pgd_t *)__va(early_pt_phys);

	memblock_reserve(__pa_symbol(_start), _end - _start);
	memblock_reserve(early_pt_phys, early_pt_bytes);
	early_init_fdt_reserve_self();
	early_init_fdt_scan_reserved_mem();
	reserve_initrd_mem();
	memblock_set_current_limit(PFN_PHYS(max_low_pfn));
}

void __init setup_arch(char **cmdline_p)
{
	if (!little64_early_scan_fdt())
		pr_warn("No valid early DTB found, falling back to empty root DT\n");

	little64_setup_memory();
	setup_initial_init_mm(_start, _etext, _edata, _end);
	little64_reserve_early_memory();

	/* Parse selected early device tree into the runtime tree. */
	unflatten_and_copy_device_tree();

	/*
	 * boot_command_line is filled by early_init_dt_scan_chosen() during
	 * early_init_devtree(). Just expose it to the generic code.
	 */
	*cmdline_p = boot_command_line;
}

static int __init customize_machine(void)
{
	of_platform_default_populate(NULL, NULL, NULL);
	return 0;
}
arch_initcall(customize_machine);

void machine_restart(char *cmd)
{
	local_irq_disable();
	while (1)
		;
}

void machine_halt(void)
{
	local_irq_disable();
	while (1)
		;
}

void machine_power_off(void)
{
	local_irq_disable();
	while (1)
		;
}
