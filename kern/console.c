#include <lib.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <server/console.h>
#include <drivers/uart.h>

#define CHUNK_SIZE 256

static void console_printfunc(void *unused, const char *buf, size_t len) {
	(void)unused;
	for(int i=0; i<len; i+=CHUNK_SIZE) {
		int chunk = (i+CHUNK_SIZE < len) ? CHUNK_SIZE : len-i;
		Send(consoletx_tid, CONSOLE_TX_DATA_MSG, buf+i, chunk, NULL, 0);
	}
}

int printf(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(console_printfunc, NULL, fmt, va);
	va_end(va);
	return ret;
}

int vprintf(const char *fmt, va_list va) {
	return func_vprintf(console_printfunc, NULL, fmt, va);
}

int getchar() {
	return Send(consolerx_tid, CONSOLE_RX_REQ_MSG, NULL, 0, NULL, 0);
}

void putchar(char c) {
	Send(consoletx_tid, CONSOLE_TX_DATA_MSG, &c, 1, NULL, 0);
}

void consoletx_task() {
	int tid, rcvlen, msgcode;
	char buf[CHUNK_SIZE];
	while(1) {
		rcvlen = Receive(&tid, &msgcode, buf, CHUNK_SIZE);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case CONSOLE_TX_NOTIFY_MSG:
		case CONSOLE_TX_DATA_MSG:
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			break;
		}
	}
}

void consolerx_task() {
	int tid, rcvlen, msgcode;
	while(1) {
		rcvlen = Receive(&tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case CONSOLE_RX_NOTIFY_MSG:
		case CONSOLE_RX_REQ_MSG:
		default:
			ReplyStatus(tid, ERR_REPLY_BADREQ);
			break;
		}
	}
}

void consoletx_notifier() {
	while(1) {
		AwaitEvent(EVENT_CONSOLE_TRANSMIT);
		Send(consoletx_tid, CONSOLE_TX_NOTIFY_MSG, NULL, 0, NULL, 0);
	}
}

void consolerx_notifier() {
	while(1) {
		AwaitEvent(EVENT_CONSOLE_RECEIVE);
		Send(consolerx_tid, CONSOLE_RX_NOTIFY_MSG, NULL, 0, NULL, 0);
	}
}
