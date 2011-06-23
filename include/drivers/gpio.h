#ifndef _GPIO_DRIVER_H
#define _GPIO_DRIVER_H

#include <types.h>
#include <kern/interrupt.h>

#define GPIO_REVISION_OFFSET 0x000
#define GPIO_SYSCONFIG_OFFSET 0x010
#define GPIO_SYSSTATUS_OFFSET 0x014
#define GPIO_IRQSTATUS1_OFFSET 0x018
#define GPIO_IRQENABLE1_OFFSET 0x01C
#define GPIO_WAKEUPENABLE_OFFSET 0x020
#define GPIO_IRQSTATUS2_OFFSET 0x028
#define GPIO_IRQENABLE2_OFFSET 0x02C
#define GPIO_CTRL_OFFSET 0x030
#define GPIO_OE_OFFSET 0x034
#define GPIO_DATAIN_OFFSET 0x038
#define GPIO_DATAOUT_OFFSET 0x03C
#define GPIO_LEVELDETECT0_OFFSET 0x040
#define GPIO_LEVELDETECT1_OFFSET 0x044
#define GPIO_RISINGDETECT_OFFSET 0x048
#define GPIO_FALLINGDETECT_OFFSET 0x04C
#define GPIO_CLEARIRQENABLE1_OFFSET 0x060
#define GPIO_SETIRQENABLE1_OFFSET 0x064
#define GPIO_CLEARIRQENABLE2_OFFSET 0x070
#define GPIO_SETIRQENABLE2_OFFSET 0x074
#define GPIO_CLEARDATAOUT_OFFSET 0x090
#define GPIO_SETDATAOUT_OFFSET 0x094

/* flags for gpio_register */
#define GPIOIRQ_LEVELDETECT0 0x01
#define GPIOIRQ_LEVELDETECT1 0x02
#define GPIOIRQ_RISINGDETECT 0x04
#define GPIOIRQ_FALLINGDETECT 0x08

void gpio_init(void);

void gpio_register(int pin, isr_func handler, int flags);
void gpio_intenable(int pin);
void gpio_intdisable(int pin);

void gpio_set(int pin, int value);
int gpio_get(int pin);
void gpio_irq(int irq);

#endif /* _GPIO_DRIVER_H */
