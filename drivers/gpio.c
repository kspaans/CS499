#include <machine.h>
#include <drivers/gpio.h>
#include <drivers/intc.h>
#include <mem.h>
#include <kern/printk.h>

const int gpio_bases[6] = {GPIO1_BASE, GPIO2_BASE, GPIO3_BASE, GPIO4_BASE, GPIO5_BASE, GPIO6_BASE};

static isr_func isrs[6][32];

#define GPIO_BASE(pin) gpio_bases[(pin)>>5]
#define GPIO_MASK(pin) (1<<((pin)&0x1f))

void gpio_register(int pin, isr_func handler, int flags) {
	isrs[pin>>5][pin&0x1f] = handler;
	/* set pin to input */
	mem32(GPIO_BASE(pin) + GPIO_OE_OFFSET) |= GPIO_MASK(pin);

#define DOFLAG(x) if(flags & GPIOIRQ_##x) \
		mem32(GPIO_BASE(pin) + GPIO_##x##_OFFSET) |= GPIO_MASK(pin); \
	else \
		mem32(GPIO_BASE(pin) + GPIO_##x##_OFFSET) &= ~GPIO_MASK(pin);

	DOFLAG(LEVELDETECT0)
	DOFLAG(LEVELDETECT1)
	DOFLAG(RISINGDETECT)
	DOFLAG(FALLINGDETECT)
#undef DOFLAG
}

void gpio_intenable(int pin) {
	write32(GPIO_BASE(pin) + GPIO_SETIRQENABLE1_OFFSET, GPIO_MASK(pin));
}

void gpio_intdisable(int pin) {
	write32(GPIO_BASE(pin) + GPIO_CLEARIRQENABLE1_OFFSET, GPIO_MASK(pin));
}

void gpio_irq(int irq) {
	int bank = irq - IRQ_GPIO1;
	uint32_t status = mem32(gpio_bases[bank] + GPIO_IRQSTATUS1_OFFSET);
	if(!status) {
		printk("Bad GPIO IRQ: no lines active\n");
		return;
	}
	int pin = 31-__builtin_clz(status);
	if(isrs[bank][pin] == NULL) {
		printk("Bad GPIO IRQ %d: no ISR associated\n", (bank<<5)+pin);
		return;
	}
	isrs[bank][pin]((bank<<5) + pin);
	write32(gpio_bases[bank] + GPIO_IRQSTATUS1_OFFSET, 1<<pin);
}

void gpio_init() {
	for(int i=0; i<6; i++) {
		write32(gpio_bases[i] + GPIO_IRQENABLE1_OFFSET, 0x00000000);
		for(int j=0; j<32; j++) {
			isrs[i][j] = NULL;
		}
	}
	intc_register(IRQ_GPIO1, gpio_irq, 0);
	intc_register(IRQ_GPIO2, gpio_irq, 0);
	intc_register(IRQ_GPIO3, gpio_irq, 0);
	intc_register(IRQ_GPIO4, gpio_irq, 0);
	intc_register(IRQ_GPIO5, gpio_irq, 0);
	intc_register(IRQ_GPIO6, gpio_irq, 0);
	intc_intenable(IRQ_GPIO1);
	intc_intenable(IRQ_GPIO2);
	intc_intenable(IRQ_GPIO3);
	intc_intenable(IRQ_GPIO4);
	intc_intenable(IRQ_GPIO5);
	intc_intenable(IRQ_GPIO6);
}

void gpio_set(int pin, int value) {
	/* set pin to output */
	mem32(GPIO_BASE(pin) + GPIO_OE_OFFSET) &= ~GPIO_MASK(pin);

	if(value)
		write32(GPIO_BASE(pin) + GPIO_SETDATAOUT_OFFSET, GPIO_MASK(pin));
	else
		write32(GPIO_BASE(pin) + GPIO_CLEARDATAOUT_OFFSET, GPIO_MASK(pin));
}

int gpio_get(int pin) {
	/* set pin to input */
	mem32(GPIO_BASE(pin) + GPIO_OE_OFFSET) |= GPIO_MASK(pin);

	return (mem32(GPIO_BASE(pin) + GPIO_DATAIN_OFFSET) & GPIO_MASK(pin)) != 0;
}
