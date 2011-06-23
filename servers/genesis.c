#include <lib.h>
#include <string.h>
#include <syscall.h>

#include <servers/net.h>
/*
 *   Genesis: The story of creation.
 *     Server accepts UDP packets that are instructions on the creation of a
 *     new task.
 *
 */

#define GENESIS_PORT 400

void genesis_task(void) {
	printf("IN THE BEGINNING\n");
	udp_bind(GENESIS_PORT);
	printf("WE HAD LIGHT\n");
	union {
		struct packet_rec rec;
		char frame[64]; // todo actually make this something
	} reply;
	while(1) {
		reply.rec.data[0] = 's';
		reply.rec.data[1] = 'a';
		reply.rec.data[2] = 'd';
		reply.rec.data[2] = '\0';
		ASSERTNOERR(udp_wait(GENESIS_PORT, &reply.rec, sizeof(reply)));
		printf("Got a message: starts with %s\n", reply.rec.data);
	}
	udp_release(GENESIS_PORT);
}
