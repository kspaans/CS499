#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <ip.h>

#define ERR_ETH_BADTX -10
#define ERR_ARP_PENDING -11
#define ERR_ARP_TIMEOUT -12

void net_reserve_tids();
void net_start_tasks();

#endif /* SERVER_NET_H */
