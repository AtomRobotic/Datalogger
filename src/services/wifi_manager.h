/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
#include "app_log.h"
#include "app_assert.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void init_wifi_manager(void);
void wifi_event_callback(WiFiEvent_t event);
void task_wifi_manager(void *pvParameters);