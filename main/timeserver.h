// timeserver.h
#ifndef TIMESERVER_H
#define TIMESERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

void timeserver_init(void);
void timeserver_set_epoch_ms(time_t epoch);
void timeserver_set_offset(int32_t offset_seconds);
void timeserver_sync(void);
const char* timeserver_get_time_str(void);
bool timeserver_should_accept_time(void);

void timeserver_sta_set_connected(bool connected);
time_t timeserver_get_epoch(void);
int32_t timeserver_get_offset(void);

#endif // TIMESERVER_H
