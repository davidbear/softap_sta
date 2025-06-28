#ifndef CONTROL_LED_H_
#define CONTROL_LED_H_

#include "sdkconfig.h"
#include "esp_system.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define LED_BRIGHTNESS CONFIG_LED_BRIGHTNESS

extern bool led_state;

#ifdef CONFIG_BLINK_LED_STRIP
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
#endif
void blink_led(void);
void configure_led(void);

#endif