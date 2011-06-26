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
	printf("Name %s Magics: %x %x (%x %x), Flags %d, Pri %d", cr->path, cr->smagic, cr->emagic, SMAGIC, EMAGIC, cr->flags, cr->priority);
}

void send_createreq(uint32_t host, int priority, const char *name, int flags) {
	struct creation_request req;
	req.smagic = SMAGIC;
	req.emagic = EMAGIC;
	req.priority = priority;
	req.flags = flags;
	strlcpy(req.path, name, GENLEN);
	printf("Spawning %s on host %d\n", req.path, host);
	send_udp(GENESIS_SRCPORT, host, GENESIS_PORT, (char *)&req, sizeof(req));
}


void genesis_task(void) {
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
			printf("Got a bad request");
			print_creation(&reply.data);
			continue;
		}

		/* Todo: Use file server to translate a string into a code pointer */
		void (*code)(void) = address_for_symbol(reply.data.path);
		if(code) {
			printf("Launching process %s with priority: %d, flags: %d\n",
					reply.data.path, reply.data.priority, reply.data.flags);
			ASSERTNOERR(spawn(reply.data.priority, code, reply.data.flags));
		} else {
			// This should be a reply:
			printf("Unknown thing %s\n", reply.data.path);
		}
	}
	udp_release(GENESIS_PORT);
}
