#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <ip.h>

#define ERR_ETH_BADTX -10
#define ERR_ARP_PENDING -11
#define ERR_ARP_TIMEOUT -12

int arp_lookup(uint32_t ip_addr, mac_addr_t *mac_addr, int timeout_msec);
int send_udp(uint16_t srcport, uint32_t addr, uint16_t dstport, const char *data, uint16_t len);

int udp_vprintf(const char *fmt, va_list va);
int udp_printf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

void net_reserve_tids();
void net_start_tasks();

#endif /* SERVER_NET_H */
