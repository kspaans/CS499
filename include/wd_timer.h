#ifndef WD_TIMER_H
#define WD_TIMER_H

void enable_wdt();
void disable_wdt();
void load_wdt(uint32_t time);
uint32_t read_wdt();
void kick_wdt();

#endif
