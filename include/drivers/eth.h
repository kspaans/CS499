#ifndef _ETH_DRIVER_H
#define _ETH_DRIVER_H

#include <types.h>
#include <eth.h>

/* Registers */
#define ETH_RX_FIFO_OFFSET 0x00
#define ETH_RX_FIFO_LEN 0x20
#define ETH_TX_FIFO_OFFSET 0x20
#define ETH_TX_FIFO_LEN 0x20
#define ETH_RX_STS_FIFO_OFFSET 0x40
#define ETH_RX_STS_PEEK_OFFSET 0x44
#define ETH_TX_STS_FIFO_OFFSET 0x48
#define ETH_TX_STS_PEEK_OFFSET 0x4C
# define ETH_TX_STS_LOST_CARRIER (1<<11)
# define ETH_TX_STS_NO_CARRIER (1<<10)
# define ETH_TX_STS_LATE_COLLISION (1<<9)
# define ETH_TX_STS_EXCESS_COLLISION (1<<8)
# define ETH_TX_STS_EXCESS_DEFER (1<<2)
# define ETH_TX_STS_ERROR ((1<<11)|(1<<9)|(1<<8)|(1<<2))
#define ETH_ID_REV_OFFSET 0x50
#define ETH_IRQ_CFG_OFFSET 0x54
# define ETH_IRQ_EN (1<<8)
# define ETH_IRQ_INT (1<<12)
#define ETH_INT_STS_OFFSET 0x58
#define ETH_INT_EN_OFFSET 0x5C
# define ETH_INT_RSFL (1<<3) // RX Status Level
# define ETH_INT_RSFF (1<<4) // RX Status Full
# define ETH_INT_TSFL (1<<7) // TX Status Level
# define ETH_INT_TSFF (1<<8) // TX Status Full
# define ETH_INT_TDFA (1<<9) // TX Data Available
# define ETH_INT_RXE (1<<13) // RX Error
# define ETH_INT_TXE (1<<14) // TX Error
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
# define ETH_GPT_TIMER_EN 0x20000000
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
# define ETH_E2P_EPC_BUSY  0x80000000
# define ETH_E2P_MACLOADED 0x00000100
#define ETH_E2P_DATA_OFFSET 0xB4

#define MAKE_BTAG(tag,len) (((tag) << 16) | ((len) & 0x7ff))

int eth_init();
void eth_set_rxlevel(int level);
void eth_set_txlevel(int level);
int eth_intstatus();
void eth_intreset(int interrupt);
void eth_intenable(int interrupt);
void eth_intdisable(int interrupt);
void eth_tx(int base, const void *buf, uint16_t nbytes, int first, int last, uint32_t btag);
void eth_rx(int base, uint32_t *buf, uint16_t nbytes);
uint32_t eth_rx_wait_sts(int base);
uint32_t eth_tx_wait_sts(int base);
mac_addr_t eth_mac_addr(int base);

#endif /* _ETH_DRIVER_H */
