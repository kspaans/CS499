#include <types.h>
#include <kern/printk.h>

#define MAXDEPTH 20

struct apcs_frame {
	void *fp;
	void *sp;
	void *lr;
	void *pc;
};

void unwind_stack(uint32_t *fp) {
	int depth = 0;
	while (fp) {
		if (depth++ > MAXDEPTH)
			break;

		struct apcs_frame *frame = (void *)(fp - 3);
		if (!frame->lr)
			break;
		uint32_t addr = (uint32_t)frame->lr - 4;
		printk("%08x ", addr);
		fp = frame->fp;
	}
}

void backtrace(void) {
	printk("stack\t");
	unwind_stack(__builtin_frame_address(0));
	printk("\n");
}
