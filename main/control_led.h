#ifndef CONTROL_LED_H_
#define CONTROL_LED_H_

#include "sdkconfig.h"
#include "esp_system.h"
#include "time.h"
#include <sys/time.h>

#define BLED_GPIO CONFIG_BLED_GPIO
#define STRIP_GPIO CONFIG_STRIP_GPIO
#define UPDATE_PERIOD CONFIG_UPDATE_PERIOD
#define LED_BRIGHTNESS CONFIG_LED_BRIGHTNESS
#define BRIGHTNESS CONFIG_BRIGHTNESS
#define N_LEDS CONFIG_N_LEDS

extern bool led_state;

void led_strip_rgb2hsv(uint32_t r, uint32_t g, uint32_t b, uint32_t *h, uint32_t *s, uint32_t *v);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
void toggle_rgb_led(void);
void configure_led(void);
void Blink_Task(void *arg);

#endif