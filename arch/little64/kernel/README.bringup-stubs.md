# Little64 Arch Kernel Bring-up Stub Map

This file separates first-boot must-have code from temporary placeholders.

## Must-Have (keep for initial direct boot)

- setup.c
  - setup_arch()
  - setup_arch() must seed `min_low_pfn` / `max_low_pfn` / `max_pfn` from DT-backed `memblock` RAM before generic MM init.
  - setup_arch() must reserve the loaded kernel image and early DT/initrd ranges in `memblock` before page allocator users like SLUB run.
  - setup_arch() must also reserve the three page-table pages that `head.S` allocates immediately after `__bss_stop`, and must point `init_mm.pgd` at that live root before any early fixmap users run.
  - machine_restart()/machine_halt()/machine_power_off()
- mm/init.c + asm/fixmap.h
  - must provide a working fixmap slot for `FIX_EARLYCON_MEM_BASE` so early 8250 console setup can map UART MMIO before slab-backed `ioremap()` is available.
- asm/tlbflush.h + asm/mmu_context.h
  - page-table updates only become visible to the emulator after SR11 is rewritten with the current root's physical address; kernel flush helpers and context switches must republish that physical root.
- process.c
  - copy_thread()
  - show_regs()/show_stack()
- stacktrace.c
  - __get_wchan()

These are needed for early arch bring-up and generic kernel integration.

## Placeholder (replace after first successful boot)

- irq.c
  - init_IRQ() is a no-op until an interrupt controller is wired.
- time.c
  - time_init() uses a conservative fixed delay baseline.
- delay.c
  - delay calibration is fixed/fallback, not hardware-derived.
- ptrace.c
  - minimal ptrace passthrough and no-op disable.
- cpuinfo.c
  - reports static single-core model info.
- process.c
  - __switch_to() currently returns prev as a temporary shim.
  - current task tracking is currently a global pointer updated by __switch_to(), not a dedicated CPU register.
  - elf_core_copy_task_fpregs() currently returns success without FP copy.

## First-Boot Replacement Order

1. irq.c + timer source (required for scheduler tick and reliable progress).
2. process.c context switch + ptrace register/FP state correctness.
3. time.c/delay.c proper clocksource/clockevent calibration.
4. cpuinfo.c dynamic CPU feature reporting.

## Guardrail

When replacing a placeholder, remove the corresponding warning path and update this file in the same change.
