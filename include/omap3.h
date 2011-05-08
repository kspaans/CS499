#ifndef OMAP3_H
#define OMAP3_H

#define UART1_PHYS_BASE 0x4806a000
#define UART2_PHYS_BASE 0x4806c000
#define UART3_PHYS_BASE 0x49020000

#define UART_RBR_OFFSET 0x00
#define UART_THR_OFFSET 0x00
#define UART_DLL_OFFSET 0x00
#define UART_IER_OFFSET 0x04
#define UART_DLH_OFFSET 0x04
#define UART_IIR_OFFSET 0x08
#define UART_FCR_OFFSET 0x08
#define UART_LCR_OFFSET 0x0C
#define UART_MCR_OFFSET 0x10
#define UART_LSR_OFFSET 0x14
#define UART_MSR_OFFSET 0x18
#define UART_SCR_OFFSET 0x1C

/* Line Status Register */
#define UART_DRS_MASK  0x01 /* DataRxStat */
#define UART_ORE_MASK  0x02 /* OverRunErr */
#define UART_PE_MASK   0x04 /* ParErr */
#define UART_FE_MASK   0x08 /* FrameErr */
#define UART_BI_MASK   0x10 /* BI */
#define UART_THRE_MASK 0x20 /* THRE */
#define UART_TE_MASK   0x40 /* TxEmpty */
#define UART_RFE_MASK  0x80 /* RxFIFOErr */

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

#define TISR 0x018 // timer interrupt status
#define TIER 0x01C // timer interrupt enable
#define TCLR 0x024 // timer control register
#define TCCR 0x028 // this is where the time is
#define TLDR 0x02C // timer load register (loads this on overflow)

#endif
