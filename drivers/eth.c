#include <machine.h>
#include <drivers/eth.h>
#include <timer.h>
#include <types.h>
#include <mem.h>
#include <kern/printk.h>

static uint32_t eth_read(int base, int offset) {
	return mem32(base + offset);
}

static void eth_write(int base, int offset, uint32_t value) {
	write32(base + offset, value);
}

static void eth_mac_wait(int base) {
	while(eth_read(base, ETH_MAC_CSR_CMD_OFFSET) & ETH_MAC_CSR_BUSY)
		;
}

static uint32_t eth_mac_read(int base, int index) {
	eth_mac_wait(base);
	eth_write(base, ETH_MAC_CSR_CMD_OFFSET, ETH_MAC_CSR_BUSY | ETH_MAC_CSR_READ | index);
	eth_mac_wait(base);
	return eth_read(base, ETH_MAC_CSR_DATA_OFFSET);
}

static void eth_mac_write(int base, int index, uint32_t value) {
	eth_mac_wait(base);
	eth_write(base, ETH_MAC_CSR_DATA_OFFSET, value);
	eth_write(base, ETH_MAC_CSR_CMD_OFFSET, ETH_MAC_CSR_BUSY | index);
	eth_mac_wait(base);
}

static void eth_phy_wait(int base) {
	while(eth_mac_read(base, ETH_MAC_MII_ACC) & ETH_MII_ACC_BUSY)
		;
}

static uint16_t eth_phy_read(int base, int index) {
	eth_phy_wait(base);
	eth_mac_write(base, ETH_MAC_MII_ACC, ETH_MII_ACC_PHY | (index << 6) | ETH_MII_ACC_BUSY);
	eth_phy_wait(base);
	return eth_mac_read(base, ETH_MAC_MII_DATA);
}

static void eth_phy_write(int base, int index, uint16_t value) {
	eth_phy_wait(base);
	eth_mac_write(base, ETH_MAC_MII_DATA, value);
	eth_mac_write(base, ETH_MAC_MII_ACC, ETH_MII_ACC_PHY | (index << 6) | ETH_MII_ACC_BUSY | ETH_MII_ACC_WRITE);
	eth_phy_wait(base);
}

static int eth_reset(int base) {
	uint32_t reg;
	/* wakeup */
	reg = eth_read(base, ETH_PMT_CTRL_OFFSET);
	if(!(reg & ETH_PMT_CTRL_READY)) {
		printk("eth: waking up controller\n");
		/* write to byte test to wake up controller */
		eth_write(base, ETH_BYTE_TEST_OFFSET, 0x87654321);
		while(!(eth_read(base, ETH_PMT_CTRL_OFFSET)) & ETH_PMT_CTRL_READY)
			;
	}

	/* interrupt disable */
	eth_write(base, ETH_INT_EN_OFFSET, 0);

	/* soft reset */
	eth_write(base, ETH_HW_CFG_OFFSET, ETH_HW_CFG_SRST);
	printk("eth: waiting for soft reset\n");
	while(1) {
		reg = eth_read(base, ETH_HW_CFG_OFFSET);
		if(!(reg & ETH_HW_CFG_SRST))
			break;
		if(reg & ETH_HW_CFG_SRST_TO) {
			/* timeout */
			printk("eth: soft reset timeout\n");
			return -1;
		}
	}

	/* reset flow control settings */
	eth_mac_write(base, ETH_MAC_FLOW, ETH_MAC_FLOW_FCPT | ETH_MAC_FLOW_FCEN);
	// AFC_HI=80  AFC_LO=40  BACK_DUR=7  FCMULT|FCBRD|FCADD|FCANY
	eth_write(base, ETH_AFC_CFG_OFFSET, (80 << 16) | (40 << 8) | (7 << 4) | 0xF);
	return 0;
}

mac_addr_t eth_mac_addr(int base) {
	mac_addr_t res;
	if(eth_read(base, ETH_E2P_CMD_OFFSET) & ETH_E2P_MACLOADED) {
		uint32_t reg;
		reg = eth_mac_read(base, ETH_MAC_ADDRL);
		res.addr[0] = reg & 0xff;
		res.addr[1] = (reg >> 8) & 0xff;
		res.addr[2] = (reg >> 16) & 0xff;
		res.addr[3] = (reg >> 24) & 0xff;
		reg = eth_mac_read(base, ETH_MAC_ADDRH);
		res.addr[4] = reg & 0xff;
		res.addr[5] = (reg >> 8) & 0xff;
	} else {
		res.addr[0] = 0xff;
		res.addr[1] = 0xff;
		res.addr[2] = 0xff;
		res.addr[3] = 0xff;
		res.addr[4] = 0xff;
		res.addr[5] = 0xff;
	}
	return res;
}

static int eth_dev_init(int base) {
	printk("eth init\n");

	uint32_t reg;
	if(eth_reset(base) != 0)
		return -1;

	/* XXX does the phy really need to be reset twice? */

	printk("eth: resetting phy via PMT\n");
	/* phy reset */
	reg = eth_read(base, ETH_PMT_CTRL_OFFSET);
	reg &= 0xFCF; /* clear power management mode and wakeup status */
	reg |= ETH_PMT_CTRL_PHY_RST;
	eth_write(base, ETH_PMT_CTRL_OFFSET, reg);
	while(eth_read(base, ETH_PMT_CTRL_OFFSET) & ETH_PMT_CTRL_PHY_RST)
		;

	printk("eth: resetting phy via MII\n");
	eth_phy_write(base, ETH_MII_BCR, ETH_MII_BCR_RESET);
	while(eth_phy_read(base, ETH_MII_BCR) & ETH_MII_BCR_RESET)
		;

	printk("eth: waiting for line\n");
	eth_phy_write(base, ETH_MII_ADVERTISE, 0x01e1);
	eth_phy_write(base, ETH_MII_BCR, ETH_MII_BCR_ANENABLE | ETH_MII_BCR_ANRESTART);
	while(!(eth_phy_read(base, ETH_MII_BSR) & ETH_MII_BSR_LSTS))
		;
	printk("eth: line up\n");

	/* configuration */
	/* offset incoming packets by two bytes to put the IP header on a 4-byte boundary */
	eth_write(base, ETH_RX_CFG_OFFSET, (2 << 8));
	/* enable TX checksum offload engine */
	reg = eth_mac_read(base, ETH_MAC_COE);
	reg |= ETH_MAC_COE_TXCOE;
	eth_mac_write(base, ETH_MAC_COE, reg);
	/* use 8KB TX FIFO */
	eth_write(base, ETH_HW_CFG_OFFSET, ETH_HW_CFG_MBO | (8 << 16));
	eth_write(base, ETH_TX_CFG_OFFSET, ETH_TX_CFG_ON);
	eth_mac_write(base, ETH_MAC_MAC_CR, ETH_MAC_TXEN | ETH_MAC_RXEN);

	return 0;
}

int eth_init() {
	int res = eth_dev_init(ETH1_BASE);
	if(res < 0)
		return res;

	eth_write(ETH1_BASE, ETH_INT_EN_OFFSET, 0);
	eth_write(ETH1_BASE, ETH_INT_STS_OFFSET, 0xFFFFFFFF);

	/* delay for 0.3s to wait for the stagecoach ks8999 switch */
	udelay(300000);

	return 0;
}

void eth_set_rxlevel(int level) {
	int reg = read32(ETH1_BASE + ETH_FIFO_INT_OFFSET);
	reg = (reg & 0xFFFFFF00) | level;
	write32(ETH1_BASE + ETH_FIFO_INT_OFFSET, reg);
}

void eth_set_txlevel(int level) {
	int reg = read32(ETH1_BASE + ETH_FIFO_INT_OFFSET);
	reg = (reg & 0xFF00FFFF) | (level << 16);
	write32(ETH1_BASE + ETH_FIFO_INT_OFFSET, reg);
}

int eth_intstatus() {
	return read32(ETH1_BASE + ETH_INT_STS_OFFSET);
}

void eth_intenable(int interrupt) {
	mem32(ETH1_BASE + ETH_IRQ_CFG_OFFSET) |= ETH_IRQ_EN;
	mem32(ETH1_BASE + ETH_INT_EN_OFFSET) |= interrupt;
}

void eth_intreset(int interrupt) {
	mem32(ETH1_BASE + ETH_INT_STS_OFFSET) = interrupt;
}

void eth_intdisable(int interrupt) {
	mem32(ETH1_BASE + ETH_INT_EN_OFFSET) &= ~interrupt;
}

/* Using a struct allows GCC to generate stmfd/ldmfd calls for transferring data
   to/from the extra memory-mapped FIFO ports */
struct fifobuf {
	uint32_t data[8];
};

static void eth_tx_wait_ready(int base, int ndw) {
	while((eth_read(base, ETH_TX_FIFO_INF_OFFSET) & 0xffff) < (ndw << 2))
		;
}

void eth_tx_coe(int base, uint32_t start_offset, uint32_t checksum_offset, uint32_t btag) {
	eth_tx_wait_ready(base, 3);
	// first = 1, last = 0
	eth_write(base, ETH_TX_FIFO_OFFSET, (0 << 16) | (1 << 13) | (0 << 12) | 4);
	eth_write(base, ETH_TX_FIFO_OFFSET, btag);
	eth_write(base, ETH_TX_FIFO_OFFSET, (checksum_offset << 16) | start_offset);
}

static void eth_tx_aligned(int base, const uint32_t *buf, uint16_t offset, uint16_t nbytes, int first, int last, uint32_t btag) {
	int ndw = (nbytes+offset+3)>>2;

	eth_tx_wait_ready(base, 2);
	eth_write(base, ETH_TX_FIFO_OFFSET, (offset << 16) | (first << 13) | (last << 12) | nbytes);
	eth_write(base, ETH_TX_FIFO_OFFSET, btag);

	while(ndw > 8) {
		eth_tx_wait_ready(base, 8);
		*(struct fifobuf *)(base + ETH_TX_FIFO_OFFSET) = *(struct fifobuf *)(buf);
		buf+=8; ndw-=8;
	}
	eth_tx_wait_ready(base, ndw);
	while(ndw-->0)
		eth_write(base, ETH_TX_FIFO_OFFSET, *buf++);
}

void eth_tx(int base, const void *buf, uint16_t nbytes, int first, int last, uint32_t btag) {
	eth_tx_aligned(base, (uint32_t *)((uint32_t)buf & ~3), (uint32_t)buf & 3, nbytes, first, last, btag);
}

void eth_rx(int base, uint32_t *buf, uint16_t nbytes) {
	int ndw = (nbytes+3)>>2;

	while(ndw > 8) {
		*(struct fifobuf *)(buf) = *(struct fifobuf *)(base + ETH_RX_FIFO_OFFSET);
		buf+=8; ndw-=8;
	}
	while(ndw-->0)
		*buf++ = eth_read(base, ETH_RX_FIFO_OFFSET);
}

uint32_t eth_rx_wait_sts(int base) {
	while((eth_read(base, ETH_RX_FIFO_INF_OFFSET) & 0x00ff0000) == 0x00)
		;

	return eth_read(base, ETH_RX_STS_FIFO_OFFSET);
}

uint32_t eth_tx_wait_sts(int base) {
	while((eth_read(base, ETH_TX_FIFO_INF_OFFSET) & 0x00ff0000) == 0x00)
		;

	return eth_read(base, ETH_TX_STS_FIFO_OFFSET);
}
