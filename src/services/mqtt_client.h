/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
#include "app_log.h"
#include "app_assert.h"
#include "cJSON.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void init_mqtt_client(void);
void mqtt_message_callback(String &topic, String &payload);
void mqtt_reset_to_default_config(void);
void mqtt_handle_remote_config(const char* payload);
void task_mqtt_client(void *pvParameters);