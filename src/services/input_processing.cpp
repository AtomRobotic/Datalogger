/* Includes ------------------------------------------------------------------*/
#include "input_processing.h"

/* Define --------------------------------------------------------------------*/
#define BUTTON_PIN 35                  
#define DEBOUNCE_TIME_MS 50           
#define LONG_PRESS_TIME_MS 2000       

/* Variables -----------------------------------------------------------------*/
TaskHandle_t _input_processing_handler_t = NULL;
static const char* TAG = "INPUT PROCESSING";


static SemaphoreHandle_t s_button_sem = NULL;

static volatile TickType_t g_last_interrupt_time = 0;

/* Functions -----------------------------------------------------------------*/

/**
 * @brief Hàm xử lý ngắt (ISR - Interrupt Service Routine)
 * @note Hàm này phải được đặt trong IRAM để tốc độ thực thi nhanh nhất.
 * Không được dùng các hàm blocking hoặc log trong ISR.
 */
void IRAM_ATTR button_isr()
{
    TickType_t now = xTaskGetTickCountFromISR();
    
    if (now - g_last_interrupt_time > pdMS_TO_TICKS(DEBOUNCE_TIME_MS))
    {
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(s_button_sem, &xHigherPriorityTaskWoken);
        g_last_interrupt_time = now;

        
        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR();
        }
    }
}

void init_input_processing(void)
{
    APP_LOGI(TAG, "Init input processing with interrupt.");

    
    s_button_sem = xSemaphoreCreateBinary();
    ASSERT_BOOL(s_button_sem != NULL, TAG, "Failed to create button semaphore.");

    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING); 

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_input_processing,
        "INPUT PROCESSING",
        1024 * 3, 
        NULL,
        5,        
        &_input_processing_handler_t,
        1
    );

    ASSERT_BOOL(task_result == pdPASS, TAG, "Create input processing task failed.");
}

void task_input_processing(void *pvParameters)
{
    const TickType_t long_press_ticks = pdMS_TO_TICKS(LONG_PRESS_TIME_MS);
    TickType_t press_time = 0;
    bool is_long_press_event_sent = false;

    for (;;)
    {        
        if (xSemaphoreTake(s_button_sem, portMAX_DELAY) == pdPASS)
        {
            
            APP_LOGI(TAG, "Button interrupt received. Debouncing...");
            
            
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));

            
            if (digitalRead(BUTTON_PIN) == LOW)
            {
                APP_LOGI(TAG, "Button press confirmed.");
                press_time = xTaskGetTickCount();
                is_long_press_event_sent = false;
                                
                while (digitalRead(BUTTON_PIN) == LOW)
                {
                    
                    if (!is_long_press_event_sent && (xTaskGetTickCount() - press_time > long_press_ticks))
                    {
                        APP_LOGI(TAG, "Button long pressed.");
                        is_long_press_event_sent = true; 
                        
                        
                        app_system_event_t cmd_evt;
                        if (_system_current_state == STATE_NORMAL_MODE) {
                            cmd_evt.command = CMD_SWITCH_TO_AP_MODE;
                        } else {
                            cmd_evt.command = CMD_SWITCH_TO_NORMAL_MODE;
                        }
                        xQueueSend(_system_cmd_queue, &cmd_evt, 0);
                        
                    }
                    vTaskDelay(pdMS_TO_TICKS(50)); 
                }
                
                
                if (!is_long_press_event_sent)
                {
                    APP_LOGI(TAG, "Button short pressed.");                    
                }

            } 
        } 
    }
}