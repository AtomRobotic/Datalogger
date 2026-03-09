#ifndef __WEB_MANAGER_SERVICES_H_
#define __WEB_MANAGER_SERVICES_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
#include "services/mqtt_client.h"
#include <LittleFS.h>

/* Define --------------------------------------------------------------------*/
#define AP_TIMEOUT_MS (5 * 50 * 1000)
#define LOGIN_TIMEOUT_MS (30 * 60 * 1000)

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/
void start_ap_mode();
void setup_http_server_endpoints();
void on_websocket_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void task_event_handler(void *pvParameters);
void task_ap_main_loop(void *pvParameters);


#endif // __WEB_MANAGER_SERVICES_H_