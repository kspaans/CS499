#ifndef _UART_DRIVER_H
#define _UART_DRIVER_H

#include <types.h>

/* Registers */
#define UART_RHR_OFFSET 0x00 // R/O, mode op
#define UART_THR_OFFSET 0x00 // W/O, mode op
#define UART_DLL_OFFSET 0x00 // R/W, mode A/B
#define UART_IER_OFFSET 0x04 // R/W, mode op
# define UART_LINE_STS_IT 0x04
# define UART_THR_IT 0x02
# define UART_RHR_IT 0x01
#define UART_DLH_OFFSET 0x04 // R/W, mode A/B
#define UART_IIR_OFFSET 0x08 // R/O, mode A/op
# define UART_IIR_RHR (2 << 1)
# define UART_IIR_THR (1 << 1)
# define UART_IIR_IT_MASK 0x3f
#define UART_FCR_OFFSET 0x08 // W/O, mode A/op
# define UART_FCR_FIFO_EN 0x01
#define UART_EFR_OFFSET 0x08 // R/O, mode B
# define UART_EFR_ENHANCED_EN 0x10
#define UART_LCR_OFFSET 0x0C // R/W, mode A/B/op
# define UART_CHARLEN_MASK 0x03
#  define UART_CHARLEN_5 0x00
#  define UART_CHARLEN_6 0x01
#  define UART_CHARLEN_7 0x02
#  define UART_CHARLEN_8 0x03
# define UART_NB_STOP 0x04
# define UART_PARITY_EN 0x08
# define UART_BREAK_EN 0x40
# define UART_DIV_EN 0x80
#define UART_MCR_OFFSET 0x10 // R/W, mode A/op
#define UART_LSR_OFFSET 0x14 // R/O, mode A/op
# define UART_DRS_MASK   0x01 /* DataRxStat */
# define UART_ORE_MASK   0x02 /* OverRunErr */
# define UART_PE_MASK    0x04 /* ParErr */
# define UART_FE_MASK    0x08 /* FrameErr */
# define UART_BI_MASK    0x10 /* BI */
# define UART_THRE_MASK  0x20 /* THRE */
# define UART_TE_MASK    0x40 /* TxEmpty */
# define UART_RFE_MASK   0x80 /* RxFIFOErr */
#define UART_MSR_OFFSET 0x18 // R/O, mode A/op, submode MSR_SPR
#define UART_TLR_OFFSET 0x1C // R/W, mode A/B/op, submode TCR_TLR
#define UART_MDR1_OFFSET 0x20 // R/W, mode A/B/op
# define UART_MODE_UART16x 0x0
# define UART_MODE_DISABLE 0x7
#define UART_SCR_OFFSET 0x40 // R/W, mode A/B/op
# define UART_SCR_RX_TRIG_GRANU1 0x80
# define UART_SCR_TX_TRIG_GRANU1 0x40
#define UART_SYSC_OFFSET 0x54 // R/W, mode A/B/op
# define UART_SYSC_SOFTRESET 0x02
#define UART_SYSS_OFFSET 0x58 // R/O, mode A/B/op
# define UART_SYSS_RESETDONE 0x01

void uart_init();
int uart_intstatus();
void uart_intenable(int interrupt);
void uart_intdisable(int interrupt);
int uart_getc();
void uart_putc(char c);

#endif /* _UART_DRIVER_H */
