#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <stdarg.h>
#include <types.h>
#include <ip.h>

#define EETH_BADTX -256
#define EARP_PENDING -257
#define EARP_TIMEOUT -258

struct packet_rec {
	in_addr_t src_ip, dst_ip;
	uint16_t src_port, dst_port;
	size_t data_len;
	char data[0];
};

int arp_lookup(uint32_t ip_addr, mac_addr_t *mac_addr, int timeout_msec);
int send_udp(uint16_t srcport, uint32_t addr, uint16_t dstport, const char *data, uint16_t len);

int udp_vprintf(const char *fmt, va_list va);
int udp_printf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
int udp_getchar(void);

int udp_bind(uint16_t port);
int udp_release(uint16_t port);
int udp_wait(uint16_t port, struct packet_rec *rec, int rec_size);

void net_start_tasks(void);

extern struct hostdata *this_host;

void net_init(void);

void ethrx_task(void);
void icmpserver_task(void);
void arpserver_task(void);
void udprx_task(void);
void udpconrx_task(void);


#endif /* SERVER_NET_H */
