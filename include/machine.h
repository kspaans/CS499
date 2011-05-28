#ifndef OMAP3_MACHINE_H
#define OMAP3_MACHINE_H

/* UARTs */
#define UART1_PHYS_BASE 0x4806a000
#define UART2_PHYS_BASE 0x4806c000
#define UART3_PHYS_BASE 0x49020000

/* PRCM */
#define PRM_GLOBAL_BASE 0x48307200
#define CM_CORE_BASE 0x48004A00
#define CM_WKUP_BASE 0x48004C00
#define CM_PER_BASE 0x48005000

/* Watchdog timer */
#define WDT2_PHYS_BASE 0x48314000

/* Timers (OMAP3 chapter 16) */
#define GPTIMER1 0x48318000
#define GPTIMER2 0x49032000
#define GPTIMER3 0x49034000
#define GPTIMER4 0x49036000
#define GPTIMER5 0x49038000
#define GPTIMER6 0x4903A000
#define GPTIMER7 0x4903C000
#define GPTIMER8 0x4903E000
#define GPTIMER9 0x49040000
#define GPTIMER10 0x48086000
#define GPTIMER11 0x48088000

/* GPIO (OMAP3 chapter 24) */
#define GPIO1_BASE 0x48310000
#define GPIO2_BASE 0x49050000
#define GPIO3_BASE 0x49052000
#define GPIO4_BASE 0x49054000
#define GPIO5_BASE 0x49056000
#define GPIO6_BASE 0x49058000
extern const int gpio_bases[6];

/* GPIO pins */
#define GPIO_ETH1_IRQ 176
#define GPIO_LED1 21 /* stagecoach red LED */
#define GPIO_LED2 22 /* stagecoach blue LED */

/* ethernet chips mapped by U-Boot */
#define ETH1_BASE 0x2C000000
#define ETH1_GPMC_SLOT 5
#define ETH2_BASE 0x2B000000
#define ETH2_GPMC_SLOT 4

/* Vectored interrupt controller (OMAP3 chapter 10) */
#define INTC_PHYS_BASE 0x48200000

#define IRQ_GPIO1 29
#define IRQ_GPIO2 30
#define IRQ_GPIO3 31
#define IRQ_GPIO4 32
#define IRQ_GPIO5 33
#define IRQ_GPIO6 34
#define IRQ_GPT1 37
#define IRQ_GPT2 38
#define IRQ_GPT3 39
#define IRQ_GPT4 40
#define IRQ_GPT5 41
#define IRQ_GPT6 42
#define IRQ_GPT7 43
#define IRQ_GPT8 44
#define IRQ_GPT9 45
#define IRQ_GPT10 46
#define IRQ_GPT11 47
#define IRQ_UART1 72
#define IRQ_UART2 73
#define IRQ_UART3 74

#endif /* OMAP3_MACHINE_H */
