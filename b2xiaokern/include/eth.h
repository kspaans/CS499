#ifndef _ETH_H
#define _ETH_H
#include "inttypes.h"
#include "ip.h"

#pragma pack(push, 2)

typedef struct {
	uint8_t addr[6];
} mac_addr_t;

struct ethhdr {
	uint8_t preamble[8];
	mac_addr_t dest;
	mac_addr_t src;
	uint16_t ethertype;
};

#define ET_IPV4 0x0800
#define ET_ARP 0x0806

struct arppkt {
	uint16_t arp_htype;
	uint16_t arp_ptype;
	uint8_t arp_hlen;
	uint8_t arp_plen;
	mac_addr_t arp_sha;
	in_addr_t arp_spa;
	mac_addr_t arp_tha;
	in_addr_t arp_tpa;
};

#define ARP_HTYPE_ETH 0x1

#pragma pack(pop)

int eth_init(int base);
void eth_tx(int base, void *buf, uint16_t nbytes, int first, int last, uint32_t btag);
void eth_rx(int base, uint32_t *buf, uint16_t nbytes);
uint32_t eth_rx_wait_sts(int base);
uint32_t eth_tx_wait_sts(int base);
mac_addr_t eth_mac_addr(int base);

#define MAKE_BTAG(tag,len) (((tag) << 16) | ((len) & 0x7ff))

#endif /* _ETH_H */