#ifndef SERVER_CLOCK_H
#define SERVER_CLOCK_H

int clockserver_tid;
#define CLOCK_NOTIFY_MSG 1
#define CLOCK_DELAY_MSG 2
#define CLOCK_TIME_MSG 3

void msleep(int msec);

int Time();

void clockserver_task();
void clockserver_notifier();

#endif /* SERVER_CLOCK_H */