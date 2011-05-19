#ifndef OMAP3_MACHINE_H
#define OMAP3_MACHINE_H

/* UARTs */
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

#define TIDR 0x000 // timer revision
#define TIOCP_CFG 0x010 // interface options
# define TIOCP_CFG_SOFTRESET 0x2
#define TISTAT 0x014 // timer status
# define TISTAT_RESETDONE 0x1
#define TISR 0x018 // timer interrupt status
#define TIER 0x01C // timer interrupt enable
#define TCLR 0x024 // timer control register
# define TCLR_AR 0x2 // autoreload enable
# define TCLR_ST 0x1 // timer start
#define TCRR 0x028 // this is where the time is
#define TLDR 0x02C // timer load register (loads this on overflow)
#define TOCR 0x054 // timer overflow counter (timers 1, 2 and 10 only)

/* GPIO (OMAP3 chapter 24) */
#define GPIO1_BASE 0x48310000
#define GPIO2_BASE 0x40950000
#define GPIO3_BASE 0x40952000
#define GPIO4_BASE 0x40954000
#define GPIO5_BASE 0x40956000
#define GPIO6_BASE 0x40958000
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
#define GPIO_CLEARIRQENABLE1 0x060
#define GPIO_SETIRQENABLE1 0x064
#define GPIO_CLEARIRQENABLE2 0x070
#define GPIO_SETIRQENABLE2 0x074

/* ethernet chips mapped by U-Boot */
#define ETH1_BASE 0x2C000000
#define ETH1_GPMC_SLOT 5
#define ETH2_BASE 0x2B000000
#define ETH2_GPMC_SLOT 4
#define ETH_RX_FIFO_OFFSET 0x00
#define ETH_RX_FIFO_LEN 0x20
#define ETH_TX_FIFO_OFFSET 0x20
#define ETH_TX_FIFO_LEN 0x20
#define ETH_RX_STS_FIFO_OFFSET 0x40
#define ETH_RX_STS_PEEK_OFFSET 0x44
#define ETH_TX_STS_FIFO_OFFSET 0x48
#define ETH_TX_STS_PEEK_OFFSET 0x4C
#define ETH_ID_REV_OFFSET 0x50
#define ETH_IRQ_CFG_OFFSET 0x54
#define ETH_INT_STS_OFFSET 0x58
#define ETH_INT_EN_OFFSET 0x5C
#define ETH_RESERVED_OFFSET 0x60
#define ETH_BYTE_TEST_OFFSET 0x64
#define ETH_FIFO_INT_OFFSET 0x68
#define ETH_RX_CFG_OFFSET 0x6C
#define ETH_TX_CFG_OFFSET 0x70
# define ETH_TX_CFG_ON 0x00000002
#define ETH_HW_CFG_OFFSET 0x74
# define ETH_HW_CFG_SRST 0x00000001
# define ETH_HW_CFG_SRST_TO 0x00000002
# define ETH_HW_CFG_MBO 0x00100000
#define ETH_RX_DP_CTL_OFFSET 0x78
#define ETH_RX_FIFO_INF_OFFSET 0x7C
#define ETH_TX_FIFO_INF_OFFSET 0x80
#define ETH_PMT_CTRL_OFFSET 0x84
# define ETH_PMT_CTRL_READY 0x00000001
# define ETH_PMT_CTRL_PHY_RST 0x00000400
#define ETH_GPIO_CFG_OFFSET 0x88
#define ETH_GPT_CFG_OFFSET 0x8C
#define ETH_GPT_CNT_OFFSET 0x90
#define ETH_MAC_CSR_CMD_OFFSET 0xA4
# define ETH_MAC_CSR_BUSY 0x80000000
# define ETH_MAC_CSR_READ 0x40000000
#define ETH_MAC_CSR_DATA_OFFSET 0xA8
# define ETH_MAC_MAC_CR 1
#  define ETH_MAC_TXEN 0x00000008
#  define ETH_MAC_RXEN 0x00000004
# define ETH_MAC_ADDRH 2
# define ETH_MAC_ADDRL 3
# define ETH_MAC_MII_ACC 6
#  define ETH_MII_BCR 0
#   define ETH_MII_BCR_RESET 0x8000
#   define ETH_MII_BCR_ANENABLE 0x1000
#   define ETH_MII_BCR_ANRESTART 0x0200
#  define ETH_MII_BSR 1
#   define ETH_MII_BSR_LSTS 0x0004
#  define ETH_MII_ADVERTISE 4
# define ETH_MAC_MII_DATA 7
#  define ETH_MII_ACC_BUSY 0x00000001
#  define ETH_MII_ACC_WRITE 0x00000002
#  define ETH_MII_ACC_PHY 0x00000800
# define ETH_MAC_FLOW 8
#  define ETH_MAC_FLOW_FCPT 0xFFFF0000
#  define ETH_MAC_FLOW_FCEN 0x00000002
#define ETH_AFC_CFG_OFFSET 0xAC
#define ETH_E2P_CMD_OFFSET 0xB0
# define ETH_E2P_MACLOADED 0x00000100
#define ETH_E2P_DATA_OFFSET 0xB4

/* Vectored interrupt controller (OMAP3 chapter 10) */
#define INTC_PHYS_BASE 0x48200000
#define INTCPS_REVISION_OFFSET 0x0000
#define INTCPS_SYSCONFIG_OFFSET 0x0010
# define INTCPS_SOFTRESET 0x2
# define INTCPS_AUTOIDLE 0x1
#define INTCPS_SYSSTATUS_OFFSET 0x0014
# define INTCPS_RESETDONE 0x1
#define INTCPS_SIR_IRQ_OFFSET 0x0040
# define INTCPS_ACTIVEIRQ_MASK 0x7F
#define INTCPS_SIR_FIQ_OFFSET 0x0044
#define INTCPS_CONTROL_OFFSET 0x0048
# define INTCPS_NEWFIQAGR 0x2
# define INTCPS_NEWIRQAGR 0x1
#define INTCPS_PROTECTION_OFFSET 0x004C
#define INTCPS_IDLE_OFFSET 0x0050
#define INTCPS_IRQ_PRIORITY_OFFSET 0x0060
#define INTCPS_FIQ_PRIORITY_OFFSET 0x0064
#define INTCPS_THRESHOLD_OFFSET 0x0068
#define INTCPS_VECTORS_OFFSET 0x0080
#define INTCPS_ILR_OFFSET 0x100

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
