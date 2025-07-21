#ifndef LIGHTS_H_
#define LIGHTS_H_

#include "sdkconfig.h"
#include "esp_system.h"
#include "led_strip.h"
#include "esp_log.h"
#include "control_led.h"

#define STRIP_GPIO CONFIG_STRIP_GPIO
#define UPDATE_PERIOD CONFIG_UPDATE_PERIOD
#define LED_BRIGHTNESS CONFIG_LED_BRIGHTNESS

#endif