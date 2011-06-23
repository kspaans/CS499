#ifndef SERVER_CONSOLE_H
#define SERVER_CONSOLE_H

#define MAX_PRINT (100 << 20) // 100 MB

void consolerx_task(void);
void consoletx_task(void);

#endif /* SERVER_CONSOLE_H */
