#ifndef _UART_DRIVER_H
#define _UART_DRIVER_H

#include <types.h>

/* Registers */
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
# define UART_DRS_MASK   0x01 /* DataRxStat */
# define UART_ORE_MASK   0x02 /* OverRunErr */
# define UART_PE_MASK    0x04 /* ParErr */
# define UART_FE_MASK    0x08 /* FrameErr */
# define UART_BI_MASK    0x10 /* BI */
# define UART_THRE_MASK  0x20 /* THRE */
# define UART_TE_MASK    0x40 /* TxEmpty */
# define UART_RFE_MASK   0x80 /* RxFIFOErr */
#define UART_MSR_OFFSET 0x18
#define UART_SCR_OFFSET 0x1C

int uart_getc();
void uart_putc(char c);

#endif /* _UART_DRIVER_H */
