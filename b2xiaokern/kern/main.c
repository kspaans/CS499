#include "io.h"
#include "lib.h"
#include "syscall.h"
#include "interrupt.h"
#include "errno.h"
#include "task.h"
#include "ip.h"
#include "machine.h"

void idle() {
	while (1)
		Pass()/* IDLE TODO */;
}

static void memcpy_bench() {
	int tid = MyTid();
	printk("memcpy_bench[%d]: testing memcpy\n", tid);
	/* Test memcpy. */
	char magichands[128];
	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands+1, magichands+7, 6);
	printk("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+8, 8);
	printk("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+9, 9);
	printk("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+16, 7);
	printk("%s\n", magichands);
	sprintf(magichands, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abXXXXghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abc");
	memcpy(magichands, magichands+64, 64);
	printk("%s\n", magichands);

	printk("memcpy_bench[%d]: benchmarking memcpy\n", tid);
	/* Run some benchmarks! */
	char buf[1<<14];
	char buf2[1<<14];

	int i;
	unsigned long long start_time = read_timer();
	for(i=0; i<(1<<12); i++) {
		memcpy(buf, buf2, sizeof(buf));
	}
	unsigned long long duration = read_timer() - start_time;
	printk("Did 64MB in %lu milliseconds\n", (unsigned long)(duration/TICKS_PER_MSEC));
}

static void srr_child() {
	char buf[4];
	int i, j;
	for(i=0; i<9; i++) {
		int tid;
		int len = Receive(&tid, buf, sizeof(buf));
		printk("  Child Received, with retval %d\n", len);
		if(len < 0) {
			printk("RECEIVE FAILED: %d\n", len);
			Reply(tid, "FAILED", 6);
			continue;
		}
		if(len > sizeof(buf)) {
			printk("  Child got TRUNCATED Receive: ");
			for(j=0; j<sizeof(buf); j++)
				putchar(buf[j]);
		} else {
			printk("  Child got Receive: ");
			for(j=0; j<len; j++)
				putchar(buf[j]);
		}
		printk("\n");
		int ret = Reply(tid, "0123456789", i);
		if(ret < 0) {
			printk("  Child Reply FAILED: %d\n", ret);
		} else {
			printk("  Child Reply!\n");
		}
	}
}

static void srr_task() {
	int tid = MyTid();
	printk("srr_task[%d]: testing SRR transaction\n", tid);
	int child = Create(1, srr_child);
	char buf[4];
	int i, j;
	for(i=0; i<10; i++) {
		int len = Send(child, "abcdefghijklmno", i, buf, sizeof(buf));
		printk(" Parent Sent, with retval %d\n", len);
		if(len < 0) {
			printk(" Parent Send failed: %d\n", len);
			continue;
		}
		if(len > sizeof(buf)) {
			printk(" Parent got TRUNCATED Reply: ");
			for(j=0; j<sizeof(buf); j++)
				putchar(buf[j]);
		} else {
			printk(" Parent got Reply: ");
			for(j=0; j<len; j++)
				putchar(buf[j]);
		}
		printk("\n");
	}
}

#define SRR_RUNS 16384
static void srrbench_child() {
	char buf[1024];
	int tid;
	int i;
	for(i=0; i<SRR_RUNS; i++) {
		Receive(&tid, buf, 4);
		Reply(tid, buf, 4);
	}
	for(i=0; i<SRR_RUNS; i++) {
		Receive(&tid, buf, 64);
		Reply(tid, buf, 64);
	}
}

#define BENCH(name, code) { \
	printk("SRR Benchmarking: " name ": "); \
	start = read_timer(); \
	for(i=0; i<SRR_RUNS; i++) code; \
	elapsed = read_timer()-start; \
	printk("%d ms\n", (int)(elapsed/TICKS_PER_MSEC)); \
	printk("%d ns/loop\n", (int)(elapsed*1000000/TICKS_PER_MSEC/SRR_RUNS)); \
}

static void srrbench_task() {
	int tid = MyTid();
	printk("srrbench_task[%d]: benchmarking SRR transaction\n", tid);
	int child = Create(0, srrbench_child);
	char buf[512];
	int i;
	unsigned long long start, elapsed;

	BENCH("Pass", Pass())
	BENCH("4-bytes", Send(child, buf, 4, buf, 4))
	BENCH("64-bytes", Send(child, buf, 64, buf, 64))

	printk("srrbench_task[%d]: benchmark finished.\n", tid);
}

static void a2_init() {
	ASSERTNOERR(Create(0, memcpy_bench));
}

static uint32_t eth1_read(int offset) {
	return *(volatile int *)(ETH1_BASE + offset);
}

static void eth1_write(int offset, uint32_t value) {
	*(volatile int *)(ETH1_BASE + offset) = value;
}

static void eth1_mac_wait() {
	while(eth1_read(ETH_MAC_CSR_CMD_OFFSET) & ETH_MAC_CSR_BUSY)
		;
}

static uint32_t eth1_mac_read(int index) {
	eth1_mac_wait();
	eth1_write(ETH_MAC_CSR_CMD_OFFSET, ETH_MAC_CSR_BUSY | ETH_MAC_CSR_READ | index);
	eth1_mac_wait();
	return eth1_read(ETH_MAC_CSR_DATA_OFFSET);
}

static void eth1_mac_write(int index, uint32_t value) {
	eth1_mac_wait();
	eth1_write(ETH_MAC_CSR_DATA_OFFSET, value);
	eth1_write(ETH_MAC_CSR_CMD_OFFSET, ETH_MAC_CSR_BUSY | index);
	eth1_mac_wait();
}

static void eth1_phy_wait() {
	while(eth1_mac_read(ETH_MAC_MII_ACC) & ETH_MII_ACC_BUSY)
		;
}

static uint16_t eth1_phy_read(int index) {
	eth1_phy_wait();
	eth1_mac_write(ETH_MAC_MII_ACC, ETH_MII_ACC_PHY | (index << 6) | ETH_MII_ACC_BUSY);
	eth1_phy_wait();
	return eth1_mac_read(ETH_MAC_MII_DATA);
}

static void eth1_phy_write(int index, uint16_t value) {
	eth1_phy_wait();
	eth1_mac_write(ETH_MAC_MII_DATA, value);
	eth1_mac_write(ETH_MAC_MII_ACC, ETH_MII_ACC_PHY | (index << 6) | ETH_MII_ACC_BUSY | ETH_MII_ACC_WRITE);
	eth1_phy_wait();
}

static void init_eth1() {
	/* phy reset */
	uint32_t reg = eth1_read(ETH_PMT_CTRL_OFFSET);
	reg &= 0xFCF;
	reg |= (1<<10);
	eth1_write(ETH_PMT_CTRL_OFFSET, reg);
	while(eth1_read(ETH_PMT_CTRL_OFFSET) & (1<<10))
		;

	eth1_phy_write(ETH_MII_BCR, ETH_MII_BCR_RESET);
	while(eth1_phy_read(ETH_MII_BCR) & ETH_MII_BCR_RESET)
		;
	eth1_phy_write(ETH_MII_ADVERTISE, 0x01e1);
	eth1_phy_write(ETH_MII_BCR, ETH_MII_BCR_ANENABLE | ETH_MII_BCR_ANRESTART);
	while(!(eth1_phy_read(ETH_MII_BSR) & ETH_MII_BSR_LSTS))
		;

	printk("phy up\n");
	eth1_write(ETH_HW_CFG_OFFSET, 8 << 16 | 0x00100000);
	eth1_write(ETH_TX_CFG_OFFSET, ETH_TX_CFG_ON);
	eth1_mac_write(ETH_MAC_MAC_CR, ETH_MAC_TXEN | ETH_MAC_RXEN);
}

static void eth1_tx_aligned(uint32_t *buf, uint16_t offset, uint16_t nbytes, int first, int last, uint32_t btag) {
	volatile uint32_t *port = (uint32_t *)(ETH1_BASE + ETH_TX_FIFO_OFFSET);

	*port = ((offset << 16) | (first << 13) | (last << 12) | nbytes);
	*port = btag;
	int ndw = (nbytes+offset+3)>>2;
	printk("%08X %08X  ", ((offset << 16) | (first << 13) | (last << 12) | nbytes), btag);
	while(ndw-->0) {
		printk("%08x ", *buf);
		*port = *buf++;
	}
	printk("\n");
}

static void eth1_tx(void *buf, uint16_t nbytes, int first, int last, uint32_t btag) {
	eth1_tx_aligned((uint32_t *)((uint32_t)buf & ~3), (uint32_t)buf & 3, nbytes, first, last, btag);
}

static uint32_t make_btag(uint32_t tag, uint32_t len) {
	return (tag << 16) | (len & 0x7ff);
}

static uint16_t ip_checksum(uint8_t *data, uint16_t len) {
	uint32_t sum = 0;
	int i;
	for(i=0; i<len; i+=2) {
		sum += ((uint16_t)(data[i]) << 8) + data[i+1];
	}
	return ~((sum & 0xffff) + (sum >> 16));
}

static uint32_t eth1_sts() {
	volatile int *flags, *sts;

	flags = (int *)(ETH1_BASE + ETH_TX_FIFO_INF_OFFSET);
	sts = (int *)(ETH1_BASE + ETH_TX_STS_FIFO_OFFSET);

	while((*flags & 0x00ff0000) == 0x00)
		;

	return *sts;
}

static uint32_t send_udp(uint32_t addr, uint16_t port, char *data, uint16_t len) {
	struct ip ip;
	struct udphdr udp;

	ip.ip_vhl = IP_VHL_BORING;
	ip.ip_tos = 0;
	ip.ip_len = SWAP16(sizeof(struct ip) + sizeof(struct udphdr) + len);
	ip.ip_id = SWAP16(0);
	ip.ip_off = SWAP16(0);
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_UDP;
	ip.ip_sum = 0;
	ip.ip_src.s_addr = SWAP32(0x0a00000b);
	ip.ip_dst.s_addr = SWAP32(addr);
	ip.ip_sum = SWAP16(ip_checksum((uint8_t *)&ip, sizeof(struct ip)));

	udp.uh_sport = SWAP16(7777);
	udp.uh_dport = SWAP16(port);
	udp.uh_ulen = SWAP16(sizeof(struct udphdr) + len);
	udp.uh_sum = SWAP16(0);

	uint32_t btag = make_btag(0xbeef, ip.ip_len);

	eth1_tx(&ip, sizeof(struct ip), 1, 0, btag);
	eth1_tx(&udp, sizeof(struct udphdr), 0, 0, btag);
	eth1_tx(data, len, 0, 1, btag);

	return eth1_sts();
}

int main() {
	struct task *next;
	/* Start up hardware */
	init_timer();
	init_cache();

	init_interrupts();

	/* Initialize task queues */
	init_tasks();

	/* Initialize idle task */
	int idle_tid = syscall_CreateDaemon(NULL, 7, idle);

#ifdef SUPERVISOR_TASKS
	(void)idle_tid;
#else
	 // execute idle task in system mode, so that it can sleep the processor
	get_task(idle_tid)->regs.psr |= 0x1f;
#endif

	/* Initialize first user program */
	syscall_Create(NULL, 0, a2_init);

	init_eth1();
	printk("PACKET STATUS: %08x\n", send_udp(0xc0a80177, 12345, "abcd", 4));

	while (nondaemon_count > 0) {
		next = task_dequeue();
		if (next == NULL)
			/* No more tasks. */
			break;
		task_activate(next);
		task_enqueue(next);
		check_stack(next);
	}
	deinit_interrupts();
	printk("Done; we're outta here!\n");
	return 0;
}
