#include <lib.h>
#include <string.h>
#include <syscall.h>
#include <types.h>

#include <servers/net.h>
/*
 *   Genesis: The story of creation.
 *     Server accepts UDP packets that are instructions on the creation of a
 *     new task.
 *
 */

#define GENESIS_PORT 400
#define SMAGIC 0xBABEB00B
#define EMAGIC 0xFACEDADA

__attribute__((unused)) static void genesis_test(void) {
	printf("MY GOD, THEY'VE CREATED LIFE");
}


struct creation_request {
		unsigned smagic;
		int priority;
		int flags;
		//char path[31]; // Todo: What's the real max length?
		void (*code)(void);
		unsigned emagic;
} __attribute__((__packed__));

void genesis_task(void);
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
		if(reply.data.smagic != SMAGIC || reply.data.emagic != EMAGIC || reply.rec.data_len != sizeof(struct creation_request)) {
			printf("Genesis: got some satan packet\n Magics: %x %x\n Length: %d (Expected %d)\n",
					reply.data.smagic, reply.data.emagic, reply.rec.data_len, sizeof(struct creation_request));
			continue;
		}

		printf("Launching process with priority: %d, flags: %d from %p\n",
				reply.data.priority, reply.data.flags, reply.data.code
				);

		/* Todo: Use file server to translate a string into a code pointer */
		// Instead, we just pass the code* right now
		ASSERTNOERR(spawn(reply.data.priority, reply.data.code, reply.data.flags));
	}
	udp_release(GENESIS_PORT);
}
