#ifndef __APP_ASSERT_H__
#define __APP_ASSERT_H__

#include <Arduino.h>

// Có thể mở rộng thêm log ra file hoặc MQTT nếu muốn
#define ASSERT_BOOL(cond, tag, message)                                            \
    do {                                                                           \
        if (!(cond)) {                                                             \
            Serial.printf("[ASSERT][%s] Failed: %s\n  at %s:%d\n",                 \
                         tag, message, __FILE__, __LINE__);                        \
            abort(); /* hoặc vTaskSuspend(NULL); hoặc return; tuỳ cách xử lý */   \
        }                                                                          \
    } while (0)

#endif // __APP_ASSERT_H__
