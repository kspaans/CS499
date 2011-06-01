#include <lib.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <task.h>
#include <servers/console.h>
#include <drivers/uart.h>
#include <mem.h>

static int consoletx_tid;
static int consolerx_tid;

enum consolemsg {
	CONSOLE_TX_NOTIFY_MSG,
	CONSOLE_RX_NOTIFY_MSG,
	CONSOLE_TX_DATA_MSG,
	CONSOLE_RX_REQ_MSG,
};

#define CHUNK_SIZE 256
#define TX_BUF_MAX 32768
#define RX_BUF_MAX 16384
#define RX_TIDS_MAX 64

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

static void consoletx_task() {
	int tid, rcvlen, msgcode;
	char rcvbuf[CHUNK_SIZE];
	char *cur;
	int newline_seen;

	charqueue chq;
	char chq_arr[TX_BUF_MAX];
	charqueue_init(&chq, chq_arr, TX_BUF_MAX);

	while(1) {
		rcvlen = Receive(&tid, &msgcode, rcvbuf, CHUNK_SIZE);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case CONSOLE_TX_NOTIFY_MSG:
			ReplyStatus(tid, 0);
			while(!charqueue_empty(&chq) && !uart_txfull())
				uart_tx(charqueue_pop(&chq));
			if(!charqueue_empty(&chq))
				uart_intenable(UART_THR_IT);
			break;
		case CONSOLE_TX_DATA_MSG:
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
					ReplyStatus(tid, ERR_NOMEM);
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
				ReplyStatus(tid, 0);
			}
			// Enable interrupts if there's still something which needs to be sent
			if(!charqueue_empty(&chq))
				uart_intenable(UART_THR_IT);
			break;
		default:
			ReplyStatus(tid, ERR_NOFUNC);
			continue;
		}
	}
}

static void consolerx_task() {
	int tid, rcvlen, msgcode;

	intqueue tidq;
	int tidq_arr[RX_TIDS_MAX];
	intqueue_init(&tidq, tidq_arr, RX_TIDS_MAX);

	charqueue chq;
	char chq_arr[RX_BUF_MAX];
	charqueue_init(&chq, chq_arr, RX_BUF_MAX);

	while(1) {
		rcvlen = Receive(&tid, &msgcode, NULL, 0);
		if(rcvlen < 0)
			continue;
		switch(msgcode) {
		case CONSOLE_RX_NOTIFY_MSG:
			ReplyStatus(tid, 0);
			while(!uart_rxempty()) {
				if(charqueue_full(&chq))
					charqueue_pop(&chq);
				charqueue_push(&chq, uart_rx());
			}
			break;
		case CONSOLE_RX_REQ_MSG:
			if(intqueue_full(&tidq)) {
				ReplyStatus(tid, ERR_NOMEM);
				continue;
			}
			intqueue_push(&tidq, tid);
			break;
		default:
			ReplyStatus(tid, ERR_NOFUNC);
			continue;
		}
		while(!intqueue_empty(&tidq) && !charqueue_empty(&chq))
			ReplyStatus(intqueue_pop(&tidq), charqueue_pop(&chq));
		uart_intenable(UART_RHR_IT);
	}
}

static void consoletx_notifier() {
	while(1) {
		AwaitEvent(EVENT_CONSOLE_TRANSMIT);
		Send(consoletx_tid, CONSOLE_TX_NOTIFY_MSG, NULL, 0, NULL, 0);
	}
}

static void consolerx_notifier() {
	while(1) {
		AwaitEvent(EVENT_CONSOLE_RECEIVE);
		Send(consolerx_tid, CONSOLE_RX_NOTIFY_MSG, NULL, 0, NULL, 0);
	}
}

void console_start_tasks() {
	consoletx_tid = KernCreateTask(1, consoletx_task, TASK_DAEMON);
	consolerx_tid = KernCreateTask(1, consolerx_task, TASK_DAEMON);
	KernCreateTask(0, consoletx_notifier, TASK_DAEMON);
	KernCreateTask(0, consolerx_notifier, TASK_DAEMON);
}
