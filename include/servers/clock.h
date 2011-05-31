#ifndef SERVER_CLOCK_H
#define SERVER_CLOCK_H

void msleep(int msec);

int Time();

void clock_reserve_tids();
void clock_start_tasks();

#endif /* SERVER_CLOCK_H */
