/* Includes ------------------------------------------------------------------*/
#include "mqtt_client.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
char                                _mqtt_host[65]              = MQTT_HOST;
char                                _mqtt_username[65]          = MQTT_USERNAME;
char                                _mqtt_password[65]          = MQTT_PASSWORD;
char                                _mqtt_topic_sub[65]         = MQTT_TOPIC_SUB;
char                                _mqtt_topic_pub[65]         = MQTT_TOPIC_PUB;
char                                _mqtt_token[65]             = MQTT_TOKEN;
char                                _mqtt_auth_method[20]       = MQTT_AUTH_METHOD;
uint32_t                            _mqtt_port                  = MQTT_PORT;

static const char*                  TAG                         = "MQTT CLIENT";
TaskHandle_t                        _mqtt_client_handler_t      = NULL;


/* Functions -----------------------------------------------------------------*/
void init_mqtt_client(void)
{
    APP_LOGI(TAG, "Mqtt Client Task init.");

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_mqtt_client,
        "MQTT CLIENT",
        1024 * 15,
        NULL,
        20,
        &_mqtt_client_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create Mqtt Client Task failed.");
}

void mqtt_message_callback(String &topic, String &payload)
{    
    APP_LOGI(TAG, "Message from server.");
    mqtt_message_t msg;
    strncpy(msg.topic, topic.c_str(), sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload.c_str(), sizeof(msg.payload) - 1);
    xQueueSend(_mqtt_incoming_queue, &msg, 0);    
}

void task_mqtt_client(void *pvParameters) {
    APP_LOGI(TAG, "MQTT Client Task started.");
    MQTTClient mqtt_client(512 * 6);
    mqtt_client.setKeepAlive(60);       
    WiFiClient wifi_client;    
    
    
    for(;;) {
        APP_LOGI(TAG, "MQTT: Waiting for WiFi connection...");
        xEventGroupWaitBits(_normal_mode_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
                
        APP_LOGI(TAG, "MQTT: WiFi connected, now connecting to MQTT broker...");        
        mqtt_client.begin(_mqtt_host, _mqtt_port, wifi_client);
        APP_LOGW(TAG, "MQTT host: %s, port: %d", _mqtt_host, _mqtt_port);
        mqtt_client.onMessage(mqtt_message_callback);        
        
        if(strcmp(_mqtt_auth_method, "User/Pass") == 0)
        {
            while(!mqtt_client.connect("2", _mqtt_username, _mqtt_password)) {
                APP_LOGW(TAG, "MQTT connection failed with user/pass, retrying in 5 seconds...");
                APP_LOGW(TAG, "MQTT id: %s, username: %s, password: %s", _id, _mqtt_username, _mqtt_password);
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
        else if(strcmp(_mqtt_auth_method, "Token") == 0)
        {
            while(!mqtt_client.connect(_id, _mqtt_token, "")) {
                APP_LOGW(TAG, "MQTT connection failed with token, retrying in 5 seconds...");
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
        else
        {
            APP_LOGE(TAG, "Identify authenticate method failed.");
        }
        
        

        APP_LOGI(TAG, "MQTT Connected!");
        xEventGroupSetBits(_normal_mode_event_group, MQTT_CONNECTED_BIT);
        
        if(mqtt_client.subscribe(_mqtt_topic_sub))
        {
            APP_LOGI(TAG, "Subcribe topic %s successful", _mqtt_topic_sub);
        }
        else
        {
            APP_LOGW(TAG, "Subcribe topic %s failed.", _mqtt_topic_sub);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));

        while(mqtt_client.connected()) {
            mqtt_client.loop(); 
            
            mqtt_message_t data_to_send;
            if (xQueueReceive(_mqtt_outgoing_queue, &data_to_send, 0) == pdPASS) {
                mqtt_client.publish(data_to_send.topic, data_to_send.payload);
                APP_LOGI(TAG, "MQTT: Published data to topic %s", data_to_send.topic);
            }
            
            mqtt_message_t data_receive;
            if (xQueueReceive(_mqtt_incoming_queue, &data_receive, 0) == pdPASS) {
                APP_LOGI(TAG, "MQTT: Received command from server: %s", data_receive.payload);           
                mqtt_handle_remote_config(data_receive.payload);
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        xEventGroupClearBits(_normal_mode_event_group, MQTT_CONNECTED_BIT);
        APP_LOGW(TAG, "MQTT Disconnected. Will retry...");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


void mqtt_reset_to_default_config(void)
{
    strcpy(_mqtt_host, MQTT_HOST);
    strcpy(_mqtt_username, MQTT_USERNAME);
    strcpy(_mqtt_password, MQTT_PASSWORD);
    strcpy(_mqtt_topic_sub, MQTT_TOPIC_SUB);
    strcpy(_mqtt_topic_pub, MQTT_TOPIC_PUB);
    strcpy(_mqtt_token, MQTT_TOKEN);
    strcpy(_mqtt_auth_method, MQTT_AUTH_METHOD);    
}

void mqtt_handle_remote_config(const char* payload) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        APP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        return;
    }

    const char* command = doc["command"];
    if (!command || strcmp(command, "set_config") != 0) {
        APP_LOGW(TAG, "Invalid or missing command in payload");
        return;
    }

    JsonObject data = doc["data"];
    if (!data) {
        APP_LOGW(TAG, "Missing 'data' object in payload");
        return;
    }

    bool wifi_updated = false;
    bool server_updated = false;

    // --- Xử lý cấu hình WiFi ---
    if (data.containsKey("wifi")) {
        JsonObject wifi_config = data["wifi"];
        strlcpy(_wifi_ssid, wifi_config["ssid"] | "", sizeof(_wifi_ssid));
        strlcpy(_wifi_password, wifi_config["password"] | "", sizeof(_wifi_password));
        wifi_updated = true;
        APP_LOGI(TAG, "Received new WiFi config: SSID=%s", _wifi_ssid);
    }
    else
    {
        APP_LOGW(TAG, "No WiFi configuration found in payload");
    }

    // --- Xử lý cấu hình Server ---
    if (data.containsKey("server")) {
        JsonObject server_config = data["server"];
        const char* mode = server_config["mode"] | "default";

        if (strcmp(mode, "default") == 0) 
        {            
            mqtt_reset_to_default_config();
        } 
        else if (strcmp(mode, "advanced") == 0) 
        {            
            strlcpy(_mqtt_host, server_config["host"] | "", sizeof(_mqtt_host));
            strlcpy(_mqtt_topic_pub, server_config["topic"] | "", sizeof(_mqtt_topic_pub));

            if (server_config.containsKey("auth")) 
            {
                JsonObject auth_config = server_config["auth"];
                const char* auth_method = auth_config["method"] | "token";
                strlcpy(_mqtt_auth_method, auth_method, sizeof(_mqtt_auth_method));

                if (strcmp(auth_method, "token") == 0) 
                {
                    strlcpy(_mqtt_token, auth_config["token"] | "", sizeof(_mqtt_token));
                } 
                else if (strcmp(auth_method, "user_pass") == 0) 
                {
                    strlcpy(_mqtt_username, auth_config["username"] | "", sizeof(_mqtt_username));
                    strlcpy(_mqtt_password, auth_config["password"] | "", sizeof(_mqtt_password));
                }
            }
        }
        server_updated = true;
        APP_LOGI(TAG, "Received new Server config");        
    }
    else
    {
        APP_LOGW(TAG, "No Server configuration found in payload");
    }    


    APP_LOGI(TAG, "MQTT Host: %s, Topic: %s, Auth Method: %s", _mqtt_host, _mqtt_topic_pub, _mqtt_auth_method);
    APP_LOGI(TAG, "MQTT Username: %s, Password: %s, Token: %s", _mqtt_username, _mqtt_password, _mqtt_token);
    APP_LOGI(TAG, "WiFi SSID: %s, Password: %s", _wifi_ssid, _wifi_password);
    
    if (wifi_updated) 
    {
        memory_save_wifi_config();                
    }
    if (server_updated) 
    {
        memory_save_mqtt_config();                
    }

    if (wifi_updated || server_updated) {
        APP_LOGI(TAG, "Configuration updated. Sending reload command to system.");
        app_system_event_t evt;
        evt.command = CMD_CONFIG_UPDATED_RELOAD;
        xQueueSend(_system_cmd_queue, &evt, 0);
    }
}