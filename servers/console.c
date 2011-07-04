#include <lib.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <task.h>
#include <msg.h>
#include <servers/console.h>
#include <drivers/uart.h>
#include <mem.h>
#include <panic.h>

static void consoletx_notifier(void);
static void consolerx_notifier(void);

enum consolemsg {
	CONSOLE_TX_NOTIFY_MSG = PRIVATE_MSG_START,
	CONSOLE_RX_NOTIFY_MSG,
};

#define TX_BUF_MAX 32768
#define RX_BUF_MAX 16384
#define RX_TIDS_MAX 64

static int console_printfunc(void *pch, const char *buf, size_t len) {
	if (len > MAX_PRINT)
		return EINVAL;

	int channel = *(int *)pch;

	for(size_t i=0; i<len; i+=PRINT_CHUNK) {
		int chunk = (i+PRINT_CHUNK < len) ? PRINT_CHUNK : len-i;
		int res = MsgSend(channel, STDOUT_WRITE_MSG, buf+i, chunk, NULL, 0, NULL);
		if(res < 0)
			return res;
	}
	return 0;
}

int fprintf(int channel, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(console_printfunc, &channel, fmt, va);
	va_end(va);
	return ret;
}

int vfprintf(int channel, const char *fmt, va_list va) {
	return func_vprintf(console_printfunc, &channel, fmt, va);
}

int printf(const char *fmt, ...) {
	int channel = STDOUT_FILENO;
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(console_printfunc, &channel, fmt, va);
	va_end(va);
	return ret;
}

int vprintf(const char *fmt, va_list va) {
	int channel = STDOUT_FILENO;
	return func_vprintf(console_printfunc, &channel, fmt, va);
}

int fgetc(int channel) {
	return MsgSend(channel, STDIN_GETCHAR_MSG, NULL, 0, NULL, 0, NULL);
}

int getchar(void) {
	return fgetc(STDIN_FILENO);
}

int fputc(char c, int channel) {
	return MsgSend(channel, STDOUT_WRITE_MSG, &c, 1, NULL, 0, NULL);
}

void putchar(char c) {
	MsgSend(STDOUT_FILENO, STDOUT_WRITE_MSG, &c, 1, NULL, 0, NULL);
}

static void consoletx_task(void) {
	int tid, rcvlen, msgcode;
	char rcvbuf[PRINT_CHUNK];
	char *cur;
	int newline_seen;

	int ret = spawn(0, consoletx_notifier, SPAWN_DAEMON);
	if(ret < 0)
		panic("consoletx_notifier failed to start: %s (%d)", strerror(ret), ret);

	charqueue chq;
	char chq_arr[TX_BUF_MAX];
	charqueue_init(&chq, chq_arr, TX_BUF_MAX);

	while(1) {
		rcvlen = MsgReceive(STDOUT_FILENO, &tid, &msgcode, rcvbuf, sizeof(rcvbuf));
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case CONSOLE_TX_NOTIFY_MSG:
			MsgReplyStatus(tid, 0);
			while(!charqueue_empty(&chq) && !uart_txfull())
				uart_tx(charqueue_pop(&chq));
			if(!charqueue_empty(&chq))
				uart_intenable(UART_THR_IT);
			break;
		case STDOUT_WRITE_MSG:
			// Transmit current buffer
			while(!charqueue_empty(&chq) && !uart_txfull())
				uart_tx(charqueue_pop(&chq));
			// Transmit as much of the new data as possible
			cur = rcvbuf;
			newline_seen = 0;
			if(charqueue_empty(&chq)) {
				while(rcvlen > 0 && !uart_txfull()) {
					if(!newline_seen && *cur == '\n') {
						uart_tx('\r');
						newline_seen = 1;
					} else {
						uart_tx(*cur++);
						--rcvlen;
					}
				}
			}
			// Enqueue the rest of the new data
			while(rcvlen > 0) {
				if(charqueue_full(&chq)) {
					MsgReplyStatus(tid, ENOMEM);
					break;
				}
				if(!newline_seen && *cur == '\n') {
					charqueue_push(&chq, '\r');
					newline_seen = 1;
				} else {
					charqueue_push(&chq, *cur++);
					--rcvlen;
				}
			}
			// Reply to user
			if(rcvlen == 0) {
				MsgReplyStatus(tid, 0);
			}
			// Enable interrupts if there's still something which needs to be sent
			if(!charqueue_empty(&chq))
				uart_intenable(UART_THR_IT);
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
	}
}

static void consolerx_task(void) {
	int tid, rcvlen, msgcode;

	xspawn(0, consolerx_notifier, SPAWN_DAEMON);

	intqueue tidq;
	int tidq_arr[RX_TIDS_MAX];
	intqueue_init(&tidq, tidq_arr, RX_TIDS_MAX);

	charqueue chq;
	char chq_arr[RX_BUF_MAX];
	charqueue_init(&chq, chq_arr, RX_BUF_MAX);

	while(1) {
		rcvlen = MsgReceive(STDIN_FILENO, &tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case CONSOLE_RX_NOTIFY_MSG:
			MsgReplyStatus(tid, 0);
			while(!uart_rxempty()) {
				if(charqueue_full(&chq))
					charqueue_pop(&chq);
				charqueue_push(&chq, uart_rx());
			}
			break;
		case STDIN_GETCHAR_MSG:
			if(intqueue_full(&tidq)) {
				MsgReplyStatus(tid, ENOMEM);
				continue;
			}
			intqueue_push(&tidq, tid);
			break;
		default:
			MsgReplyStatus(tid, ENOFUNC);
			continue;
		}
		while(!intqueue_empty(&tidq) && !charqueue_empty(&chq))
			MsgReplyStatus(intqueue_pop(&tidq), charqueue_pop(&chq));
		uart_intenable(UART_RHR_IT);
	}
}

static void consoletx_notifier(void) {
	while(1) {
		event_wait(EVENT_CONSOLE_TRANSMIT);
		MsgSend(STDOUT_FILENO, CONSOLE_TX_NOTIFY_MSG, NULL, 0, NULL, 0, NULL);
	}
}

static void consolerx_notifier(void) {
	while(1) {
		event_wait(EVENT_CONSOLE_RECEIVE);
		MsgSend(STDIN_FILENO, CONSOLE_RX_NOTIFY_MSG, NULL, 0, NULL, 0, NULL);
	}
}

void console_init(void) {
	int res = spawn(1, consoletx_task, SPAWN_DAEMON);
	if(res < 0)
		panic("consoletx_task failed to start: %s (%d)", strerror(res), res);
	xspawn(1, consolerx_task, SPAWN_DAEMON);
}
