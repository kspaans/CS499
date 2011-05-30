#ifndef SERVER_CONSOLE_H
#define SERVER_CONSOLE_H

int consoletx_tid;
int consolerx_tid;
#define CONSOLE_TX_NOTIFY_MSG 1
#define CONSOLE_RX_NOTIFY_MSG 2
#define CONSOLE_TX_DATA_MSG 3
#define CONSOLE_RX_REQ_MSG 4

void consoletx_task();
void consolerx_task();
void consoletx_notifier();
void consolerx_notifier();

#endif /* SERVER_CONSOLE_H */
