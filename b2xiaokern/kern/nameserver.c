#include "nameserver.h"
#include "errno.h"
#include "syscall.h"
#include "lib.h"
#include "message.h"
#include "io.h"

static int nameserver_tid;

int RegisterAs(char *name) {
	int_msg_t reply;
	int replylen = SendTyped(nameserver_tid, name, strlen(name)+1, (char*)&reply, sizeof(reply), NAMESERVER_REGISTER);
	if(replylen == ERR_SEND_BADTID || replylen == ERR_SEND_NOSUCHTID)
		return ERR_NAMESERVER_BADTID;
	if(replylen == ERR_SEND_SRRFAIL)
		return ERR_NAMESERVER_UNKNOWN;
	if(replylen != sizeof(reply))
		return ERR_NAMESERVER_NOTI;
	if(reply.type == NAMESERVER_INVALIDREQUEST)
		return ERR_NAMESERVER_NAMETOOLONG;
	if(reply.type != NAMESERVER_REGISTERREPLY)
		return ERR_NAMESERVER_NOTI;
	return reply.value;
}

int WhoIs(char* name) {
	int_msg_t reply;
	int replylen = SendTyped(nameserver_tid, name, strlen(name)+1, (char*)&reply, sizeof(reply), NAMESERVER_WHOIS);
	if(replylen == ERR_SEND_BADTID || replylen == ERR_SEND_NOSUCHTID)
		return ERR_NAMESERVER_BADTID;
	if(replylen == ERR_SEND_SRRFAIL)
		return ERR_NAMESERVER_UNKNOWN;
	if(replylen != sizeof(reply))
		return ERR_NAMESERVER_NOTI;
	if(reply.type == NAMESERVER_INVALIDREQUEST)
		return ERR_NAMESERVER_NAMETOOLONG;
	if(reply.type != NAMESERVER_WHOISREPLY)
		return ERR_NAMESERVER_NOTI;
	return reply.value;
}

int GetName(int tid, char *buffer) {
	char buf[NAMESERVER_MAXNAME+4];
	int replylen = SendTyped(nameserver_tid, (char *)&tid, 4, buf, sizeof(buf), NAMESERVER_GETNAME);
	if(replylen == ERR_SEND_BADTID || replylen == ERR_SEND_NOSUCHTID)
		return ERR_NAMESERVER_BADTID;
	if(replylen == ERR_SEND_SRRFAIL)
		return ERR_NAMESERVER_UNKNOWN;
	if(replylen <= 4)
		return ERR_NAMESERVER_NOSUCHTASK;
	if(((msg_t*)buf)->type == NAMESERVER_INVALIDREQUEST)
		return ERR_NAMESERVER_NAMETOOLONG;
	if(((msg_t*)buf)->type != NAMESERVER_GETNAMEREPLY)
		return ERR_NAMESERVER_NOTI;
	memcpy(buffer, buf+4, replylen-4);
	return replylen-4;
}

#define HASH_SIZE 1511
#define DATA_SIZE 40000

int calchash(char* str) {
	int hash = 0;
	while(*str) {
		hash = (hash*37 + *str)%HASH_SIZE;
		str++;
	}
	return hash;
}

void task_nameserver() {
	nameserver_tid = MyTid();
	char* hash_table[HASH_SIZE];
	int tids[HASH_SIZE];
	int tidtoloc[HASH_SIZE];
	char data[DATA_SIZE];
	int i;
	for(i = 0; i<HASH_SIZE; i++) {
		hash_table[i] = NULL;
		tidtoloc[i] = -1;
	}
	char* end_loc = data+DATA_SIZE;
	char* current_loc = data;
	char messagedata[NAMESERVER_MAXNAME+4];
	printf("nameserver[%d] initialized\n", nameserver_tid);
	while(1) {
		int tid;
		int received_length = Receive(&tid, messagedata, sizeof(messagedata));
		if(received_length > sizeof(messagedata)) {
			ReplyTyped(tid, "    ", 4, NAMESERVER_INVALIDREQUEST);
			continue;
		}
		switch(((msg_t*)messagedata)->type) {
		case NAMESERVER_REGISTER:
		{
			char* message = ((msg_t*)messagedata)->data;
			int hash = calchash(message);
			int len = strlen(message);
			if(end_loc-current_loc < len + 1) {
				int_msg_t reply;
				reply.type = NAMESERVER_REGISTERREPLY;
				reply.value = ERR_NAMESERVER_NOMEM;
				Reply(tid, (char*)&reply, sizeof(reply));
				break;
			}
			int found = 0;
			int loc;
			for(i = 0; i<HASH_SIZE; i++) {
				loc = (hash+i)%HASH_SIZE;
				if(hash_table[loc] == NULL) {
					found = 1;
					hash_table[loc] = current_loc;
					tids[loc] = tid;
					break;
				}
			}
			if(found) {
				for(i=0; i<HASH_SIZE; i++) {
					int tidloc = (tid+i)%HASH_SIZE;
					if(tidtoloc[tidloc] == tid) {
						continue;
					} else if(tidtoloc[tidloc] == -1) {
						tidtoloc[tidloc] = loc;
						break;
					}
				}
			}
			int_msg_t reply;
			reply.type = NAMESERVER_REGISTERREPLY;
			if(found) {
				memcpy(current_loc, message, len+1);
				current_loc+=len+1;
				reply.value = 0;
			} else {
				reply.value = ERR_NAMESERVER_NOMEM;
			}
			Reply(tid, (char*)&reply, sizeof(reply));
			break;
		}
		case NAMESERVER_WHOIS:
		{
			char* message = ((msg_t*)messagedata)->data;
			int hash = calchash(message);
			int found = ERR_NAMESERVER_NOSUCHTASK;
			for(i = 0; i<HASH_SIZE; i++) {
				int loc = (hash+i)%HASH_SIZE;
				if(hash_table[loc] == NULL)
					break;
				if(!strcmp(message, hash_table[loc])) {
					found = tids[loc];
					break;
				}
			}
			int_msg_t reply;
			reply.type = NAMESERVER_WHOISREPLY;
			reply.value = found;
			Reply(tid, (char*)&reply, sizeof(reply));
			break;
		}
		case NAMESERVER_GETNAME:
		{
			if(received_length != sizeof(int_msg_t)) {
				ReplyTyped(tid, "    ", 4, NAMESERVER_INVALIDREQUEST);
				continue;
			}
			int targettid = ((int_msg_t *)messagedata)->value;
			char *name = NULL;
			for(i=0; i<HASH_SIZE; i++) {
				int tidloc = (targettid+i)%HASH_SIZE;
				if(tidtoloc[tidloc] == -1) {
					name = NULL;
					break;
				} else if(tids[tidtoloc[tidloc]] != targettid) {
					continue;
				} else {
					name = hash_table[tidtoloc[tidloc]];
					break;
				}
			}
			if(name == NULL) {
				ReplyTyped(tid, "", 1, NAMESERVER_GETNAMEREPLY);
				continue;
			} else {
				ReplyTyped(tid, name, strlen(name)+1, NAMESERVER_GETNAMEREPLY);
				break;
			}
		}
		default:
			ReplyTyped(tid, "    ", 4, NAMESERVER_INVALIDREQUEST);
			break;
		}
	}
}
