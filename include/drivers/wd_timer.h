#ifndef WD_TIMER_H
#define WD_TIMER_H

/* Watchdog Timer Registers */
#define WD_CFG 0x010
# define WD_CFG_SOFTRESET 0x2
#define WDSTAT 0x014
# define WDSTAT_RESETDONE 0x1
#define WISR 0x018
#define WIER 0x01C
# define WD_OVF 0x1
#define WCLR 0x024
#define WCRR 0x028
#define WLDR 0x02C
#define WTGR 0x030
#define WWPS 0x034
#define WSPR 0x048
# define WSPR_STOP_1 0xAAAA
# define WSPR_STOP_2 0x5555
# define WSPR_START_1 0xBBBB
# define WSPR_START_2 0x4444
#define WD_TICKS_PER_SEC 32768

void wdt_go(int value);
void wdt_stop(void);
void wdt_reload(void);

#endif
