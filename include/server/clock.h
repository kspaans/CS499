#ifndef SERVER_CLOCK_H
#define SERVER_CLOCK_H

int tid_clockserver;
#define CLOCK_NOTIFY_MSG 1
#define CLOCK_DELAY_MSG 2
#define CLOCK_TIME_MSG 3

void msleep(int msec);

int Time();
int DelayUntil(int time);

void task_clockserver();

#endif /* SERVER_CLOCK_H */