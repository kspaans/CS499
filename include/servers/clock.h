#ifndef SERVER_CLOCK_H
#define SERVER_CLOCK_H

void msleep(int msec);

int time(void);

void clockserver_task(void);

#endif /* SERVER_CLOCK_H */
