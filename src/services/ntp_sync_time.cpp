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
    const TickType_t xLongDelay = pdMS_TO_TICKS(24 * 60 * 60 * 1000); 

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

        
        // Gọi thư viện NTP và kiểm tra lấy giờ thành công
        if (ntp_update_time()) { 
            // 1. Lấy giờ chuẩn (Epoch time) từ thư viện NTPClient
            // (time_client đã được khai báo ở common.h/system_events)
            uint32_t current_epoch = time_client.getEpochTime();

            // 2. GHI XUỐNG RTC PHẦN CỨNG
            rtc_set_time_epoch(current_epoch);
            
            // 3. Đọc ngược lại RTC để kiểm tra
            char rtc_check[32];
            rtc_get_time_buffer(rtc_check, sizeof(rtc_check));
            APP_LOGI(TAG, "NTP: Server time saved to RTC: %s", rtc_check);
            
            if (isFirstSync) {
                xEventGroupSetBits(_normal_mode_event_group, NTP_SYNCED_BIT);
                isFirstSync = false;
            }

            vTaskDelete(NULL); // Tự xóa Task sau khi đồng bộ thành công
            // vTaskDelay(xLongDelay);

        } else {
            APP_LOGW(TAG, "NTP: Failed to sync time. Retrying in 15 seconds...");
            vTaskDelay(xRetryInterval);
        }
    }

}
