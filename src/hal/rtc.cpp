/* Includes ------------------------------------------------------------------*/
#include "rtc.h"
#include <Wire.h>
#include <RTClib.h>
#include "app_log.h"

/* Variables -----------------------------------------------------------------*/
static RTC_DS3231 rtc;
static bool       is_rtc_ready = false;
static const char* TAG = "RTC_MGR";

// Mutex bảo vệ Bus I2C khi giao tiếp với RTC
static SemaphoreHandle_t s_rtc_mutex = NULL;

/* Functions -----------------------------------------------------------------*/

/**
 * @brief Khởi tạo module RTC DS3231
 */
void rtc_init() {
    if (s_rtc_mutex == NULL) {
        s_rtc_mutex = xSemaphoreCreateMutex();
    }

    if (!rtc.begin()) {
        APP_LOGE(TAG, "Couldn't find RTC DS3231");
        is_rtc_ready = false;
        return;
    }

    if (rtc.lostPower()) {
        APP_LOGW(TAG, "RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    is_rtc_ready = true;
    APP_LOGI(TAG, "RTC DS3231 initialized.");
}

/**
 * @brief Lấy thời gian an toàn vào buffer do Task tự cấp phát
 */
bool rtc_get_time_buffer(char* out_buffer, size_t max_len) {
    if (!is_rtc_ready || out_buffer == NULL) {
        if (out_buffer) snprintf(out_buffer, max_len, "1970-01-01T00:00:00");
        return false;
    }

    if (xSemaphoreTake(s_rtc_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        DateTime now = rtc.now();
        xSemaphoreGive(s_rtc_mutex);

        snprintf(out_buffer, max_len, "%04d-%02d-%02dT%02d:%02d:%02d", 
                 now.year(), now.month(), now.day(), 
                 now.hour(), now.minute(), now.second());
        return true;
    }
    
    return false;
}

/**
 * @brief Cập nhật lại thời gian cho RTC bằng Epoch Time (Dùng cho NTP)
 */
bool rtc_set_time_epoch(uint32_t epoch) {
    if (!is_rtc_ready) return false;

    if (xSemaphoreTake(s_rtc_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        rtc.adjust(DateTime(epoch));
        xSemaphoreGive(s_rtc_mutex);
        APP_LOGI(TAG, "RTC Time synchronized with Server (Epoch: %u)", epoch);
        return true;
    }
    return false;
}

/**
 * @brief Cập nhật lại thời gian cho RTC thủ công
 */
bool rtc_set_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (!is_rtc_ready) return false;
    
    if (xSemaphoreTake(s_rtc_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        rtc.adjust(DateTime(year, month, day, hour, min, sec));
        xSemaphoreGive(s_rtc_mutex);
        APP_LOGI(TAG, "RTC Time updated manually.");
        return true;
    }
    return false;
}

float rtc_get_temperature() {
    if (!is_rtc_ready) return 0.0f;
    float temp = 0.0f;
    if (xSemaphoreTake(s_rtc_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        temp = rtc.getTemperature();
        xSemaphoreGive(s_rtc_mutex);
    }
    return temp;
}