/* Includes ------------------------------------------------------------------*/
#include "system_events.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
QueueHandle_t               _system_cmd_queue           = NULL;
QueueHandle_t               _mqtt_outgoing_queue        = NULL;
QueueHandle_t               _mqtt_incoming_queue        = NULL;
QueueHandle_t               _raw_sensor_data_queue      = NULL;
QueueHandle_t               _web_manager_event_queue    = NULL;

EventGroupHandle_t          _normal_mode_event_group;
app_system_state_t          _system_current_state;
static const char*          TAG                         = "SYSTEM SUPERVISOR";

uint16_t                    _version;           
uint16_t                    reset_count                 = 0;

WiFiUDP                     ntpUDP;
NTPClient                   time_client(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);  // GMT+7

/* Functions -----------------------------------------------------------------*/
void init_system_supervisor(void)
{
    APP_LOGI(TAG, "Init system supervisor.");

    memory_init();    

    led_init();
    

    if(EepromRead16b(_address_version) == VERSION_DEFAULT)
    {
        APP_LOGI(TAG, "Load all configuration.");                        

        memory_load_device_id();
        APP_LOGI(TAG, "Device ID: %s, Station code: %s", _id, _station_code);
        memory_load_reset_count();
        APP_LOGI(TAG, "Reset count: %d", reset_count);
        reset_count = (reset_count + 1) % 65535;
        memory_save_reset_count();

        memory_load_wifi_config();
        APP_LOGI(TAG, "WiFi SSID: %s, Password: %s", _wifi_ssid, _wifi_password);
        
        memory_load_mqtt_config();
        APP_LOGI(TAG, "MQTT Host: %s, MQTT Topic: %s", _mqtt_host, _mqtt_topic_pub);
        APP_LOGI(TAG, "MQTT Authenticat Method: %s", _mqtt_auth_method);
        APP_LOGI(TAG, "MQTT Username: %s, MQTT Password: %s", _mqtt_username, _mqtt_password);
        APP_LOGI(TAG, "MQTT Token: %s, port: %d", _mqtt_token, _mqtt_port);

        // APP_LOGI(TAG, "MQTT Host: %s, MQTT username: %s, MQTT password", _mqtt_host, _mqtt_username, _mqtt_password);
        // APP_LOGI(TAG, "MQTT Topic Subscribed: %s, MQTT Publish: %s", _mqtt_topic_sub, _mqtt_topic_pub);
    }
    else
    {
        APP_LOGI(TAG, "First init system");  
        _version = VERSION_DEFAULT;
        memory_update_version();        
        memory_save_device_id();
        memory_save_station_code();
        memory_save_reset_count();
        memory_save_wifi_config();      
        memory_save_mqtt_config();
    }        
    

    /** NOTE WHEN CREATE QUEUE AND EVENT GROUP
     *  Cause this task will never be delete, so we should define some global resources, that we can
     *  optimize the system by not free the memory for it
     *  When we delete a task, we should delete all the resource that we allocate in that.
     * 
     *  NEED TO CLARIFY THIS PROBLEM LATER
     */

    _normal_mode_event_group = xEventGroupCreate();
    ASSERT_BOOL(_normal_mode_event_group != NULL, TAG, "Create event group failed.");

    delay(100);

    _system_cmd_queue = xQueueCreate(5, sizeof(app_system_event_t));
    ASSERT_BOOL(_system_cmd_queue != NULL, TAG, "Create system message queue failed.");

    delay(100);

    _mqtt_outgoing_queue = xQueueCreate(10, sizeof(mqtt_message_t*));
    ASSERT_BOOL(_mqtt_outgoing_queue != NULL, TAG, "Create outgoing queue failed.");

    delay(100);

    _mqtt_incoming_queue = xQueueCreate(5, sizeof(mqtt_message_t));
    ASSERT_BOOL(_mqtt_incoming_queue != NULL, TAG, "Create incoming queue failed.");

    delay(100);        

    _web_manager_event_queue = xQueueCreate(5, sizeof(app_event_t));
    ASSERT_BOOL(_web_manager_event_queue != NULL, TAG, "Create web manager event queue failed.");
    
    

    delay(100);

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_system_supervisor,
        "SYSTEM SUPERVISOR",
        1024 * 15,
        NULL,
        25,
        NULL,
        1
    );    
    

    delay(100);

    init_input_processing();

    vTaskDelay(100);
    init_backup_manager();         

    delay(100);

    ASSERT_BOOL(task_result, TAG, "Create system supervisor failed!");        
}

void task_system_supervisor(void *pvParameters)
{    
    app_system_event_t evt;

    _system_current_state = STATE_AP_MODE;    
    /**
     * When the system start, we will go to the ap mode first, just for configuration and check system health.
     */
    start_ap_mode();
    vTaskDelay(100);
    init_ntp_sync_time_task();    
    all_led_on();

    for(;;)
    {
        if (xQueueReceive(_system_cmd_queue, &evt, portMAX_DELAY) == pdPASS) 
        {
            APP_LOGI(TAG, "Received command, start processing.");
            if (evt.command == CMD_SWITCH_TO_NORMAL_MODE && _system_current_state == STATE_AP_MODE) 
            {
                APP_LOGI(TAG, "Received command to switch to NORMAL mode.");
                all_led_off();
                                            
                if (_event_task_handler_t != NULL)
                {
                    vTaskDelete(_event_task_handler_t);
                    _event_task_handler_t = NULL; 
                }

                vTaskDelay(pdMS_TO_TICKS(500));

                APP_LOGI(TAG, "Starting NORMAL mode tasks...");

                ASSERT_BOOL(WiFi.softAPdisconnect(), TAG, "Disconnect AP mode failed.");
                ASSERT_BOOL(WiFi.disconnect(), TAG, "Reset WiFi failed.");                                        

                /** Update needed
                 *  We can create an better normal mode mechanism
                 */                
                

                init_wifi_manager();
                vTaskDelay(100);
                init_mqtt_client();
                vTaskDelay(100);      
                init_modbus_manager();
                vTaskDelay(100);                                  
                   


                _system_current_state = STATE_NORMAL_MODE;
            }
             else if (evt.command == CMD_SWITCH_TO_AP_MODE && _system_current_state == STATE_NORMAL_MODE)
            {
                APP_LOGI(TAG, "Received command to switch to AP mode.");
                all_led_on();

                if(_wifi_manager_handler_t != NULL)
                {
                    vTaskDelete(_wifi_manager_handler_t);
                    _wifi_manager_handler_t = NULL;
                }

                if(_mqtt_client_handler_t != NULL)
                {
                    vTaskDelete(_mqtt_client_handler_t);
                    _mqtt_client_handler_t = NULL;
                }                

                if(_modbus_manager_handler_t != NULL)
                {
                    vTaskDelete(_modbus_manager_handler_t);
                    _modbus_manager_handler_t = NULL;
                }                
                
                vTaskDelay(pdMS_TO_TICKS(500));

                APP_LOGI(TAG, "Starting AP mode tasks...");

                if(WiFi.isConnected())
                {
                    ASSERT_BOOL(WiFi.disconnect(), TAG, "Disconnect from STA mode failed.");                
                    vTaskDelay(100);
                }
                ASSERT_BOOL(WiFi.mode(WIFI_OFF), TAG, "Reset WiFi failed.");
                
                start_ap_mode();

                _system_current_state = STATE_AP_MODE;
            }
            else if (evt.command == CMD_CONFIG_UPDATED_RELOAD && _system_current_state == STATE_NORMAL_MODE) 
            {
                APP_LOGI(TAG, "Received command to reload configuration.");              
                 if(_wifi_manager_handler_t != NULL)
                {
                    vTaskDelete(_wifi_manager_handler_t);
                    _wifi_manager_handler_t = NULL;
                }

                if(_mqtt_client_handler_t != NULL)
                {
                    vTaskDelete(_mqtt_client_handler_t);
                    _mqtt_client_handler_t = NULL;
                }                

                if(_modbus_manager_handler_t != NULL)
                {
                    vTaskDelete(_modbus_manager_handler_t);
                    _modbus_manager_handler_t = NULL;
                }       
                
                for(uint8_t i = 0; i < 5; i++)
                {
                    all_led_on();
                    vTaskDelay(pdMS_TO_TICKS(500));
                    all_led_off();
                    vTaskDelay(pdMS_TO_TICKS(500));
                }

                init_wifi_manager();
                vTaskDelay(100);
                init_mqtt_client();
                vTaskDelay(100);      
                init_modbus_manager();
                vTaskDelay(100);
                
                
            }
            else if (evt.command == CMD_CONFIG_UPDATED_RELOAD || evt.command == CMD_SWITCH_TO_NORMAL_MODE)
            {
                APP_LOGI(TAG, "Configuration updated or Mode changed. REBOOTING SYSTEM...");
                vTaskDelay(pdMS_TO_TICKS(1000)); // Đợi log in ra hết
                esp_restart(); // Reset phần cứng, an toàn 100%, tự load EEPROM mới cấu hình
            }
            else
            {
                APP_LOGW(TAG, "Unknown command received: %d", evt.command);
            }
        }
       
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
