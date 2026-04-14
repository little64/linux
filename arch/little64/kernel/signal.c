// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/mm.h>
#include <linux/ptrace.h>
#include <linux/resume_user_mode.h>
#include <linux/signal.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>

#include <asm/cacheflush.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/signal.h>
#include <asm/syscall.h>
#include <asm/ucontext.h>

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
	unsigned short sigreturn_code[2];
};

static const unsigned short little64_rt_sigreturn_code[2] = {
	0x88b4, /* LDI #139, R4 */
	0xdb00, /* SYSCALL */
};

static inline void __user *get_sigframe(struct ksignal *ksig,
					struct pt_regs *regs,
					size_t frame_size)
{
	unsigned long sp = sigsp(user_stack_pointer(regs), ksig);

	sp = (sp - frame_size) & ~15UL;
	return (void __user *)sp;
}

static int setup_sigcontext(struct pt_regs *regs, struct sigcontext __user *sc)
{
	struct user_regs_struct saved = {
		.epc = regs->epc,
		.cpu_ctl = regs->cpu_ctl,
		.tp = regs->tp,
	};

	memcpy(saved.gpr, regs->regs, sizeof(saved.gpr));
	return copy_to_user(&sc->sc_regs, &saved, sizeof(saved)) ? -EFAULT : 0;
}

static int restore_sigcontext(struct pt_regs *regs, struct sigcontext __user *sc)
{
	struct user_regs_struct restored;

	current->restart_block.fn = do_no_restart_syscall;

	if (copy_from_user(&restored, &sc->sc_regs, sizeof(restored)))
		return -EFAULT;

	memcpy(regs->regs, restored.gpr, sizeof(regs->regs));
	regs->regs[0] = 0;
	regs->epc = restored.epc;
	regs->cpu_ctl = restored.cpu_ctl;
	regs->tp = restored.tp;
	regs->cpu_ctl &= LITTLE64_CPU_CTL_IRQ_ENABLE;
	regs->cpu_ctl |= LITTLE64_CPU_CTL_PAGING_ENABLE | LITTLE64_CPU_CTL_USER_MODE;
	regs->cpu_ctl &= ~(LITTLE64_CPU_CTL_IN_INTERRUPT |
			   LITTLE64_CPU_CTL_CURRENT_IRQ_MASK);

	return 0;
}

static int setup_rt_frame(struct ksignal *ksig, sigset_t *set,
			  struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	unsigned long restorer;
	int err = 0;

	frame = get_sigframe(ksig, regs, sizeof(*frame));
	if (!access_ok(frame, sizeof(*frame)))
		return -EFAULT;

	if (ksig->ka.sa.sa_flags & SA_SIGINFO)
		err |= copy_siginfo_to_user(&frame->info, &ksig->info);
	else
		err |= clear_user(&frame->info, sizeof(frame->info));

	err |= __put_user(0UL, &frame->uc.uc_flags);
	err |= __put_user(NULL, &frame->uc.uc_link);
	err |= __save_altstack(&frame->uc.uc_stack, user_stack_pointer(regs));
	err |= setup_sigcontext(regs, &frame->uc.uc_mcontext);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));
	err |= copy_to_user(frame->sigreturn_code, little64_rt_sigreturn_code,
			   sizeof(little64_rt_sigreturn_code));
	if (err)
		return -EFAULT;

	restorer = (unsigned long)frame->sigreturn_code;
	flush_icache_range(restorer, restorer + sizeof(frame->sigreturn_code));

	regs->regs[10] = (unsigned long)ksig->sig;
	regs->regs[9] = (unsigned long)&frame->info;
	regs->regs[8] = (unsigned long)&frame->uc;
	regs->regs[14] = restorer;
	regs->regs[13] = (unsigned long)frame;
	regs->epc = (unsigned long)ksig->ka.sa.sa_handler;

	return 0;
}

static void handle_signal(struct ksignal *ksig, struct pt_regs *regs)
{
	long ret = regs->regs[1];
	int setup_err;

	rseq_signal_deliver(ksig, regs);

	switch (ret) {
	case -ERESTART_RESTARTBLOCK:
		regs->regs[4] = __NR_restart_syscall;
		regs->epc -= 2;
		break;
	case -ERESTARTNOHAND:
		regs->regs[1] = -EINTR;
		break;
	case -ERESTARTSYS:
		if (ksig->ka.sa.sa_flags & SA_RESTART)
			regs->epc -= 2;
		else
			regs->regs[1] = -EINTR;
		break;
	case -ERESTARTNOINTR:
		regs->epc -= 2;
		break;
	default:
		break;
	}

	setup_err = setup_rt_frame(ksig, sigmask_to_save(), regs);
	signal_setup_done(setup_err, ksig, 0);
}

void arch_do_signal_or_restart(struct pt_regs *regs)
{
	struct ksignal ksig;
	long ret = regs->regs[1];

	if (!user_mode(regs))
		return;

	if (get_signal(&ksig)) {
		handle_signal(&ksig, regs);
		return;
	}

	restore_saved_sigmask();

	switch (ret) {
	case -ERESTART_RESTARTBLOCK:
		regs->regs[4] = __NR_restart_syscall;
		regs->epc -= 2;
		break;
	case -ERESTARTNOHAND:
	case -ERESTARTSYS:
	case -ERESTARTNOINTR:
		regs->epc -= 2;
		break;
	default:
		break;
	}
}

asmlinkage long sys_rt_sigreturn(struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	sigset_t set;

	frame = (struct rt_sigframe __user *)user_stack_pointer(regs);
	if (!access_ok(frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(regs, &frame->uc.uc_mcontext))
		goto badframe;
	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return regs->regs[1];

badframe:
	force_sig(SIGSEGV);
	return 0;
}

asmlinkage long little64_rt_sigreturn(void)
{
	return sys_rt_sigreturn(current_pt_regs());
}