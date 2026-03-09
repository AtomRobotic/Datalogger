#ifndef __APP_LOG_H__
#define __APP_LOG_H__

#include <Arduino.h>

// Mức log
#define APP_LOG_LEVEL_NONE    0
#define APP_LOG_LEVEL_ERROR   1
#define APP_LOG_LEVEL_WARN    2
#define APP_LOG_LEVEL_INFO    3
#define APP_LOG_LEVEL_DEBUG   4
#define APP_LOG_LEVEL_VERBOSE 5

// Mức log hiện tại (có thể đổi tùy lúc build/debug)
#define APP_LOG_LEVEL APP_LOG_LEVEL_VERBOSE

// Macro nội bộ để in kèm tag và level
#define __APP_LOG(levelStr, tag, format, ...)              \
    do {                                                   \
        Serial.printf("[%s] %s: " format "\n",             \
            levelStr, tag, ##__VA_ARGS__);                 \
    } while(0)

// Các macro tương ứng từng mức
#if APP_LOG_LEVEL >= APP_LOG_LEVEL_ERROR
#define APP_LOGE(tag, format, ...) __APP_LOG("ERROR", tag, format, ##__VA_ARGS__)
#else
#define APP_LOGE(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_LEVEL_WARN
#define APP_LOGW(tag, format, ...) __APP_LOG("WARN", tag, format, ##__VA_ARGS__)
#else
#define APP_LOGW(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_LEVEL_INFO
#define APP_LOGI(tag, format, ...) __APP_LOG("INFO", tag, format, ##__VA_ARGS__)
#else
#define APP_LOGI(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_LEVEL_DEBUG
#define APP_LOGD(tag, format, ...) __APP_LOG("DEBUG", tag, format, ##__VA_ARGS__)
#else
#define APP_LOGD(tag, format, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_LEVEL_VERBOSE
#define APP_LOGV(tag, format, ...) __APP_LOG("VERBOSE", tag, format, ##__VA_ARGS__)
#else
#define APP_LOGV(tag, format, ...)
#endif

#endif // __APP_LOG_H__
