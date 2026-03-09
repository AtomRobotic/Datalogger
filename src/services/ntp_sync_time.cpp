/* Includes ------------------------------------------------------------------*/
#include "ntp_sync_time.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static const char*          TAG                         = "NTP SYNC TIME";
TaskHandle_t                _ntp_sync_time_handler_t    = NULL;


/* Functions -----------------------------------------------------------------*/
void init_ntp_sync_time_task(void)
{
    APP_LOGI(TAG, "NTP Sync init"); 
    BaseType_t task_result = xTaskCreatePinnedToCore(
        ntp_sync_time_task,
        "SYNC TIME",
        1024 * 2,
        NULL,
        10,
        &_ntp_sync_time_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create NTP Sync Task failed.");
}

void ntp_sync_time_task(void *pvParameters)
{
    bool isFirstSync = true;
    const TickType_t xRetryInterval = pdMS_TO_TICKS(5000);
    const TickType_t xLongDelay = pdMS_TO_TICKS(1 * 60 * 60 * 1000); 

    APP_LOGI(TAG, "NTP Sync Task started.");

    for (;;) {
        
        APP_LOGI(TAG, "NTP: Waiting for WiFi connection...");
        xEventGroupWaitBits(_normal_mode_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        
        IPAddress resolvedIP;
        if (!WiFi.hostByName(NTP_SERVER, resolvedIP)) { 
            APP_LOGE(TAG, "NTP: DNS resolution failed. Retrying in 15 seconds...");
            vTaskDelay(xRetryInterval);
            continue; 
        }
        APP_LOGI(TAG, "NTP: DNS OK. '%s' is at %s", NTP_SERVER, resolvedIP.toString().c_str());

        vTaskDelay(5000);
        ntp_time_init();
        vTaskDelay(5000);

        
        if (ntp_update_time()) {
            APP_LOGI(TAG, "NTP: Time synced successfully: %s", ntp_time_get_buffer());
            
            if (isFirstSync) {
                xEventGroupSetBits(_normal_mode_event_group, NTP_SYNCED_BIT);
                isFirstSync = false;
            }

            
            vTaskDelay(xLongDelay);

        } else {
            APP_LOGW(TAG, "NTP: Failed to sync time. Retrying in 15 seconds...");
            vTaskDelay(xRetryInterval);
        }
    }

}
