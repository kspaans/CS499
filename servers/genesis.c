#include <lib.h>
#include <string.h>
#include <syscall.h>
#include <types.h>

#include <kern/ksyms.h>
#include <servers/net.h>
#include <servers/genesis.h>
/*
 *   Genesis: The story of creation.
 *     Server accepts UDP packets that are instructions on the creation of a
 *     new task.
 *
 */

#define GENESIS_PORT 400
#define GENESIS_SRCPORT 50400
#define SMAGIC 0xBABEB00B
#define EMAGIC 0xFACEDADA

void genesis_test(void) {
	printf("MY GOD, THEY'VE CREATED LIFE\n");
}

#define GENLEN 64

/* The start and end magic are just to make sure we don't get UDP noise */
struct creation_request {
		unsigned smagic;
		int priority;
		int flags;
		char path[GENLEN]; // Todo: What's the real max length?
		unsigned emagic;
} __attribute__((__packed__));

static void print_creation(struct creation_request *cr) {
	printf("Name %s Magics: %x %x (%x %x), Flags %d, Pri %d\n", cr->path, cr->smagic, cr->emagic, SMAGIC, EMAGIC, cr->flags, cr->priority);
}

static void handle_createreq(int priority, const char *path, int flags) {
	void (*code)(void) = address_for_symbol(path);
	// This is some pointer to a symbol, which could quite possibly no
	// be a function. Potential hurt there. So don't do that.
	if(code) {
		printf("Launching process %s with priority: %d, flags: %d\n",
				path, priority, flags);
		ASSERTNOERR(spawn(priority, code, flags));
	} else {
		printf("Failure: Unknown symbol %s\n", path);
	}
}

void send_createreq(uint32_t host, int priority, const char *name, int flags) {
	if(host == this_host->ip) {
		handle_createreq(priority, name, flags);
		return;
	}
	struct creation_request req;
	req.smagic = SMAGIC;
	req.emagic = EMAGIC;
	req.priority = priority;
	req.flags = flags;
	strlcpy(req.path, name, GENLEN);
	printf("Spawning %s on host %08x\n", req.path, host);

	// I'd like to avoid using the send_udp primitive if I can.
	// If we have channels that work over the network, that might be better.
	int ret = send_udp(GENESIS_SRCPORT, host, GENESIS_PORT, (char *)&req, sizeof(req));
	if(ret != 0) {
		printf("Error transmitting, %d", ret);
	}
	// Wait for a reply?
}

static void genesis_task(void) {
	struct {
		struct packet_rec rec;
		struct creation_request data;
	} __attribute__((__packed__)) reply;

	printf("Listening for creation requests\n");
	udp_bind(GENESIS_PORT);

	while(1) {
		ASSERTNOERR(udp_wait(GENESIS_PORT, &reply.rec, sizeof(reply)));
		// Check contents:
		if(reply.data.smagic != SMAGIC || reply.data.emagic != EMAGIC ||
			reply.rec.data_len != sizeof(struct creation_request)) {
			printf("Got a bad request: ");
			print_creation(&reply.data);
			continue;
		}
		handle_createreq(reply.data.priority, reply.data.path, reply.data.flags);
	}
	udp_release(GENESIS_PORT);
}

void genesis_init(void) {
	xspawn(3, genesis_task, SPAWN_DAEMON);
}
