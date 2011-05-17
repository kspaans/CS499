#include "io.h"
#include "lib.h"
#include "syscall.h"
#include "interrupt.h"
#include "errno.h"
#include "task.h"
#include "ip.h"
#include "eth.h"
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

static uint16_t ip_checksum(uint8_t *data, uint16_t len) {
	uint32_t sum = 0;
	int i;
	for(i=0; i<len; i+=2) {
		sum += ((uint16_t)(data[i]) << 8) + data[i+1];
	}
	return ~((sum & 0xffff) + (sum >> 16));
}

static mac_addr_t arp_lookup(uint32_t addr) {
	struct ethhdr eth;
	struct arppkt arp;

	memcpy(eth.preamble, "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xab", 8);
	memset(&eth.dest, 0xff, 6);
	eth.src = eth_mac_addr(ETH1_BASE);
	eth.ethertype = SWAP16(ET_ARP);

	arp.arp_htype = SWAP16(ARP_HTYPE_ETH);
	arp.arp_ptype = SWAP16(ET_IPV4);
	arp.arp_hlen = 6;
	arp.arp_plen = 4;
	arp.arp_sha = eth.src;
	arp.arp_spa = SWAP32(0x0a00000b);
	arp.arp_tpa = SWAP32(addr);

	uint32_t btag = MAKE_BTAG(0xcafe, sizeof(eth) + sizeof(arp));

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &arp, sizeof(arp), 0, 1, btag);

	printk("ARP send status 0x%08x\n", eth_tx_wait_sts(ETH1_BASE));

	uint8_t buf[2048];
	uint32_t sts = eth_rx_wait_sts(ETH1_BASE);
	uint32_t len = (sts >> 16) & 0x3fff;
	printk("ARP recv status 0x%08x (len %d)\n", sts, len);
	eth_rx(ETH1_BASE, (uint32_t *)buf, len);
	printk("packet:");
	for(int i=0; i<len; i++) {
		printk(" %02x", buf[i]);
	}
	printk("\n");
	return arp.arp_sha;
}

static uint32_t send_udp(uint32_t addr, uint16_t port, char *data, uint16_t len) {
	struct ethhdr eth;
	struct ip ip;
	struct udphdr udp;

	memcpy(eth.preamble, "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xab", 8);
	/* ??? */
	eth.ethertype = SWAP16(0x0800);

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

	uint32_t btag = MAKE_BTAG(0xbeef, sizeof(eth) + sizeof(ip) + sizeof(udp) + len);

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &ip, sizeof(ip), 0, 0, btag);
	eth_tx(ETH1_BASE, &udp, sizeof(udp), 0, 0, btag);
	eth_tx(ETH1_BASE, data, len, 0, 1, btag);

	return eth_tx_wait_sts(ETH1_BASE);
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

	eth_init(ETH1_BASE);
	arp_lookup(0x0a000000);
	printk("UDP STATUS: %08x\n", send_udp(0xc0a80177, 12345, "abcd", 4));

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
