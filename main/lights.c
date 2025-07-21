#include <stdio.h>
#include <time.h>
#include "lights.h"

bool twelve = false;
bool bcd = true;
bool reverse_order = false;
bool reverse_color = false;
uint32_t sec_color = 0xFF0000;
uint32_t min_color = 0x0;
uint32_t hour_color = 0x0;

extern led_strip_handle_t led_strip;

void init_clock(void)
{
    uint32_t h;
    uint32_t s;
    uint32_t v;
    uint32_t r;
    uint32_t g;
    uint32_t b;
    led_strip_rgb2hsv(sec_color>>16, 0xFF&(sec_color>>8), 0xFF&sec_color, &h, &s, &v);
    h += 120;
    if(reverse_color) h += 120;
    h %= 360;
    led_strip_hsv2rgb(h, s, v, &r, &g, &b);
    min_color = r << 16 | g << 8 | b;
    h += 120;
    if(reverse_color) h += 120;
    h %= 360;
    led_strip_hsv2rgb(h, s, v, &r, &g, &b);
    hour_color = r << 16 | g << 8 | b;
}

uint8_t make_bcd(uint8_t two_digit)
{
    uint8_t tens = two_digit / 10;
    uint8_t ones = two_digit % 10;
    return (tens << 4) | ones;
}

void update() {
    time_t current_time;
    struct tm *local_time_info;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    // Get the current calendar time
    current_time = time(NULL);

    // Convert to local time
    local_time_info = localtime(&current_time);

    second = local_time_info->tm_sec;
    minute = local_time_info->tm_min;
    hour = local_time_info->tm_hour;
    if(twelve) hour %= 12;

    if(bcd) {
        second = make_bcd(second);
        minute = make_bcd(minute);
        hour = make_bcd(hour);
    }


}