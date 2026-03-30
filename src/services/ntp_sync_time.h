/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_log.h"
#include "app_assert.h"
#include "hal/ntp_time.h"
#include "hal/rtc.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void init_ntp_sync_time_task(void);
void ntp_sync_time_task(void *pvParameters);

