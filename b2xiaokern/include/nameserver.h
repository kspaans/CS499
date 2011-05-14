#ifndef NAMESERVER_H
#define NAMESERVER_H

/* Maximum possible name length */
#define NAMESERVER_MAXNAME 64

int RegisterAs(char *name);
int WhoIs(char *name);
int GetName(int tid, char *buffer);

void task_nameserver();

#endif
