#ifndef LEDS_H
#define LEDS_H

enum leds {
    LED1, // GPIO 21
    LED2, // GPIO 22
    LED3, // eth0 1
    LED4, // eth0 2
    LED5, // PWM A
};

void led_set(enum leds led, int status);

#endif