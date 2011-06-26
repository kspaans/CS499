#include <types.h>
#include <kern/printk.h>
#include <kern/ksyms.h>
#include <kern/backtrace.h>

#define MAXDEPTH 20

struct call_frame {
	void *fp;
	void *lr;
};

void unwind_stack(uint32_t *fp) {
	int depth = 0;
	while (fp) {
		if (depth++ > MAXDEPTH)
			break;

#ifdef __clang__
		struct call_frame *frame = (void *)fp;
#else
		struct call_frame *frame = (void *)(fp - 1);
#endif
		if (!frame->lr)
			break;
		uint32_t addr = (uint32_t)frame->lr - 4;
		printk("%08x(%s) ", addr, SYMBOL(addr));
		fp = frame->fp;
	}
}

void backtrace(void) {
	printk("stack\t");
	unwind_stack(__builtin_frame_address(0));
	printk("\n");
}
