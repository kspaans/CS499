#include <inttypes.h>
#include <printk.h>

struct apcs_frame {
	void *fp;
	void *sp;
	void *lr;
	void *pc;
};

void unwind_stack(uint32_t *fp) {
	while (fp) {
		struct apcs_frame *frame = (void *)(fp - 3);
		if (!frame->lr)
			break;
		uint32_t addr = (uint32_t)frame->lr - 4;
		printk("%x ", addr);
		fp = frame->fp;
	}
}

void backtrace(void) {
	printk("stack\t");
	unwind_stack(__builtin_frame_address(0));
	printk("\r\n");
}
