#ifndef NTP_TIME_H
#define NTP_TIME_H

#include "common.h"
#include <stddef.h>
#include <time.h> 

void ntp_time_init(void);
bool ntp_update_time(void);
char* ntp_time_get_buffer(void);
bool ntp_time_is_synced(void);


#endif // NTP_TIME_H