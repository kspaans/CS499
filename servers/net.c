#include <lib.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <task.h>
#include <servers/clock.h>
#include <servers/net.h>

static int ethrx_tid; // general Ethernet receiver
static int udprx_tid; // UDP receiver server
static int arpserver_tid; // ARP server (MAC cache, ARP responder)
static int udpconrx_tid; // UDP-based console receiver

#define UDPCON_CLIENT_PORT 26846
#define UDPCON_SERVER_PORT 26847
#define UDPCON_SERVER_IP IP(10,0,0,1)

enum netmsg {
	ETH_RX_NOTIFY_MSG,
	ETH_RX_REQ_MSG,

	ARP_QUERY_MSG,
	ARP_TIMEOUT_MSG,

	UDP_RX_NOTIFY_MSG,
	UDP_RX_BIND_MSG,
	UDP_RX_REQ_MSG,
	UDP_RX_RELEASE_MSG,

	UDPCON_RX_NOTIFY_MSG,
	UDPCON_RX_REQ_MSG,
};

/* reserve_tids and start_tasks are called as part of kernel initialization */
void net_reserve_tids() {
	ethrx_tid = reserve_tid();
	udprx_tid = reserve_tid();
	arpserver_tid = reserve_tid();
	udpconrx_tid = reserve_tid();
}

void net_start_tasks() {
}
