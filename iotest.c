 /*
  * iotest.c
  */

#include <bwio.h>
#include <ts7800.h>

int main(int argc, char *argv[]) {
    char *str = "Hello\r\n";
    bwputstr(COM1, str);
    bwputw(COM1, 10, '*', str);
    bwprintf(COM1, "Hello world.\r\n");
    bwprintf(COM1, "%s world%u.\r\n", "Well, hello", 23);
    bwprintf(COM1, "%d worlds for %u person.\r\n", -23, 1);
    bwprintf(COM1, "%x worlds for %d people.\r\n", -23, 723);
    str[0] = bwgetc(COM1);
    bwprintf(COM1, "You typed: %c\r\n", str[0]);
    return 0;
}
