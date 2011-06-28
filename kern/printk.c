/* General I/O and system functions */
#include <drivers/uart.h>
#include <lib.h>
#include <kern/task.h>
#include <kern/printk.h>
#include <panic.h>

__attribute__((noreturn))
void prm_reset(void);

static int panic_recursion;

/* Busy-wait I/O */
static int bw_printfunc(void *unused, const char *str, size_t len) {
	(void)unused;
	while(len--) {
		/* busy wait */
		if(*str == '\n') {
			uart_putc('\r');
			uart_putc('\n');
		} else {
			uart_putc(*str++);
		}
	}
	return 0;
}

int printk(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(bw_printfunc, NULL, fmt, va);
	va_end(va);
	return ret;
}

void panic(const char *fmt, ...) {
	printk("panic: ");

	va_list va;
	va_start(va, fmt);
	func_vprintf(bw_printfunc, NULL, fmt, va);
	va_end(va);

	printk("\n");

	if (!panic_recursion) {
		panic_recursion = 1;
		dump_tasks();
	}

	/* hacks */
	for (volatile int i = 0; i < 10000000; ++i);

	prm_reset();
}

void kernel_dabt(struct regs *regs) {
	printk("Kernel Data Abort\n");
	print_regs(regs);
	prm_reset();
}

void kernel_pabt(struct regs *regs) {
	printk("Kernel Prefetch Abort\n");
	print_regs(regs);
	prm_reset();
}

void kernel_und(struct regs *regs) {
	printk("Kernel Undefined Instruction\n");
	print_regs(regs);
	prm_reset();
}

void kernel_irq(struct regs *regs) {
	printk("Kernel IRQ\n");
	print_regs(regs);
	prm_reset();
}

int vprintk(const char *fmt, va_list va) {
	return func_vprintf(bw_printfunc, NULL, fmt, va);
}
