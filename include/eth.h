#ifndef _ETH_H
#define _ETH_H
#include <types.h>

struct ethhdr {
	mac_addr_t dest;
	mac_addr_t src;
	uint16_t ethertype;
#define ET_IPV4 0x0800
#define ET_ARP 0x0806
} __attribute__((packed)) ;


struct arppkt {
	uint16_t arp_htype;
#define ARP_HTYPE_ETH 0x1
	uint16_t arp_ptype;
	uint8_t arp_hlen;
	uint8_t arp_plen;
	uint16_t arp_oper;
#define ARP_OPER_REQUEST 1
#define ARP_OPER_REPLY 2
	mac_addr_t arp_sha;
	in_addr_t arp_spa;
	mac_addr_t arp_tha;
	in_addr_t arp_tpa;
} __attribute__((packed)) ;

mac_addr_t my_mac;

#endif /* _ETH_H */
