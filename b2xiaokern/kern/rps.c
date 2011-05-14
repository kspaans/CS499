#include "lib.h"
#include "nameserver.h"
#include "syscall.h"
#include "message.h"
#include "rps.h"

int SignUp(int rps_tid) {
	msg_t reply;
	int replylen = SendTyped(rps_tid, "", 0, (char *) &reply, sizeof(reply),
			RPS_SIGNUP);
	if (replylen < 0)
		return replylen;
	if (replylen != sizeof(reply))
		return ERR_RPS_NOTI;
	if (reply.type != RPS_SIGNUPREPLY)
		return ERR_RPS_NOTI;
	return 0;
}

int Play(int rps_tid, int move) {
	int_msg_t reply;
	int replylen = SendTyped(rps_tid, (char *) &move, sizeof(int),
			(char *) &reply, sizeof(reply), RPS_PLAY);
	if (replylen < 0)
		return replylen;
	if (replylen != sizeof(reply))
		return ERR_RPS_NOTI;
	if (reply.type != RPS_PLAYREPLY)
		return ERR_RPS_NOTI;
	return reply.value;
}

int Quit(int rps_tid) {
	msg_t reply;
	int replylen = SendTyped(rps_tid, "", 0, (char *) &reply, sizeof(reply),
			RPS_QUIT);
	if (replylen < 0)
		return replylen;
	if (replylen != sizeof(reply))
		return ERR_RPS_NOTI;
	if (reply.type != RPS_QUITREPLY)
		return ERR_RPS_NOTI;
	return 0;
}

static const char *playstr[] = { "Rock", "Paper", "Scissors" };

#define MAX_TASKS 1024
void rps_server() {
	int queuedtid = 0;
	int partners[MAX_TASKS];
	int plays[MAX_TASKS];
	int round[MAX_TASKS];
	int nwins[MAX_TASKS], nlosses[MAX_TASKS], ndraws[MAX_TASKS];
	char namebuf1[100];
	char namebuf2[100];
	int i, recvlen;
	int mytid = MyTid();
	int tid;
	int_msg_t request, reply;

	for (i = 0; i < MAX_TASKS; i++) {
		partners[i] = 0;
		plays[i] = -1;
		round[i] = nwins[i] = nlosses[i] = ndraws[i] = 0;
	}

	ASSERTNOERR(RegisterAs("rps_server"));
	printf("rps_server[%d]: initialized\n", mytid);
	while (1) {
		recvlen = Receive(&tid, (char *) &request, sizeof(request));
		if (recvlen > sizeof(request)) {
			ReplyTyped(tid, "", 0, RPS_INVALIDREQUEST);
			continue;
		}
		switch (request.type) {
		case RPS_SIGNUP:
			if (queuedtid == 0) {
				printf("rps_server[%d]: tid %d queued\n", mytid, tid);
				queuedtid = tid;
				continue;
			}
			/* Sign up queuedtid and tid as partners. */
			partners[tid] = queuedtid;
			partners[queuedtid] = tid;
			round[tid] = round[queuedtid] = 0;
			ReplyTyped(tid, "", 0, RPS_SIGNUPREPLY);
			ReplyTyped(queuedtid, "", 0, RPS_SIGNUPREPLY);
			GetName(tid, namebuf1);
			GetName(queuedtid, namebuf2);
			printf("rps_server[%d]: %s[%d] paired with %s[%d]\n", mytid,
					namebuf1, tid, namebuf2, queuedtid);
			queuedtid = 0;
			break;
		case RPS_PLAY: {
			if (recvlen < sizeof(request)) {
				ReplyTyped(tid, "", 0, RPS_INVALIDREQUEST);
				continue;
			}
			if (partners[tid] == 0) {
				GetName(tid, namebuf1);
				printf("rps_server[%d]: %s[%d]'s partner has quit.\n", mytid,
						namebuf1, tid);
				reply.type = RPS_PLAYREPLY;
				reply.value = RPS_RESULT_QUIT;
				Reply(tid, (char *) &reply, sizeof(reply));
				break;
			}
			plays[tid] = request.value;
			if (plays[partners[tid]] == -1) {
				continue;
			}
			int tid1, tid2;
			if (tid < partners[tid]) {
				tid1 = tid;
				tid2 = partners[tid];
			} else {
				tid1 = partners[tid];
				tid2 = tid;
			}
			round[tid1]++;
			round[tid2]++;
			int result1 = 0;
			int result2 = 0;
			int p1play = plays[tid1] % 3;
			int p2play = plays[tid2] % 3;
			plays[tid1] = -1;
			plays[tid2] = -1;
			if (p1play == p2play) {
				result1 = result2 = RPS_RESULT_DRAW;
				ndraws[tid1]++;
				ndraws[tid2]++;
			} else if (p1play == (p2play + 1) % 3) {
				result1 = RPS_RESULT_WIN;
				result2 = RPS_RESULT_LOSE;
				nwins[tid1]++;
				nlosses[tid2]++;
			} else {
				result1 = RPS_RESULT_LOSE;
				result2 = RPS_RESULT_WIN;
				nlosses[tid1]++;
				nwins[tid2]++;
			}
			reply.type = RPS_PLAYREPLY;
			reply.value = result1;
			Reply(tid1, (char *) &reply, sizeof(reply));
			reply.value = result2;
			Reply(tid2, (char *) &reply, sizeof(reply));
			GetName(tid1, namebuf1);
			GetName(tid2, namebuf2);
			printf("rps_server[%d]: %s[%d] vs. %s[%d]: %s played against %s: ",
					mytid, namebuf1, tid1, namebuf2, tid2, playstr[p1play],
					playstr[p2play]);
			if (result1 == RPS_RESULT_WIN) {
				printf("%s[%d] wins\n", namebuf1, tid1);
			} else if (result1 == RPS_RESULT_LOSE) {
				printf("%s[%d] wins\n", namebuf2, tid2);
			} else {
				printf("draw\n");
			}
			printf("    Press <enter> to continue; q to quit.\n");
			if(getchar() == 'q')
				Exit();
			break;
		}
		case RPS_QUIT: {
			int partner = partners[tid];
			int partnerplay = plays[partner];
			if (partner) {
				partners[partner] = 0;
				plays[partner] = -1;
			}
			partners[tid] = 0;
			plays[tid] = -1;
			GetName(tid, namebuf1);
			printf("rps_server[%d]: %s[%d] has quit after round %d.\n", mytid, namebuf1, tid,
					round[tid]);
			printf("    %s[%d]'s lifetime record: %d-%d-%d\n", namebuf1, tid, nwins[tid],
					nlosses[tid], ndraws[tid]);
			ReplyTyped(tid, "", 0, RPS_QUITREPLY);
			if (partnerplay != -1) {
				reply.type = RPS_PLAYREPLY;
				reply.value = RPS_RESULT_QUIT;
				Reply(partner, (char *) &reply, sizeof(reply));
			}
			break;
		}
		default:
			ReplyTyped(tid, "", 0, RPS_INVALIDREQUEST);
			continue;
		}
	}
}

#undef ASSERTNOERR
#define ASSERTNOERR(ret) { \
	int rval = (ret); \
	if(rval < 0) { \
		if(rval != ERR_RPS_NOSUCHTID) { /* RPS server gone = system quit, not an error */ \
			printf("\033[1;41mAssert \"" #ret " >= 0\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
			printf("Return value %d\033[m\n", rval); \
		} \
		Exit(); \
	} \
}

void rps_rockbot() {
	int rps_tid = WhoIs("rps_server");
	int tid = MyTid();
	RegisterAs("rps_rockbot");
	printf("rps_rockbot[%d] started\n", tid);
	ASSERTNOERR(rps_tid);
	ASSERTNOERR(SignUp(rps_tid));
	int i;
	int result;
	for (i = 0; i < 5; i++) {
		result = Play(rps_tid, RPS_PLAY_ROCK);
		if (result <= 0 || result > 4) {
			Exit();
		}
		//printf("ROCKBOT ROUND %d PLAY ROCK: RESULT: %s\n", i, resultstr[result]);
		if (result == RPS_RESULT_QUIT) {
			ASSERTNOERR(Quit(rps_tid));
			ASSERTNOERR(SignUp(rps_tid));
			i--;
		}
	}
	ASSERTNOERR(Quit(rps_tid));
	Exit();
}

void rps_paperbot() {
	int rps_tid = WhoIs("rps_server");
	int tid = MyTid();
	RegisterAs("rps_paperbot");
	printf("rps_paperbot[%d] started\n", tid);
	ASSERTNOERR(rps_tid);
	ASSERTNOERR(SignUp(rps_tid));
	int i;
	int result;
	for (i = 0; i < 5; i++) {
		result = Play(rps_tid, RPS_PLAY_PAPER);
		if (result <= 0 || result > 4) {
			Exit();
		}
		//printf("PaperBot round %d played Paper! Result: %s\n", i, resultstr[result]);
		if (result == RPS_RESULT_QUIT) {
			ASSERTNOERR(Quit(rps_tid));
			ASSERTNOERR(SignUp(rps_tid));
			i--;
		}
	}
	Quit(rps_tid);
	Exit();
}

void rps_randbot() {
	int rps_tid = WhoIs("rps_server");
	int tid = MyTid();
	RegisterAs("rps_randbot");
	printf("rps_randbot[%d] started\n", tid);
	int play;
	ASSERTNOERR(rps_tid);
	ASSERTNOERR(SignUp(rps_tid));
	int i;
	int result;
	for (i = 0; i < rand() % 10; i++) {
		play = rand() % 3;
		result = Play(rps_tid, play);
		if (result <= 0 || result > 4) {
			Exit();
		}
		//printf("RandBot round %d played %s! Result: %s\n", i, playstr[play], resultstr[result]);
		if (result == RPS_RESULT_QUIT) {
			ASSERTNOERR(Quit(rps_tid));
			ASSERTNOERR(SignUp(rps_tid));
			i--;
		}
	}
	Quit(rps_tid);
	Exit();
}

void rps_quitbot() {
	int rps_tid = WhoIs("rps_server");
	int tid = MyTid();
	RegisterAs("rps_quitbot");
	int i;
	printf("rps_quitbot[%d] started\n", tid);
	for (i = 0; i < 2; i++) {
		SignUp(rps_tid);
		Quit(rps_tid);
	}
	Exit();
}

void rps_onebot() {
	int rps_tid = WhoIs("rps_server");
	int tid = MyTid();
	RegisterAs("rps_onebot");
	int i;
	printf("rps_onebot[%d] started\n", tid);
	for (i = 0; i < 3; i++) {
		SignUp(rps_tid);
		Play(rps_tid, rand() % 3);
		Quit(rps_tid);
	}
	Exit();
}

void rps_freqbot() {
	int rps_tid = WhoIs("rps_server");
	RegisterAs("rps_freqbot");
	printf("rps_freqbot[%d] started\n", MyTid());

	SignUp(rps_tid);

	int freq[3] = { 0, 0, 0 };
	int i, j;
	for (i = 0; i < 14; i++) {
		int mostfreq = 0;
		for (j = 1; j < 3; j++) {
			if (freq[j] > freq[mostfreq]) {
				mostfreq = j;
			}
		}
		int result = Play(rps_tid, (mostfreq + 1) % 3);
		//printf("RandBot round %d played %s! Result: %s\n", i, playstr[play], resultstr[result]);
		if (result == RPS_RESULT_QUIT) {
			Quit(rps_tid);
			SignUp(rps_tid);
			freq[0] = freq[1] = freq[2] = 0;
			i--;
		} else if (result == RPS_RESULT_WIN) {
			freq[mostfreq]++;
		} else if (result == RPS_RESULT_LOSE) {
			freq[(mostfreq + 2) % 3]++;
		} else if (result == RPS_RESULT_DRAW) {
			freq[(mostfreq + 1) % 3]++;
		}
	}
	Quit(rps_tid);
	Exit();
}

void rps_antifreqbot() {
	int rps_tid = WhoIs("rps_server");
	RegisterAs("rps_antifreqbot");
	printf("rps_antifreqbot[%d] started\n", MyTid());

	SignUp(rps_tid);

	int freq[3] = { 0, 0, 0 };
	int i, j;
	for (i = 0; i < 12; i++) {
		int mostfreq = 0;
		for (j = 1; j < 3; j++) {
			if (freq[j] < freq[mostfreq]) {
				mostfreq = j;
			}
		}
		int result = Play(rps_tid, (mostfreq + 1) % 3);
		//printf("RandBot round %d played %s! Result: %s\n", i, playstr[play], resultstr[result]);
		if (result == RPS_RESULT_QUIT) {
			Quit(rps_tid);
			SignUp(rps_tid);
			freq[0] = freq[1] = freq[2] = 0;
			i--;
		} else if (result == RPS_RESULT_WIN) {
			freq[mostfreq]++;
		} else if (result == RPS_RESULT_LOSE) {
			freq[(mostfreq + 2) % 3]++;
		} else if (result == RPS_RESULT_DRAW) {
			freq[(mostfreq + 1) % 3]++;
		}
	}
	Quit(rps_tid);
	Exit();
}

void rps_doublebot() {
	int rps_tid = WhoIs("rps_server");
	RegisterAs("rps_doublebot");
	printf("rps_doublebot[%d] started\n", MyTid());

	SignUp(rps_tid);

	int order[6] = { 0, 0, 1, 1, 2, 2 };
	int current_in_order = 0;

	int i;
	for (i = 0; i < 13; i++) {
		int result = Play(rps_tid, order[current_in_order]);
		current_in_order = (current_in_order + 1) % 6;
		if (result == RPS_RESULT_QUIT) {
			Quit(rps_tid);
			SignUp(rps_tid);
			current_in_order = 0;
			i--;
		}
	}
	Quit(rps_tid);
	Exit();
}
