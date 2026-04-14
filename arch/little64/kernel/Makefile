#
# Makefile for Little64 kernel parts
#

extra-y = vmlinux.lds
obj-y += head.o

# Must-have for first direct ELF boot handoff and basic arch init.
obj-y += setup.o stacktrace.o process.o
obj-y += signal.o

# Syscall table and userspace trap dispatch.
obj-y += syscall_table.o

# Trap / exception / IRQ framework (entry.S + traps.c).
obj-y += entry.o traps.o

# Timer / clockevent driver.
obj-y += timer.o

# Bring-up placeholders required to satisfy generic kernel link/runtime hooks.
# See README.bringup-stubs.md for ownership and replacement plan.
obj-y += ptrace.o time.o irq.o delay.o cpuinfo.o
