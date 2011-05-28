#include <machine.h>
#include <drivers/eth.h>
#include <types.h>
#include <eth.h>
#include <ip.h>
#include <string.h>

uint16_t ip_checksum(const uint8_t *data, uint16_t len) {
	uint32_t sum = 0;
	int i;
	for(i=0; i<len; i+=2) {
		sum += ((uint16_t)(data[i]) << 8) + data[i+1];
	}
	return ~((sum & 0xffff) + (sum >> 16));
}

mac_addr_t arp_lookup(uint32_t addr) {
	struct ethhdr eth;
	struct arppkt arp;

	memset(&eth.dest, 0xff, 6);
	eth.src = eth_mac_addr(ETH1_BASE);
	eth.ethertype = htons(ET_ARP);

	arp.arp_htype = htons(ARP_HTYPE_ETH);
	arp.arp_ptype = htons(ET_IPV4);
	arp.arp_hlen = 6;
	arp.arp_plen = 4;
	arp.arp_oper = htons(ARP_OPER_REQUEST);
	arp.arp_sha = eth.src;
	arp.arp_spa = htonl(0x0a00000b);
	arp.arp_tpa = htonl(addr);
	memset(&arp.arp_tha, 0x00, 6);

	uint32_t btag = MAKE_BTAG(0xcafe, sizeof(eth) + sizeof(arp));

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &arp, sizeof(arp), 0, 1, btag);

	uint8_t buf[2048];
	while(1) {
		uint32_t sts = eth_rx_wait_sts(ETH1_BASE);
		uint32_t len = (sts >> 16) & 0x3fff;
		eth_rx(ETH1_BASE, (uint32_t *)buf, len);
		struct ethhdr *recveth = (void *)buf;
		if(recveth->ethertype != htons(ET_ARP))
			continue;
		if(memcmp(&recveth->dest, &eth.src, 6))
			continue;
		return ((struct arppkt *)(buf + sizeof(struct ethhdr)))->arp_sha;
	}
}

uint32_t send_udp(mac_addr_t macaddr, uint32_t addr, uint16_t port, const char *data, uint16_t len) {
	struct ethhdr eth;
	struct ip ip;
	struct udphdr udp;

	eth.dest = macaddr;
	eth.src = eth_mac_addr(ETH1_BASE);
	eth.ethertype = htons(ET_IPV4);

	ip.ip_vhl = IP_VHL_BORING;
	ip.ip_tos = 0;
	ip.ip_len = htons(sizeof(struct ip) + sizeof(struct udphdr) + len);
	ip.ip_id = htons(0);
	ip.ip_off = htons(0);
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_UDP;
	ip.ip_sum = 0;
	ip.ip_src.s_addr = htonl(0x0a00000b);
	ip.ip_dst.s_addr = htonl(addr);
	ip.ip_sum = htons(ip_checksum((uint8_t *)&ip, sizeof(struct ip)));

	udp.uh_sport = htons(7777);
	udp.uh_dport = htons(port);
	udp.uh_ulen = htons(sizeof(struct udphdr) + len);
	udp.uh_sum = htons(0);

	uint32_t btag = MAKE_BTAG(0xbeef, sizeof(eth) + sizeof(ip) + sizeof(udp) + len);

	eth_tx(ETH1_BASE, &eth, sizeof(eth), 1, 0, btag);
	eth_tx(ETH1_BASE, &ip, sizeof(ip), 0, 0, btag);
	eth_tx(ETH1_BASE, &udp, sizeof(udp), 0, 0, btag);
	eth_tx(ETH1_BASE, data, len, 0, 1, btag);

	return eth_tx_wait_sts(ETH1_BASE);
}
