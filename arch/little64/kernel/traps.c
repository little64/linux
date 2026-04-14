// SPDX-License-Identifier: GPL-2.0-only
/*
 * arch/little64/kernel/traps.c — Trap / exception / IRQ dispatch
 *
 * Provides:
 *   trap_init()           — installs the vector table and the thread offset
 *   do_trap(pt_regs *)    — master dispatch called from entry.S
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <linux/irq-entry-common.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/randomize_kstack.h>
#include <linux/sched/signal.h>
#include <linux/sched/task_stack.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <asm/siginfo.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/interrupt_vectors.h>
#include <asm/irqflags.h>
#include <asm/special_registers.h>
#include <asm/syscall.h>

/* Defined in entry.S */
extern char little64_trap_table[];
extern unsigned long little64_thread_offset;
asmlinkage void noinstr do_trap(struct pt_regs *regs);

/* ------------------------------------------------------------------ */
/*  SR accessors (inline asm)                                         */
/* ------------------------------------------------------------------ */

static inline unsigned long read_sr(unsigned long idx)
{
	unsigned long val;
	asm volatile("LSR %1, %0" : "=r"(val) : "r"(idx));
	return val;
}

static inline void write_sr(unsigned long idx, unsigned long val)
{
	asm volatile("SSR %0, %1" : : "r"(idx), "r"(val) : "memory");
}

/* ------------------------------------------------------------------ */
/*  do_page_fault                                                     */
/* ------------------------------------------------------------------ */

static bool page_fault_access_error(unsigned long acc, struct vm_area_struct *vma)
{
	switch (acc) {
	case 0:
		return !(vma->vm_flags & (VM_READ | VM_WRITE));
	case 1:
		return !(vma->vm_flags & VM_WRITE);
	case 2:
		return !(vma->vm_flags & VM_EXEC);
	default:
		return true;
	}
}

static void show_unhandled_page_fault(struct pt_regs *regs, unsigned long cause,
					    unsigned long addr,
					    unsigned long acc,
					    unsigned long tpc,
					    unsigned long root)
{
	struct mm_struct *walk_mm = NULL;

	static const char * const access_names[] = { "read", "write", "exec" };
	const char *acc_str = (acc < 3) ? access_names[acc] : "???";

	pr_emerg("little64: PAGE FAULT at VA %016lx (%s), cause %lu\n",
		 addr, acc_str, cause);
	pr_emerg("  trap_pc=%016lx  epc=%016lx  cpu_ctl=%016lx\n",
		 tpc, regs->epc, regs->cpu_ctl);
	pr_emerg("  R1=%016lx R2=%016lx R3=%016lx R4=%016lx\n",
		 regs->regs[1], regs->regs[2], regs->regs[3], regs->regs[4]);
	pr_emerg("  R5=%016lx R6=%016lx R7=%016lx R8=%016lx\n",
		 regs->regs[5], regs->regs[6], regs->regs[7], regs->regs[8]);
	pr_emerg("  R9=%016lx R10=%016lx R11=%016lx R12=%016lx\n",
		 regs->regs[9], regs->regs[10], regs->regs[11], regs->regs[12]);
	pr_emerg("  SP=%016lx R14=%016lx R15=%016lx\n",
		 regs->regs[13], regs->regs[14], regs->regs[15]);
	pr_emerg("  SR11(root)=%016lx current=%px mm=%px active_mm=%px mm->pgd=%px init_mm.pgd=%px\n",
		 root, current, current ? current->mm : NULL,
		 current ? current->active_mm : NULL,
		 (current && current->mm) ? current->mm->pgd : NULL,
		 init_mm.pgd);
	walk_mm = current ? (current->mm ? current->mm : current->active_mm) : NULL;
	if (walk_mm) {
		const unsigned long idx2 = (addr >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1);
		const unsigned long idx1 = (addr >> PMD_SHIFT) & (PTRS_PER_PMD - 1);
		const unsigned long idx0 = (addr >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
		const pgd_t pgd = walk_mm->pgd[idx2];

		pr_emerg("  walk: idx2=%lu idx1=%lu idx0=%lu pgd=%016lx\n",
			 idx2, idx1, idx0, pgd_val(pgd));
		if (pgd_val(pgd) & _PAGE_PRESENT) {
			pmd_t *pmd_base = (pmd_t *)__va((pgd_val(pgd) >> PTE_PFN_SHIFT) << PAGE_SHIFT);
			const pmd_t pmd = pmd_base[idx1];

			pr_emerg("  walk: pmd=%016lx\n", pmd_val(pmd));
			if (pmd_val(pmd) & _PAGE_PRESENT) {
				pte_t *pte_base = (pte_t *)__va((pmd_val(pmd) >> PTE_PFN_SHIFT) << PAGE_SHIFT);
				const pte_t pte = pte_base[idx0];

				pr_emerg("  walk: pte=%016lx present=%d user=%d exec=%d write=%d read=%d\n",
					 pte_val(pte), pte_present(pte), pte_user(pte),
					 pte_exec(pte), pte_write(pte), pte_read(pte));
			}
		}
	}

	/* Clear trap_cause so stale value doesn't confuse next entry */
	write_sr(LITTLE64_SR_TRAP_CAUSE, 0);
}

static void do_page_fault(struct pt_regs *regs, unsigned long cause)
{
	unsigned long addr  = read_sr(LITTLE64_SR_TRAP_FAULT_ADDR);
	unsigned long acc   = read_sr(LITTLE64_SR_TRAP_ACCESS);
	unsigned long tpc   = read_sr(LITTLE64_SR_TRAP_PC);
	unsigned long root  = read_sr(LITTLE64_SR_PAGE_TABLE_ROOT_PHYSICAL);
	struct mm_struct *mm = current ? current->mm : NULL;
	struct vm_area_struct *vma;
	unsigned int flags = FAULT_FLAG_DEFAULT;
	int code = SEGV_MAPERR;
	vm_fault_t fault;

	if (acc == 1)
		flags |= FAULT_FLAG_WRITE;
	else if (acc == 2)
		flags |= FAULT_FLAG_INSTRUCTION;

	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;

	if (!regs_irqs_disabled(regs))
		local_irq_enable();

	if (likely(mm) && !faulthandler_disabled()) {
	retry:
		mmap_read_lock(mm);
		vma = find_vma(mm, addr);
		if (!vma || addr < vma->vm_start) {
			mmap_read_unlock(mm);
			goto bad_area;
		}

		code = SEGV_ACCERR;
		if (unlikely(page_fault_access_error(acc, vma))) {
			mmap_read_unlock(mm);
			goto bad_area;
		}

		fault = handle_mm_fault(vma, addr, flags, regs);
		if (fault_signal_pending(fault, regs)) {
			if (!user_mode(regs))
				goto no_context;
			return;
		}

		if (fault & VM_FAULT_COMPLETED)
			return;

		if (unlikely(fault & VM_FAULT_RETRY)) {
			flags |= FAULT_FLAG_TRIED;
			goto retry;
		}

		mmap_read_unlock(mm);

		if (!(fault & VM_FAULT_ERROR))
			return;

		if (fault & VM_FAULT_OOM) {
			if (!user_mode(regs))
				goto no_context;
			pagefault_out_of_memory();
			return;
		}

		if (fault & VM_FAULT_SIGBUS) {
			force_sig_fault(SIGBUS, BUS_ADRERR, (void __user *)addr);
			if (!user_mode(regs))
				goto no_context;
			return;
		}

		goto bad_area;
	}

	goto no_context;

bad_area:
	if (user_mode(regs)) {
		force_sig_fault(SIGSEGV, code, (void __user *)addr);
		return;
	}

no_context:
	show_unhandled_page_fault(regs, cause, addr, acc, tpc, root);

	panic("Unhandled page fault");
}

/* ------------------------------------------------------------------ */
/*  do_IRQ — external device interrupt                                */
/* ------------------------------------------------------------------ */

extern int little64_handle_irq(unsigned int hwirq);

static void noinstr do_IRQ(unsigned int hwirq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter_rcu();
	if (little64_handle_irq(hwirq))
		pr_warn_ratelimited("little64: failed to handle IRQ vector %u\n", hwirq);
	irq_exit_rcu();

	set_irq_regs(old_regs);
}

static long little64_invoke_syscall(struct pt_regs *regs, unsigned long nr)
{
	syscall_fn_t syscall_fn;

	if (unlikely(nr >= NR_syscalls))
		return -ENOSYS;

	syscall_fn = (syscall_fn_t)sys_call_table[nr];
	return syscall_fn(regs->regs[10], regs->regs[9], regs->regs[8],
			  regs->regs[7], regs->regs[6], regs->regs[5]);
}

static void noinstr do_user_syscall(struct pt_regs *regs)
{
	long nr;
	long ret;

	irqentry_enter_from_user_mode(regs);
	local_irq_enable();

	/* Skip over the 16-bit SYSCALL instruction before running the handler. */
	regs->epc += 2;

	nr = syscall_get_nr(current, regs);

	add_random_kstack_offset();
	ret = little64_invoke_syscall(regs, nr);
	syscall_set_return_value(current, regs, 0, ret);
	local_irq_disable();
	irqentry_exit_to_user_mode(regs);
}

/* ------------------------------------------------------------------ */
/*  do_trap — master dispatcher (called from entry.S)                 */
/* ------------------------------------------------------------------ */

/*
 * Determine the real trap/IRQ number:
 *   - Exceptions set trap_cause (SR24) to a non-zero compact exception vector.
 *   - Device IRQs leave trap_cause at zero; cpu_control bits [8:2] hold the
 *     current IRQ vector, which now lives above 64.
 */
asmlinkage void noinstr do_trap(struct pt_regs *regs)
{
	unsigned long trap_cause = read_sr(LITTLE64_SR_TRAP_CAUSE);
	unsigned long cpu_ctl    = read_sr(LITTLE64_SR_CPU_CONTROL);
	unsigned int  irq_num    = (cpu_ctl >> 2) & 0x7F;

	if (trap_cause != LITTLE64_TRAP_NONE) {
		/* --- Exception path --- */
		/* Clear trap_cause so it doesn't stick for next interrupt */
		write_sr(LITTLE64_SR_TRAP_CAUSE, 0);

		switch (trap_cause) {
		case LITTLE64_TRAP_EXEC_ALIGN:
			pr_emerg("little64: EXEC ALIGNMENT FAULT  epc=%016lx\n",
				 regs->epc);
			panic("Execution alignment fault");
			break;

		case LITTLE64_TRAP_PRIVILEGED_INSTRUCTION:
			pr_emerg("little64: PRIVILEGED INSTRUCTION  epc=%016lx  cpu_ctl=%016lx\n",
				 regs->epc, regs->cpu_ctl);
			panic("Privileged instruction in user mode");
			break;

		case LITTLE64_TRAP_SYSCALL:
			do_user_syscall(regs);
			break;

		case LITTLE64_TRAP_SYSCALL_FROM_SUPERVISOR:
			pr_emerg("little64: supervisor-mode SYSCALL at epc=%016lx nr=%016lx\n",
				 regs->epc, regs->regs[4]);
			panic("Supervisor-mode syscall trap");
			break;

		case LITTLE64_TRAP_PAGE_FAULT_NOT_PRESENT:
		case LITTLE64_TRAP_PAGE_FAULT_PERMISSION:
		case LITTLE64_TRAP_PAGE_FAULT_RESERVED:
		case LITTLE64_TRAP_PAGE_FAULT_CANONICAL:
			do_page_fault(regs, trap_cause);
			break;

		default:
			pr_emerg("little64: UNKNOWN EXCEPTION %lu  epc=%016lx\n",
				 trap_cause, regs->epc);
			panic("Unknown exception");
		}
		return;
	}

	/* --- Device IRQ path --- */
	if (!little64_is_irq_vector(irq_num)) {
		pr_emerg("little64: INVALID IRQ VECTOR %u  epc=%016lx cpu_ctl=%016lx\n",
			 irq_num, regs->epc, regs->cpu_ctl);
		panic("Invalid IRQ vector");
	}
	irqentry_state_t state = irqentry_enter(regs);
	do_IRQ(irq_num, regs);
	irqentry_exit(regs, state);
}

/* ------------------------------------------------------------------ */
/*  trap_init — install the vector table at boot                      */
/* ------------------------------------------------------------------ */

void __init trap_init(void)
{
	unsigned long table_phys;

	/*
	 * The vector table is a .data symbol linked at virtual address.
	 * SR16 (interrupt_table_base) is used by the MMU-enabled CPU to
	 * look up handlers, so it must hold the virtual address (since
	 * paging is on when interrupts fire).  The CPU fetches
	 *   handler = MEM64[SR16 + irq * 8]
	 * using the normal load path which goes through the MMU.
	 */
	table_phys = (unsigned long)little64_trap_table;
	write_sr(LITTLE64_SR_INTERRUPT_TABLE_BASE, table_phys);

	/*
	 * Export the thread_struct offset so that the assembly
	 * __switch_to can find it without asm-offsets.
	 */
	little64_thread_offset = offsetof(struct task_struct, thread);
}
