#include <lib.h>
#include <string.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <task.h>
#include <eth.h>
#include <ip.h>
#include <machine.h>
#include <drivers/eth.h>
#include <servers/clock.h>
#include <servers/net.h>

static int ethrx_tid; // general Ethernet receiver
static int arpserver_tid; // ARP server (MAC cache, ARP responder)
static int udprx_tid; // UDP receiver server
static int udpconrx_tid; // UDP-based console receiver

#define UDPCON_CLIENT_PORT 26845
#define UDPCON_SERVER_PORT 26846
#define UDPCON_SERVER_IP IP(10,0,0,1)
#define GATEWAY_IP IP(10,0,0,1)
#define SUBNET_MASK IP(255,0,0,0)

enum netmsg {
	ETH_RX_NOTIFY_MSG,

	ARP_DISPATCH_MSG,
	ARP_QUERY_MSG,

	UDP_RX_DISPATCH_MSG,
	UDP_RX_BIND_MSG,
	UDP_RX_REQ_MSG,
	UDP_RX_RELEASE_MSG,

	UDPCON_RX_NOTIFY_MSG,
	UDPCON_RX_REQ_MSG,
};

uint16_t ip_checksum(const uint8_t *data, uint16_t len) {
	uint32_t sum = 0;
	int i;
	for(i=0; i<len; i+=2) {
		sum += ((uint16_t)(data[i]) << 8) + data[i+1];
	}
	return ~((sum & 0xffff) + (sum >> 16));
}

int arp_lookup(uint32_t ip_addr, mac_addr_t *mac_addr, int timeout_msec) {
	if((ip_addr & SUBNET_MASK) != (GATEWAY_IP & SUBNET_MASK))
		return arp_lookup(GATEWAY_IP, mac_addr, timeout_msec);
	int count = 0;
	while(1) {
		int res = Send(arpserver_tid, ARP_QUERY_MSG, &ip_addr, sizeof(uint32_t), &mac_addr, sizeof(mac_addr_t));
		if(res == ERR_ARP_PENDING) {
			if(count >= timeout_msec) {
				return ERR_ARP_TIMEOUT;
			} else {
				msleep(10);
				count += 10;
			}
		} else {
			return res;
		}
	}
}

static int send_arp_req(uint32_t addr) {
	struct ethhdr eth;
	struct arppkt arp;

	memset(&eth.dest, 0xff, 6);
	eth.src = my_mac;
	eth.ethertype = htons(ET_ARP);

	arp.arp_htype = htons(ARP_HTYPE_ETH);
	arp.arp_ptype = htons(ET_IPV4);
	arp.arp_hlen = 6;
	arp.arp_plen = 4;
	arp.arp_oper = htons(ARP_OPER_REQUEST);
	arp.arp_sha = my_mac;
	arp.arp_spa = htonl(my_ip);
	memset(&arp.arp_tha, 0x00, 6);
	arp.arp_tpa = htonl(addr);

	uint32_t btag = MAKE_BTAG(0xcafe, sizeof(eth) + sizeof(arp));

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &arp, sizeof(arp), 0, 1, btag);

	uint32_t sts = eth_tx_wait_sts(ETH1_BASE);
	if(sts >> 16 != 0xcafe)
		return -1;
	if(sts & ETH_TX_STS_ERROR)
		return -(sts & ETH_TX_STS_ERROR);

	return 0;
}

static int send_arp_reply(uint32_t addr, mac_addr_t macaddr) {
	struct ethhdr eth;
	struct arppkt arp;

	eth.dest = macaddr;
	eth.src = my_mac;
	eth.ethertype = htons(ET_ARP);

	arp.arp_htype = htons(ARP_HTYPE_ETH);
	arp.arp_ptype = htons(ET_IPV4);
	arp.arp_hlen = 6;
	arp.arp_plen = 4;
	arp.arp_oper = htons(ARP_OPER_REPLY);
	arp.arp_sha = my_mac;
	arp.arp_spa = htonl(my_ip);
	arp.arp_tha = macaddr;
	arp.arp_tpa = htonl(addr);

	uint32_t btag = MAKE_BTAG(0xcaff, sizeof(eth) + sizeof(arp));

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &arp, sizeof(arp), 0, 1, btag);

	uint32_t sts = eth_tx_wait_sts(ETH1_BASE);
	if(sts >> 16 != 0xcaff)
		return -1;
	if(sts & ETH_TX_STS_ERROR)
		return -(sts & ETH_TX_STS_ERROR);

	return 0;
}

int send_udp(uint16_t srcport, uint32_t addr, uint16_t dstport, const char *data, uint16_t len) {
	struct ethhdr eth;
	struct ip ip;
	struct udphdr udp;

	if(len > UDPMTU)
		return -1;

	int res = arp_lookup(addr, &eth.dest, 100);
	if(res < 0)
		return res;

	eth.src = my_mac;
	eth.ethertype = htons(ET_IPV4);

	ip.ip_vhl = IP_VHL_BORING;
	ip.ip_tos = 0;
	ip.ip_len = htons(sizeof(struct ip) + sizeof(struct udphdr) + len);
	ip.ip_id = htons(0);
	ip.ip_off = htons(0);
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_UDP;
	ip.ip_sum = 0;
	ip.ip_src.s_addr = htonl(my_ip);
	ip.ip_dst.s_addr = htonl(addr);
	ip.ip_sum = htons(ip_checksum((uint8_t *)&ip, sizeof(struct ip)));

	udp.uh_sport = htons(srcport);
	udp.uh_dport = htons(dstport);
	udp.uh_ulen = htons(sizeof(struct udphdr) + len);
	udp.uh_sum = htons(0); // be lazy and don't bother computing a checksum

	uint32_t btag = MAKE_BTAG(0xbeef, sizeof(eth) + sizeof(ip) + sizeof(udp) + len);

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &ip, sizeof(ip), 0, 0, btag);
	eth_tx(ETH1_BASE, &udp, sizeof(udp), 0, 0, btag);
	eth_tx(ETH1_BASE, data, len, 0, 1, btag);

	uint32_t sts = eth_tx_wait_sts(ETH1_BASE);
	if(sts >> 16 != 0xbeef)
		return -1;
	if(sts & ETH_TX_STS_ERROR)
		return -(sts & ETH_TX_STS_ERROR);

	return 0;
}

static void udp_printfunc(void *data, const char *buf, size_t len) {
	for(int i=0; i<len; i+=UDPMTU) {
		int chunk = (i+UDPMTU < len) ? UDPMTU : len-i;
		send_udp(UDPCON_CLIENT_PORT, UDPCON_SERVER_IP, UDPCON_SERVER_PORT, buf+i, chunk);
	}
}

int udp_vprintf(const char *fmt, va_list va) {
	return func_vprintf(udp_printfunc, NULL, fmt, va);
}

int udp_printf(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(udp_printfunc, NULL, fmt, va);
	va_end(va);
	return ret;
}

static void ethrx_task() {
	/* TODO */
	int tid, rcvlen, msgcode;

	while(1) {
		rcvlen = Receive(&tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case ETH_RX_NOTIFY_MSG:
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			continue;
		}
	}
}

static void ethrx_notifier() {
	while(1) {
		AwaitEvent(EVENT_ETH_RECEIVE);
		Send(ethrx_tid, ETH_RX_NOTIFY_MSG, NULL, 0, NULL, 0);
	}
}

static void arpserver_task() {
	/* TODO */
	int tid, rcvlen, msgcode;

	while(1) {
		rcvlen = Receive(&tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case ARP_DISPATCH_MSG:
		case ARP_QUERY_MSG:
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			continue;
		}
	}
}

static void udprx_task() {
	/* TODO */
	int tid, rcvlen, msgcode;

	while(1) {
		rcvlen = Receive(&tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case UDP_RX_DISPATCH_MSG:
		case UDP_RX_BIND_MSG:
		case UDP_RX_REQ_MSG:
		case UDP_RX_RELEASE_MSG:
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			continue;
		}
	}
}

static void udpconrx_task() {
	/* TODO */
	int tid, rcvlen, msgcode;

	while(1) {
		rcvlen = Receive(&tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case UDPCON_RX_NOTIFY_MSG:
		case UDPCON_RX_REQ_MSG:
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			continue;
		}
	}
}

static void udpconrx_notifier() {
	/* TODO */
	/*
	Bind port UDPCON_CLIENT_PORT
	while(1) {
		Request udprx_tid
		Notify udpconrx_tid
	}
	*/
}

/* reserve_tids and start_tasks are called as part of kernel initialization */
void net_reserve_tids() {
	ethrx_tid = reserve_tid();
	arpserver_tid = reserve_tid();
	udprx_tid = reserve_tid();
	udpconrx_tid = reserve_tid();
}

void net_start_tasks() {
	KernCreateTask(1, ethrx_task, TASK_DAEMON, ethrx_tid);
	KernCreateTask(1, arpserver_task, TASK_DAEMON, arpserver_tid);
	KernCreateTask(1, udprx_task, TASK_DAEMON, udprx_tid);
	KernCreateTask(2, udpconrx_task, TASK_DAEMON, udpconrx_tid);
	KernCreateTask(0, ethrx_notifier, TASK_DAEMON, TID_AUTO);
	KernCreateTask(1, udpconrx_notifier, TASK_DAEMON, TID_AUTO);
}
