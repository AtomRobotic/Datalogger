/* Includes ------------------------------------------------------------------*/
#include "led.h"

/* Define --------------------------------------------------------------------*/
static const char *TAG = "LED_MGR";
SystemLedState current_led_state = STATE_NORMAL;
TaskHandle_t _led_task_handle = NULL;

/* Variables -----------------------------------------------------------------*/
static uint8_t led_a_state = 0;
static uint8_t led_b_state = 0;

/* Functions -----------------------------------------------------------------*/
// Hàm bật/tắt cơ bản (Giữ nguyên)
void led_a_on(void) { led_a_state = LED_ON; digitalWrite(LED_A_PIN, led_a_state); }
void led_b_on(void) { led_b_state = LED_ON; digitalWrite(LED_B_PIN, led_b_state); }
void led_a_off(void) { led_a_state = LED_OFF; digitalWrite(LED_A_PIN, led_a_state); }
void led_b_off(void) { led_b_state = LED_OFF; digitalWrite(LED_B_PIN, led_b_state); }
void led_a_toggle(void) { led_a_state = (led_a_state + 1) % 2; digitalWrite(LED_A_PIN, led_a_state); }
void led_b_toggle(void) { led_b_state = (led_b_state + 1) % 2; digitalWrite(LED_B_PIN, led_b_state); }
void all_led_on(void) { led_a_on(); led_b_on(); }
void all_led_off(void) { led_a_off(); led_b_off(); }

// ---------------------------------------------------------
// TIẾN TRÌNH QUẢN LÝ LED (STATE MACHINE)
// ---------------------------------------------------------
void led_manager_task(void *pvParameters) {
    SystemLedState last_logged_state = (SystemLedState)-1; // Biến nhớ trạng thái cũ
    vTaskDelay(pdMS_TO_TICKS(3000));
    for(;;) {
        // Chỉ in log 1 lần khi có sự chuyển đổi trạng thái để tránh rác màn hình
        if (current_led_state != last_logged_state) {
            // APP_LOGI(TAG, "System State Changed: %d (0=AP, 1=NORMAL, 2=ERROR)", current_led_state);
            last_logged_state = current_led_state;
        }

        switch (current_led_state) {
            case STATE_AP_CONFIG:
                led_a_on();
                led_b_on();
                vTaskDelay(pdMS_TO_TICKS(500)); 
                break;

            case STATE_NORMAL:
                led_a_on();  // A sáng
                led_b_off(); // B tắt
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case STATE_ERROR1:
                led_a_off();       // A giữ sáng
                led_b_toggle();   // B nhấp nháy liên tục
                vTaskDelay(pdMS_TO_TICKS(1000)); // Nhịp nháy 1 giây
                break;
            case STATE_ERROR2:
                led_a_off();       // A giữ sáng
                led_b_on();        // B sáng liên tục
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
    }
}

// ---------------------------------------------------------
// HÀM KHỞI TẠO (Đã gộp Task vào đây)
// ---------------------------------------------------------
void led_init(void)
{
    pinMode(LED_A_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    all_led_off();
    
    // Tạo tiến trình chạy ngầm điều khiển LED
    xTaskCreatePinnedToCore(
        led_manager_task, 
        "LED_MANAGER", 
        2048, 
        NULL, 
        5, 
        &_led_task_handle, 
        1
    );
}

// Hàm giao tiếp để các file khác thay đổi trạng thái
void set_system_led_state(SystemLedState new_state) {
    current_led_state = new_state;
}