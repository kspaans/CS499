#ifndef SERVER_CONSOLE_H
#define SERVER_CONSOLE_H

#define MAX_PRINT (100 << 20) // 100 MB
/* PRINT_CHUNK should not exceed UDPMTU (~1400) */
#define PRINT_CHUNK 1024

void console_init(void);

#endif /* SERVER_CONSOLE_H */
