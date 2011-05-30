#include <syscall.h>
#include <server/clock.h>

void msleep(int msec) {
	if(msec <= 0) return;
	Send(tid_clockserver, CLOCK_DELAY_MSG, &msec, sizeof(msec), NULL, 0);
}

int Time() {
	return Send(tid_clockserver, CLOCK_TIME_MSG, NULL, 0, NULL, 0);
}

typedef struct {
	int tid;
	int time;
} delayinfo;

static delayinfo delayinfoheap_pop(delayinfo *delays, int *n) {
	delayinfo ret = delays[0];
	(*n)--;
	int current_location = 0;
	delayinfo current = delays[*n];
	while (2 * current_location + 1 < (*n)) {
		int child = 2 * current_location + 1;
		int childvalue = delays[child].time;
		if (2 * current_location + 2 < (*n)
				&& delays[2 * current_location + 2].time < childvalue) {
			child = 2 * current_location + 2;
			childvalue = delays[child].time;
		}
		if (current.time <= childvalue) {
			break;
		}
		delays[current_location] = delays[child];
		current_location = child;
	}
	delays[current_location] = current;
	return ret;
}

static void delayinfoheap_push(delayinfo *delays, int *n, delayinfo value) {
	(*n)++;
	int current_location = (*n) - 1;
	int parent = (current_location - 1) / 2;
	while (current_location > 0 && value.time < delays[parent].time) {
		delays[current_location] = delays[parent];
		current_location = parent;
		parent = (current_location - 1) / 2;
	}
	delays[current_location] = value;
}

#define DELAYS 5000

void task_clockserver() {
	int tid;
	int rcvlen;
	int rcvdata;
	int msgcode;
	int time = 0;
	delayinfo delays[DELAYS];
	int num_delays = 0;
	while(1) {
		rcvlen = Receive(&tid, &msgcode, &rcvdata, sizeof(rcvdata));
		if(rcvlen < 0) {
			printk("ERROR: Clock server receive failed\n");
			continue;
		}
		switch(msgcode) {
		case CLOCK_NOTIFY_MSG:
			ReplyStatus(current_info.tid, 0);
			time++;
			while (num_delays > 0 && delays[0].time <= time) {
				delayinfo current_info = delayinfoheap_pop(delays, &num_delays);
				ReplyStatus(current_info.tid, 0);
			}
			break;
		case CLOCK_DELAY_MSG:
			if(rcvlen < sizeof(int)) {
				printk("Bad clock message");
				ReplyStatus(tid, ERR_REPLY_BADREQ);
				break;
			}
			rcvdata += time;
			if (rcvdata <= time) {
				ReplyStatus(tid, 0);
			} else if (num_delays == DELAYS) {
				ReplyStatus(tid, ERR_REPLY_NOSPACE);
			} else {
				delayinfo current_info;
				current_info.tid = tid;
				current_info.time = rcvdata;
				delayinfoheap_push(delays, &num_delays, current_info);
			}
			break;
		case CLOCK_TIME_MSG:
			ReplyStatus(tid, time);
			break;
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			break;
		}
	}
}
