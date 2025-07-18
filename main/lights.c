#include <stdio.h>
#include <time.h>

extern bool twelve;
extern bool bcd;

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