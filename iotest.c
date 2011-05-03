 /*
  * iotest.c
  */

#include <bwio.h>
#include <omap3.h>

int main(int argc, char *argv[]) {
    char *str = "Hello\r\n";
    bwputstr(COM3, str);
    bwputw(COM3, 10, '*', str);
    bwprintf(COM3, "Hello world.\r\n");
    bwprintf(COM3, "%s world%u.\r\n", "Well, hello", 23);
    bwprintf(COM3, "%d worlds for %u person.\r\n", -23, 1);
    bwprintf(COM3, "%x worlds for %d people.\r\n", -23, 723);
    str[0] = bwgetc(COM3);
    bwprintf(COM3, "You typed: %c\r\n", str[0]);
    return 0;
}
