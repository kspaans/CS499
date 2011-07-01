#include <lib.h>
#include <string.h>
#include <errno.h>
#include <event.h>
#include <hashtable.h>
#include <syscall.h>
#include <task.h>
#include <msg.h>

#include <eth.h>
#include <ip.h>
#include <machine.h>
#include <drivers/eth.h>
#include <mem.h>
#include <panic.h>

#include <servers/console.h>
#include <servers/clock.h>
#include <servers/net.h>
#include <servers/fs.h>

/* TODO Use userspace malloc (once implemented) */
#include <kern/kmalloc.h>
#define malloc kmalloc

/* Important points to keep in mind:

1) The network chip has been instructed to add 2 bytes of padding to the start
   of all received packets. The purpose of this padding is to force headers
   after the ethernet header to be DWORD aligned, which improves performance,
   at the expense of a bit of clarity.
   This setting can be changed in the RX_CFG register.
*/

#define RXPAD 2 /* set in drivers/eth.c with RX_CFG */
#define FRAME_MAX 1600

#define UDPCON_CLIENT_PORT (this_host->ncport)
#define UDPCON_SERVER_PORT (this_host->ncport)
#define UDPCON_SERVER_IP (this_host->ncip)
#define GATEWAY_IP (this_host->gwip)
#define SUBNET_MASK (this_host->netmask)

static void ethrx_notifier(void);
static void udpconrx_notifier(void);

static struct hostdata hosts[] = {
	{ "tobi",   MAC(0x00, 0x15, 0xc9, 0x28, 0xd9, 0x1b), IP(10, 0, 0, 10), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6010, 1 },
	{ "tide1",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xc0), IP(10, 0, 0, 11), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6011, 1 },
	{ "earth1", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x53), IP(10, 0, 0, 12), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6012, 1 },
	{ "tide2",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xbf), IP(10, 0, 0, 13), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6013, 0 },
	{ "earth2", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x62), IP(10, 0, 0, 14), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6014, 0 },
	{ "tide3",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xbe), IP(10, 0, 0, 15), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6015, 0 },
	{ "earth3", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x25), IP(10, 0, 0, 16), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6016, 0 },
	{ "tide4",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xcf), IP(10, 0, 0, 17), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6017, 0 },
	{ "earth4", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x34), IP(10, 0, 0, 18), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6018, 0 },
	{ "tide5",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xce), IP(10, 0, 0, 19), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6019, 0 },
	{ "earth5", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x24), IP(10, 0, 0, 20), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6020, 0 },
	{ "tide6",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xde), IP(10, 0, 0, 21), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6021, 0 },
	{ "earth6", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x33), IP(10, 0, 0, 22), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6022, 0 },
	{ "tide7",  MAC(0x00, 0x15, 0xc9, 0x28, 0xe1, 0xdd), IP(10, 0, 0, 23), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6023, 0 },
	{ "earth7", MAC(0x00, 0x15, 0xc9, 0x28, 0xdf, 0x43), IP(10, 0, 0, 24), IP(255, 255, 255, 0), IP(10, 0, 0, 1), IP(10, 0, 0, 1), 6024, 0 },
};

struct hostdata *this_host;

enum netmsg {
	ETH_RX_NOTIFY_MSG = PRIVATE_MSG_START,

	ICMP_DISPATCH_MSG,

	ARP_DISPATCH_MSG,
	ARP_QUERY_MSG,

	UDP_RX_DISPATCH_MSG,
	UDP_RX_BIND_MSG,
	UDP_RX_REQ_MSG,
	UDP_RX_RELEASE_MSG,

	UDPCON_RX_NOTIFY_MSG,
};

static uint16_t ip_checksum(const uint8_t *data, uint16_t len) {
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
		int res = sendpath("/services/arp", ARP_QUERY_MSG, &ip_addr, sizeof(uint32_t), mac_addr, sizeof(mac_addr_t));
		if(res == EARP_PENDING) {
			if(count >= timeout_msec) {
				return EARP_TIMEOUT;
			} else {
				msleep(50);
				count += 50;
			}
		} else {
			return res;
		}
	}
}

static int tx_wait_sts(uint32_t btag) {
	uint32_t sts = eth_tx_wait_sts(ETH1_BASE);
	if((sts >> 16) != (btag >> 16))
		return EINVAL;
	if(sts & ETH_TX_STS_ERROR)
		return -(sts & ETH_TX_STS_ERROR) - 0x10000;
	return 0;
}

static int send_arp_request(uint32_t addr) {
	struct ethhdr eth;
	struct arppkt arp;

	memset(&eth.dest, 0xff, 6);
	eth.src = this_host->mac;
	eth.ethertype = htons(ET_ARP);

	arp.arp_htype = htons(ARP_HTYPE_ETH);
	arp.arp_ptype = htons(ET_IPV4);
	arp.arp_hlen = 6;
	arp.arp_plen = 4;
	arp.arp_oper = htons(ARP_OPER_REQUEST);
	arp.arp_sha = this_host->mac;
	arp.arp_spa = htonl(this_host->ip);
	memset(&arp.arp_tha, 0x00, 6);
	arp.arp_tpa = htonl(addr);

	uint32_t btag = MAKE_BTAG(0xcafe, sizeof(eth) + sizeof(arp));

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &arp, sizeof(arp), 0, 1, btag);

	return tx_wait_sts(btag);
}

static int send_arp_reply(uint32_t addr, mac_addr_t macaddr) {
	struct ethhdr eth;
	struct arppkt arp;

	eth.dest = macaddr;
	eth.src = this_host->mac;
	eth.ethertype = htons(ET_ARP);

	arp.arp_htype = htons(ARP_HTYPE_ETH);
	arp.arp_ptype = htons(ET_IPV4);
	arp.arp_hlen = 6;
	arp.arp_plen = 4;
	arp.arp_oper = htons(ARP_OPER_REPLY);
	arp.arp_sha = this_host->mac;
	arp.arp_spa = htonl(this_host->ip);
	arp.arp_tha = macaddr;
	arp.arp_tpa = htonl(addr);

	uint32_t btag = MAKE_BTAG(0xcaff, sizeof(eth) + sizeof(arp));

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &arp, sizeof(arp), 0, 1, btag);

	return tx_wait_sts(btag);
}

int send_udp(uint16_t srcport, uint32_t addr, uint16_t dstport, const char *data, uint16_t len) {
	struct ethhdr eth;
	struct ip ip;
	struct udphdr udp;

	if(len > UDPMTU)
		return EINVAL;

	int res = arp_lookup(addr, &eth.dest, 100);
	if(res < 0)
		return res;

	eth.src = this_host->mac;
	eth.ethertype = htons(ET_IPV4);

	ip.ip_vhl = IP_VHL_BORING;
	ip.ip_tos = 0;
	ip.ip_len = htons(sizeof(struct ip) + sizeof(struct udphdr) + len);
	ip.ip_id = htons(0);
	ip.ip_off = htons(0);
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_UDP;
	ip.ip_sum = 0;
	ip.ip_src.s_addr = htonl(this_host->ip);
	ip.ip_dst.s_addr = htonl(addr);
	ip.ip_sum = htons(ip_checksum((uint8_t *)&ip, sizeof(struct ip)));

	udp.uh_sport = htons(srcport);
	udp.uh_dport = htons(dstport);
	udp.uh_ulen = htons(sizeof(struct udphdr) + len);
	udp.uh_sum = htons(0); // be lazy and don't bother computing a checksum

	uint32_t btag = MAKE_BTAG(0xbeef, sizeof(eth) + sizeof(ip) + sizeof(udp) + len);

	int has_data = (data && len);

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &ip, sizeof(ip), 0, 0, btag);
	eth_tx(ETH1_BASE, &udp, sizeof(udp), 0, !has_data, btag);
	if(has_data)
		eth_tx(ETH1_BASE, data, len, 0, 1, btag);

	return tx_wait_sts(btag);
}

int udp_bind(uint16_t port) {
	return sendpath("/services/udprx", UDP_RX_BIND_MSG, &port, sizeof(port), NULL, 0);
}

int udp_release(uint16_t port) {
	return sendpath("/services/udprx", UDP_RX_RELEASE_MSG, &port, sizeof(port), NULL, 0);
}

int udp_wait(uint16_t port, struct packet_rec *rec, int recsize) {
	return sendpath("/services/udprx", UDP_RX_REQ_MSG, &port, sizeof(port), rec, recsize);
}

static void ethrx_dispatch(uint32_t sts) {
	union msg {
		struct {
			char padding[RXPAD];
			struct ethhdr eth;
			struct ip ip;
		} __attribute__((packed)) pkt;
		char raw[FRAME_MAX];
	} __attribute__((aligned(4))) frame;
	uint32_t len = ((sts >> 16) & 0x3fff)+RXPAD;
	eth_rx(ETH1_BASE, (uint32_t *)&frame, len);
	len -= 4; // remove crc at end

	switch(ntohs(frame.pkt.eth.ethertype)) {
	case ET_ARP:
		sendpath("/services/arp", ARP_DISPATCH_MSG, &frame.pkt.ip, sizeof(struct arppkt), NULL, 0);
		break;
	case ET_IPV4:
		switch(frame.pkt.ip.ip_p) {
		case IPPROTO_ICMP:
			sendpath("/services/icmp", ICMP_DISPATCH_MSG, &frame, len, NULL, 0);
			break;
		case IPPROTO_UDP:
			sendpath("/services/udprx", UDP_RX_DISPATCH_MSG, &frame, len, NULL, 0);
			break;
		default:
			printf("unknown IP proto %d\n", frame.pkt.ip.ip_p);
			break;
		}
		break;
	default:
		printf("unknown ethertype %d\n", ntohs(frame.pkt.eth.ethertype));
		break;
	}
}

void ethrx_task(void) {
	int tid, rcvlen, msgcode;

	int ethrx_fd = mkopenchan("/services/ethrx");

	spawn(0, ethrx_notifier, SPAWN_DAEMON);

	while(1) {
		rcvlen = MsgReceive(ethrx_fd, &tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case ETH_RX_NOTIFY_MSG:
			MsgReplyStatus(tid, 0);
			while(read32(ETH1_BASE + ETH_RX_FIFO_INF_OFFSET) & 0x00ff0000) {
				ethrx_dispatch(read32(ETH1_BASE + ETH_RX_STS_FIFO_OFFSET));
			}
			eth_intenable(ETH_INT_RSFL);
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
	}
}

static void ethrx_notifier(void) {
	int ethrx_fd = xopen(ROOT_DIRFD, "/services/ethrx");
	while(1) {
		waitevent(EVENT_ETH_RECEIVE);
		MsgSend(ethrx_fd, ETH_RX_NOTIFY_MSG, NULL, 0, NULL, 0, NULL);
	}
}

void icmpserver_task(void) {
	union msg {
		struct {
			char padding[RXPAD];
			struct ethhdr eth;
			struct ip ip;
			struct icmphdr icmp;
		} __attribute__((packed)) pkt;
		char raw[FRAME_MAX];
	} msg;
	int tid, rcvlen, msgcode;

	int icmpserver_fd = mkopenchan("/services/icmp");

	while(1) {
		rcvlen = MsgReceive(icmpserver_fd, &tid, &msgcode, &msg, sizeof(msg));
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case ICMP_DISPATCH_MSG:
			MsgReplyStatus(tid, 0);
			if(msg.pkt.icmp.icmp_type == ICMP_TYPE_ECHO_REQ) {
				int len = ntohs(msg.pkt.ip.ip_len) + sizeof(struct ethhdr);
				msg.pkt.icmp.icmp_type = ICMP_TYPE_ECHO_REPLY;
				msg.pkt.eth.dest = msg.pkt.eth.src;
				msg.pkt.eth.src = this_host->mac;
				msg.pkt.ip.ip_dst = msg.pkt.ip.ip_src;
				msg.pkt.ip.ip_src.s_addr = htonl(this_host->ip);
				msg.pkt.ip.ip_sum = 0;
				msg.pkt.ip.ip_sum = htons(ip_checksum((uint8_t *)&msg.pkt.ip, sizeof(struct ip)));
				uint32_t btag = MAKE_COE_BTAG(0x1c1c, len);
				msg.pkt.icmp.icmp_sum = 0;
				// Compute checksum starting at ICMP header and continuing
				// to the end of the packet. Store checksum at icmp_sum.
				eth_tx_coe(ETH1_BASE, offsetof(union msg, pkt.icmp)-RXPAD,
					offsetof(union msg, pkt.icmp.icmp_sum)-RXPAD, btag);
				eth_tx(ETH1_BASE, &msg.pkt.eth, len, 0, 1, btag);
				tx_wait_sts(btag);
			} else {
				printf("icmp unknown function %d.%d\n", msg.pkt.icmp.icmp_type, msg.pkt.icmp.icmp_code);
			}
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
	}
}

static void arpserver_store(mac_addr_t *mac_arr, intqueue *ipq, hashtable *addrmap, uint32_t addr, mac_addr_t macaddr) {
	mac_addr_t *loc;
	int i = hashtable_reserve(addrmap, addr);
	if(i < 0) {
		panic("ran out of space in arpserver");
		return;
	} else if(active_ht_item(&addrmap->arr[i])) {
		/* update existing entry */
		loc = addrmap->arr[i].value;
		*loc = macaddr;
		return;
	}

	if(intqueue_full(ipq)) {
		/* free up an old entry */
		int old_addr = intqueue_front(ipq);
		int old_idx = hashtable_get(addrmap, old_addr);
		if(old_idx < 0)
			return;
		loc = addrmap->arr[old_idx].value;
		delete_ht_item(&addrmap->arr[old_idx]);
		intqueue_pop(ipq);
	} else {
		/* allocate new entry */
		loc = mac_arr + ipq->len;
	}

	*loc = macaddr;
	addrmap->arr[i].intkey = addr;
	addrmap->arr[i].value = loc;
	activate_ht_item(&addrmap->arr[i]);
	intqueue_push(ipq, addr);
}

void arpserver_task(void) {
	union msg {
		struct arppkt pkt;
		uint32_t addr;
	} msg;
	int tid, rcvlen, msgcode;

	mac_addr_t mac_arr[512];
	intqueue ipq;
	int ipq_arr[512];
	intqueue_init(&ipq, ipq_arr, arraysize(ipq_arr));
	int idx;

	hashtable addrmap;
	struct ht_item addrmap_arr[1024];
	hashtable_init(&addrmap, addrmap_arr, arraysize(addrmap_arr), NULL, NULL);

	int arpserver_fd = mkopenchan("/services/arp");

	while(1) {
		rcvlen = MsgReceive(arpserver_fd, &tid, &msgcode, &msg, sizeof(msg));
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case ARP_DISPATCH_MSG:
			MsgReplyStatus(tid, 0);
			arpserver_store(mac_arr, &ipq, &addrmap, ntohl(msg.pkt.arp_spa), msg.pkt.arp_sha);
			if(ntohs(msg.pkt.arp_oper) == ARP_OPER_REQUEST
				&& ntohl(msg.pkt.arp_tpa) == this_host->ip) {
				send_arp_reply(ntohl(msg.pkt.arp_spa), msg.pkt.arp_sha);
			}
			break;
		case ARP_QUERY_MSG:
			idx = hashtable_get(&addrmap, msg.addr);
			if(idx < 0) {
				send_arp_request(msg.addr);
				MsgReplyStatus(tid, EARP_PENDING);
			} else {
				MsgReply(tid, 0, addrmap_arr[idx].value, sizeof(mac_addr_t), -1);
			}
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
	}
}

#define UDP_BUF_MAX 65536

struct packet_rec_buf {
	union {
		char buf[UDP_BUF_MAX];
		uint32_t bufu[UDP_BUF_MAX / 4]; // alignment
	};
	size_t head, tail, top;
	struct packet_rec_buf *next_free;
	int tid;
};

/* align record to 4 byte boundary */
static size_t calc_rec_size(size_t data_len) {
	return ((data_len + sizeof(struct packet_rec)) + 3) & (~3);
}

static struct packet_rec *pop_pkt(struct packet_rec_buf *buf) {
	/* Pop the packet record at the head */
	if(buf->head == buf->tail) {
		/* buffer is empty */
		return NULL;
	}

	struct packet_rec *pkt = (struct packet_rec *)(buf->bufu + (buf->head / 4));
	buf->head += calc_rec_size(pkt->data_len);
	if(buf->head == buf->top) {
		/* wraparound */
		buf->head = 0;
	}
	return pkt;
}

static struct packet_rec *alloc_pkt(struct packet_rec_buf *buf, size_t rec_size) {
	/* Allocate a packet record at the tail */
	if(buf->head == buf->tail) {
		buf->head = buf->tail = buf->top = 0;
	}
	if(rec_size >= UDP_BUF_MAX)
		return NULL;
	if(buf->tail + rec_size >= UDP_BUF_MAX) {
		/* wraparound */
		buf->top = buf->tail;
		if(buf->head == 0)
			pop_pkt(buf);
		buf->tail = 0;
	}
	while(buf->tail < buf->head && buf->tail + rec_size >= buf->head)
		pop_pkt(buf);

	struct packet_rec *pkt = (struct packet_rec *)(buf->bufu + (buf->tail / 4));
	buf->tail += rec_size;
	return pkt;
}

void udprx_task(void) {
	union msg {
		struct {
			char padding[RXPAD];
			struct ethhdr eth;
			struct ip ip;
			struct udphdr udp;
			char data[];
		} __attribute__((packed)) pkt;
		uint16_t port;
		char raw[FRAME_MAX];
	} msg;

	int tid, rcvlen, msgcode;

	int udprx_fd = mkopenchan("/services/udprx");

	struct packet_rec_buf *free_head = NULL;
	struct packet_rec_buf *buf;
	struct packet_rec *pkt;

	hashtable portmap;
	struct ht_item portmap_arr[512];
	hashtable_init(&portmap, portmap_arr, arraysize(portmap_arr), NULL, NULL);

	while(1) {
		rcvlen = MsgReceive(udprx_fd, &tid, &msgcode, &msg, sizeof(msg));
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case UDP_RX_DISPATCH_MSG: {
			MsgReplyStatus(tid, 0);
			int idx = hashtable_get(&portmap, (int)ntohs(msg.pkt.udp.uh_dport));
			if(idx < 0) {
				/* TODO: send ICMP port-unreachable message? */
				break;
			}
			buf = portmap_arr[idx].value;
			size_t datalen = ntohs(msg.pkt.udp.uh_ulen) - sizeof(struct udphdr);
			pkt = alloc_pkt(buf, calc_rec_size(datalen));
			if(pkt != NULL) {
				pkt->src_ip = ntohl(msg.pkt.ip.ip_src.s_addr);
				pkt->dst_ip = ntohl(msg.pkt.ip.ip_dst.s_addr);
				pkt->src_port = ntohs(msg.pkt.udp.uh_sport);
				pkt->dst_port = ntohs(msg.pkt.udp.uh_dport);
				pkt->data_len = datalen;
				memcpy(pkt->data, msg.pkt.data, datalen);
			}
			if(buf->tid != -1) {
				pkt = pop_pkt(buf);
				if(pkt != NULL) {
					int nbytes = calc_rec_size(pkt->data_len);
					MsgReply(buf->tid, nbytes, pkt, nbytes, -1);
					buf->tid = -1;
				}
			}
			break;
		}
		case UDP_RX_BIND_MSG: {
			int idx = hashtable_reserve(&portmap, (int)msg.port);
			if(idx < 0) {
				MsgReplyStatus(tid, ENOMEM);
				break;
			}
			if(active_ht_item(&portmap_arr[idx])) {
				MsgReplyStatus(tid, EEXIST);
				break;
			}

			if(free_head != NULL) {
				buf = free_head;
				free_head = buf->next_free;
			} else {
				buf = malloc(sizeof(*buf));
			}
			buf->head = buf->tail = buf->top = 0;
			buf->tid = -1;

			portmap_arr[idx].intkey = msg.port;
			portmap_arr[idx].value = buf;
			activate_ht_item(&portmap_arr[idx]);
			MsgReplyStatus(tid, 0);
			break;
		}
		case UDP_RX_RELEASE_MSG: {
			int idx = hashtable_get(&portmap, (int)msg.port);
			if(idx < 0) {
				MsgReplyStatus(tid, ENOENT);
				break;
			}
			buf = portmap_arr[idx].value;
			if(buf->tid != -1) {
				MsgReplyStatus(tid, EBUSY);
				break;
			}

			buf->next_free = free_head;
			free_head = buf;
			delete_ht_item(&portmap_arr[idx]);
			MsgReplyStatus(tid, 0);
			break;
		}
		case UDP_RX_REQ_MSG: {
			int idx = hashtable_get(&portmap, (int)msg.port);
			if(idx < 0) {
				MsgReplyStatus(tid, ENOENT);
				break;
			}
			buf = portmap_arr[idx].value;
			if(buf->tid != -1) {
				MsgReplyStatus(tid, EBUSY);
				break;
			}

			pkt = pop_pkt(buf);
			if(pkt != NULL) {
				int nbytes = calc_rec_size(pkt->data_len);
				MsgReply(tid, nbytes, pkt, nbytes, -1);
			} else {
				// block task until packet comes in
				buf->tid = tid;
			}
			break;
		}
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
	}
}

void udpcontx_task(void) {
	int tid, rcvlen, msgcode;
	char rcvbuf[PRINT_CHUNK];

	int udpcontx_fd = mkopenchan("/dev/netconout");

	while(1) {
		rcvlen = MsgReceive(udpcontx_fd, &tid, &msgcode, rcvbuf, sizeof(rcvbuf));
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case STDOUT_WRITE_MSG:
			MsgReplyStatus(tid, send_udp(UDPCON_CLIENT_PORT, UDPCON_SERVER_IP, UDPCON_SERVER_PORT, rcvbuf, rcvlen));
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
	}
}

#define RX_BUF_MAX 16384
#define RX_TIDS_MAX 64

void udpconrx_task(void) {
	int tid, rcvlen, msgcode;

	int udpconrx_fd = mkopenchan("/dev/netconin");

	spawn(0, udpconrx_notifier, SPAWN_DAEMON);

	char buf[FRAME_MAX];

	intqueue tidq;
	int tidq_arr[RX_TIDS_MAX];
	intqueue_init(&tidq, tidq_arr, RX_TIDS_MAX);

	charqueue chq;
	char chq_arr[RX_BUF_MAX];
	charqueue_init(&chq, chq_arr, RX_BUF_MAX);

	while(1) {
		rcvlen = MsgReceive(udpconrx_fd, &tid, &msgcode, buf, sizeof(buf));
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case UDPCON_RX_NOTIFY_MSG:
			MsgReplyStatus(tid, 0);
			for(int i=0; i<rcvlen; ++i) {
				if(charqueue_full(&chq))
					charqueue_pop(&chq);
				charqueue_push(&chq, buf[i]);
			}
			break;
		case STDIN_GETCHAR_MSG:
			if(intqueue_full(&tidq)) {
				MsgReplyStatus(tid, ENOMEM);
				continue;
			}
			intqueue_push(&tidq, tid);
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
		while(!intqueue_empty(&tidq) && !charqueue_empty(&chq))
			MsgReplyStatus(intqueue_pop(&tidq), charqueue_pop(&chq));
	}
}

static void udpconrx_notifier(void) {
	union {
		struct packet_rec rec;
		char pkt[FRAME_MAX];
	} reply;

	int netconin = open(ROOT_DIRFD, "/dev/netconin");

	ASSERTNOERR(udp_bind(UDPCON_CLIENT_PORT));
	while(1) {
		ASSERTNOERR(udp_wait(UDPCON_CLIENT_PORT, &reply.rec, sizeof(reply)));
		ASSERTNOERR(MsgSend(netconin, UDPCON_RX_NOTIFY_MSG, &reply.rec.data, reply.rec.data_len, NULL, 0, NULL));
	}
}

static struct hostdata *get_host_data(struct mac_addr *mac) {
	for (size_t i = 0; i < arraysize(hosts); ++i)
		if (!memcmp(mac, &hosts[i].mac, sizeof(*mac)))
			return &hosts[i];
	printf("no IP address for this host");
	exit();
}

struct hostdata *get_host_data_from_name(const char *name) {
	if(!strcmp(name, "localhost")) {
		return this_host;
	}
	for (size_t i = 0; i < arraysize(hosts); ++i) {
		if(!strcmp(hosts[i].hostname, name))
			return &hosts[i];
	}
	return NULL;
}

void net_init(void) {
	struct mac_addr mac = eth_mac_addr(ETH1_BASE);

	printf("net init\n");

	printf("  MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		mac.addr[0], mac.addr[1], mac.addr[2],
		mac.addr[3], mac.addr[4], mac.addr[5]);

	this_host = get_host_data(&mac);

	printf("  IP address: %d.%d.%d.%d\n",
		(this_host->ip >> 24) & 0xff,
		(this_host->ip >> 16) & 0xff,
		(this_host->ip >> 8)  & 0xff,
		(this_host->ip >> 0)  & 0xff);

	printf("  Host: %s\n", this_host->hostname);

	printf("  Console: %s%d.%d.%d.%d:%d\n",
		this_host->has_uart ? "uart, " : "",
		(this_host->ncip >> 24) & 0xff,
		(this_host->ncip >> 16) & 0xff,
		(this_host->ncip >> 8)  & 0xff,
		(this_host->ncip >> 0)  & 0xff,
		this_host->ncport);

	xspawn(1, ethrx_task, SPAWN_DAEMON);
	xspawn(1, icmpserver_task, SPAWN_DAEMON);
	xspawn(1, arpserver_task, SPAWN_DAEMON);
	xspawn(1, udprx_task, SPAWN_DAEMON);
	xspawn(2, udpconrx_task, SPAWN_DAEMON);
	xspawn(2, udpcontx_task, SPAWN_DAEMON);
}
