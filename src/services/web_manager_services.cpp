#include "web_manager_services.h"


static AsyncWebServer   s_http_server(80);   
static AsyncWebServer   s_ws_server(81);     
static AsyncWebSocket   s_ws("/");         

static char             s_ap_ssid_temp[32]          = "";
static uint8_t          s_is_ap_logged_in           = 0;
static TickType_t       s_last_activity_tick        = 0;
static TickType_t       s_login_timestamp_tick      = 0;

char                    _id[11]                     = ID_DEFAULT;            
char                    _station_code[17]           = STATION_CODE;
TaskHandle_t            _event_task_handler_t       = NULL;
TaskHandle_t            _ap_main_loop_handler_t     = NULL;

char                    _ap_ssid[65]                = AP_SSID_DEFAULT;
char                    _ap_password[65]            = AP_PASSWORD_DEFAULT;

static const char*      TAG                         = "WEB MANAGER";

static ws_fsm_state_t   s_ws_fsm_state              = FSM_WS_IDLE;
static TickType_t       s_ws_last_sent_tick         = 0; 
static uint32_t         s_current_ws_client_num     = 0; 

static char             s_latest_sensor_json[256]   = "";
static SemaphoreHandle_t s_sensor_data_mutex        = NULL;


static void send_ws_json_to_client(uint32_t client_num, const JsonDocument& doc) {
    String json_string;
    serializeJson(doc, json_string);
    s_ws.text(client_num, json_string);
}

// Gửi cho tất cả các client đang kết nối
static void broadcast_ws_json(const JsonDocument& doc) {
    String json_string;
    serializeJson(doc, json_string);
    s_ws.textAll(json_string);
}

void start_ap_mode()
{
    APP_LOGI(TAG, "Ap mode started.");    

    s_sensor_data_mutex = xSemaphoreCreateMutex();
    ASSERT_BOOL(s_sensor_data_mutex != NULL, TAG, "Failed to create sensor data mutex");    


    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_event_handler,
        "EVENT HANDLER TASK",
        1024 * 8,
        NULL,
        5,
        &_event_task_handler_t,
        1
    );
    ASSERT_BOOL(task_result == pdPASS, TAG, "Failed to create EVENT HANDLER TASK");

    // ASSERT_BOOL(WiFi.disconnect(), TAG, "Failed to disconnect WiFi");
    // ASSERT_BOOL(WiFi.mode(WIFI_OFF), TAG, "Failed to turn off WiFi");

    snprintf(_ap_ssid, sizeof(_ap_ssid), "ABC_IDH_INVERTER_DATALOGGER_%s", _id);
    sprintf(s_ap_ssid_temp, "%s %s", DEVICE_NAME, _id);

    ASSERT_BOOL(WiFi.mode(WIFI_AP_STA), TAG, "Failed to set WiFi to AP+STA mode");
    ASSERT_BOOL(WiFi.softAP(_ap_ssid, _ap_password), TAG, "Failed to start softAP");

    APP_LOGI(TAG, "AP Started");
    APP_LOGI(TAG, "AP SSID: %s", _ap_ssid);
    APP_LOGI(TAG, "AP Password: %s", _ap_password);

    IPAddress IP = WiFi.softAPIP();
    APP_LOGI(TAG, "AP IP: %s", IP.toString().c_str());

    setup_http_server_endpoints();
    ElegantOTA.begin(&s_http_server);
    s_http_server.begin();
    APP_LOGI(TAG, "HTTP Server started on port 80.");

    // 2. Cấu hình và khởi động WebSocket Server trên port 81
    s_ws.onEvent(on_websocket_event);
    s_ws_server.addHandler(&s_ws); // Gắn WebSocket handler vào server port 81
    s_ws_server.begin();
    APP_LOGI(TAG, "WebSocket Server started on port 81.");    

    delay(100);

    ASSERT_BOOL(LittleFS.begin(true), TAG, "Failed to mount LittleFS");

    APP_LOGI(TAG, "AP Mode with unified event handler is running.");
}



void setup_http_server_endpoints() {        

    s_http_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        APP_LOGI(TAG, "HTTP GET request on /");
        request->send(LittleFS, "/setting.html", "text/html");
    });    

    s_http_server.on("/device-info", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["model"] = PRODUCT_MODEL;
        doc["firmware"] = FIRMWARE_VERSION;
        doc["build"] = BUILD_NUMBER;
        
        String output;
        serializeJson(doc, output);
        
        request->send(200, "application/json", output);
    });

    s_http_server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = String(_wifi_ssid) + "," + String(s_ap_ssid_temp);
        request->send(200, "text/plain", data.c_str());
    });
    
    
    s_http_server.on("/scan_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        if(WiFi.scanComplete() == WIFI_SCAN_RUNNING){
            request->send(503, "text/plain", "Scan in progress");
            return;
        }
        WiFi.scanNetworks(true);
        request->send(200, "text/plain", "Scan started");
    });

    s_http_server.on("/updateWiFi", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("new_ssid", true) && request->hasParam("new_password", true)) {
            app_event_t evt;
            evt.source = EVT_SRC_HTTP_SERVER;
            evt.type   = HTTP_REQ_UPDATE_WIFI;
            
            strncpy(evt.data.http.param1, request->getParam("new_ssid", true)->value().c_str(), sizeof(evt.data.http.param1) - 1);
            strncpy(evt.data.http.param2, request->getParam("new_password", true)->value().c_str(), sizeof(evt.data.http.param2) - 1);

            evt.data.http.param1[sizeof(evt.data.http.param1) - 1] = '\0';
            evt.data.http.param2[sizeof(evt.data.http.param2) - 1] = '\0';

            if (xQueueSend(_web_manager_event_queue, &evt, 0) == pdPASS) {
                request->send(200, "text/plain", "OK. WiFi update request received.");
            } else {
                request->send(503, "text/plain", "Server busy.");
            }
        } else {
            request->send(400, "text/plain", "Bad Request.");
        }
    });

    s_http_server.on("/saveServer", HTTP_POST, 
        [](AsyncWebServerRequest *request){ /* Để trống */ }, 
        NULL, 
        
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) { 
                app_event_t evt;
                evt.source = EVT_SRC_HTTP_SERVER;
                evt.type   = HTTP_REQ_UPDATE_SERVER; 
                
                
                size_t len_to_copy = min(len, sizeof(evt.data.http.param1) - 1);
                memcpy(evt.data.http.param1, data, len_to_copy);
                evt.data.http.param1[len_to_copy] = '\0'; 

                
                if (xQueueSend(_web_manager_event_queue, &evt, 0) == pdPASS) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(503, "application/json", "{\"status\":\"error\", \"message\":\"Server busy\"}");
                }
            }
        }
    );
    
    s_http_server.on("/normal", HTTP_POST, [](AsyncWebServerRequest *request) {
        app_event_t evt;
        evt.source = EVT_SRC_HTTP_SERVER;
        evt.type   = HTTP_REQ_GO_NORMAL;
        
        if (xQueueSend(_web_manager_event_queue, &evt, 0) == pdPASS) {
            request->send(200, "text/plain", "OK. Switching to normal mode...");
        } else {
            request->send(503, "text/plain", "Server busy.");
        }
    });
    
    s_http_server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}


void on_websocket_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (_web_manager_event_queue == NULL) {
        return;
    }
    

    app_event_t evt;
    evt.source = EVT_SRC_WEBSOCKET;

    switch (type) {
        case WS_EVT_CONNECT:
        {
            APP_LOGI(TAG, "WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
            evt.type = WEBSOCKET_CONNECTED;
            evt.data.ws.client_num = client->id(); // Lưu client ID
            xQueueSend(_web_manager_event_queue, &evt, 0);
            break;
        }
        case WS_EVT_DISCONNECT:
        {
            APP_LOGI(TAG, "WebSocket client #%u disconnected", client->id());
            evt.type = WEBSOCKET_DISCONNECTED;
            evt.data.ws.client_num = client->id();
            xQueueSend(_web_manager_event_queue, &evt, 0);
            break;
        }
        case WS_EVT_DATA:
        {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                evt.type = WEBSOCKET_TEXT_RECEIVED;
                evt.data.ws.client_num = client->id();
                
                size_t len_to_copy = min(len, sizeof(evt.data.ws.payload) - 1);
                memcpy(evt.data.ws.payload, data, len_to_copy);
                evt.data.ws.payload[len_to_copy] = '\0';
                
                xQueueSend(_web_manager_event_queue, &evt, 0);
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void task_event_handler(void *pvParameters)
{
    app_event_t evt;
    const TickType_t xBlockTime = pdMS_TO_TICKS(1000);

    s_last_activity_tick = xTaskGetTickCount();


    for(;;)
    {
        if(xQueueReceive(_web_manager_event_queue, &evt, portMAX_DELAY) == pdPASS)
        {
            s_last_activity_tick = xTaskGetTickCount();
            APP_LOGI(TAG, "Activity detected, AP timeout reset.");

            switch (evt.source)
            {
            case EVT_SRC_HTTP_SERVER:
                switch (evt.type)
                {
                case HTTP_REQ_UPDATE_WIFI:
                {
                    APP_LOGI(TAG, "Processing 'Update Wifi' request.");
                    APP_LOGI(TAG, "NEW WIFI SSID: %s", evt.data.http.param1);
                    APP_LOGI(TAG, "NEW WIFI PASSWORD: %s", evt.data.http.param2);
                    strcpy(_wifi_ssid ,evt.data.http.param1);
                    strcpy(_wifi_password, evt.data.http.param2);

                    _wifi_ssid_length = sizeof(_wifi_ssid);
                    _wifi_password_length = sizeof(_wifi_password_length);
                    
                    memory_save_wifi_config();                    
                    
                    break;
                }                                
                case HTTP_REQ_UPDATE_SERVER:
                {
                    APP_LOGI(TAG, "Processing 'Update Server' request. Payload: %s", evt.data.http.param1);
    
                    JsonDocument doc; 
                    deserializeJson(doc, evt.data.http.param1);
                    
                    const char* mode = doc["serverMode"];
                    
                    if (strcmp(mode, "default") == 0) {
                        mqtt_reset_to_default_config();
                        
                        APP_LOGI(TAG, "Server mode: default");
                    } 
                    else if (strcmp(mode, "advanced") == 0) {
                        const char* server_host = doc["server"];
                        const char* topic = doc["topic"];
                        const char* authMethod = doc["authMethod"];

                        strcpy(_mqtt_auth_method, authMethod);
                        strcpy(_mqtt_topic_pub, topic);
                        strcpy(_mqtt_host, server_host);
                                                
                        APP_LOGI(TAG, "Server mode: advanced, Host: %s, Topic: %s", server_host, topic);
                        
                        const char* topic_sub = doc["topic_sub"];
                        if (topic_sub) {
                            strcpy(_mqtt_topic_sub, topic_sub);
                        }

                        if (strcmp(authMethod, "token") == 0) {
                            const char* token = doc["token"];                            
                            strcpy(_mqtt_token, token);                        
                            APP_LOGI(TAG, "Auth method: Token, Token: %s", token);
                        } 
                        else if (strcmp(authMethod, "userpass") == 0) {
                            const char* username = doc["username"];
                            const char* password_mqtt = doc["password_mqtt"];
                            
                            strcpy(_mqtt_username, username);
                            strcpy(_mqtt_password, password_mqtt);
                            

                            APP_LOGI(TAG, "Auth method: User/Pass, User: %s", username);
                        }
                    }

                    memory_save_mqtt_config();
                    break;
                }                                    
                case HTTP_REQ_GO_NORMAL:
                {
                    APP_LOGI(TAG, "Processing 'Go to Normal Mode' request.");
                    // delete every things related to web configuration & turn system into normal mode
                    // we need to turn off all mode related to ap host and websocket before we change to normal, cause
                    // those configurations are set downside the system (or we can think those configuration for taske 
                    // running on core 0 which for BLE and Wifi) so delete this task doesnt affect it.
                    app_system_event_t cmd_evt;
                    cmd_evt.command = CMD_SWITCH_TO_NORMAL_MODE;                    

                    xQueueSend(_system_cmd_queue, &cmd_evt, 0);
                                        
                    break;
                }                                    
                default:
                {
                    break;
                }                    

                }
                break;
            
            case EVT_SRC_WEBSOCKET:
                switch (evt.type) {
                    case WEBSOCKET_CONNECTED:
                    {
                        s_current_ws_client_num = evt.data.ws.client_num;
                        APP_LOGI(TAG, "Handler: WS Client #%u connected.", s_current_ws_client_num);
                        
                        JsonDocument json_doc;
                        json_doc["state"] = "request_wifi";
                        
                        send_ws_json_to_client(s_current_ws_client_num, json_doc);
                        
                        APP_LOGI(TAG, "Handler: Sent 'request_wifi'.");
                        s_ws_fsm_state = FSM_WS_WAITING_FOR_WIFI;
                        s_ws_last_sent_tick = xTaskGetTickCount(); 
                        break;
                    }

                    case WEBSOCKET_DISCONNECTED:
                    {
                        APP_LOGI(TAG, "Handler: WS Client #%u disconnected.", evt.data.ws.client_num);
                        // Nếu client đang hoạt động bị ngắt kết nối, reset FSM
                        if (evt.data.ws.client_num == s_current_ws_client_num) {
                            s_ws_fsm_state = FSM_WS_IDLE;
                            s_current_ws_client_num = 0;
                        }
                        break;
                    }

                    case WEBSOCKET_TEXT_RECEIVED:
                    {
                        APP_LOGI(TAG, "Handler: WS msg from #%u: %s", evt.data.ws.client_num, evt.data.ws.payload);

                        JsonDocument json_doc;
                        if (deserializeJson(json_doc, evt.data.ws.payload) == DeserializationError::Ok) {
                            const char* state = json_doc["state"];
                            if (state) {
                                // Xử lý dựa trên trạng thái FSM hiện tại
                                if (strcmp(state, "response_wifi") == 0 && s_ws_fsm_state == FSM_WS_WAITING_FOR_WIFI) {
                                    const char* ssid = json_doc["ssid"];
                                    const char* password = json_doc["password"];
                                    // memory_save_wifi_config(); // Gọi hàm lưu của bạn ở đây
                                    strcpy(_wifi_ssid, ssid);
                                    strcpy(_wifi_password, password);
                                    _wifi_ssid_length = sizeof(_wifi_ssid);
                                    _wifi_password_length = sizeof(_wifi_password_length);
                                    
                                    JsonDocument out_json_doc;
                                    out_json_doc["state"] = "send_topic";
                                    out_json_doc["topic"] = _mqtt_topic_pub; 
                                    send_ws_json_to_client(evt.data.ws.client_num, out_json_doc);
                                    
                                    APP_LOGI(TAG, "Handler: Sent 'send_topic'.");
                                    s_ws_fsm_state = FSM_WS_WAITING_FOR_ACK;
                                    s_ws_last_sent_tick = xTaskGetTickCount(); 
                                } else if (strcmp(state, "receive_topic") == 0 && s_ws_fsm_state == FSM_WS_WAITING_FOR_ACK) {
                                    APP_LOGI(TAG, "Handler: WiFi config via WebSocket complete.");
                                    s_ws_fsm_state = FSM_WS_FINISHED;
                                    app_event_t self_evt = { .source = EVT_SRC_HTTP_SERVER, .type = HTTP_REQ_GO_NORMAL };
                                    xQueueSend(_web_manager_event_queue, &self_evt, 0);
                                }
                            }
                        }
                        break;
                    }
                }
                break;
            }
        } else {
            // KHÔNG CÓ SỰ KIỆN MỚI -> KIỂM TRA CÁC LOẠI TIMEOUT
            TickType_t current_tick = xTaskGetTickCount();

            if ((current_tick - s_last_activity_tick) > pdMS_TO_TICKS(AP_TIMEOUT_MS)) {
                APP_LOGI(TAG, "AP mode timed out. Switching to normal mode.");
                app_event_t self_evt = { .source = EVT_SRC_HTTP_SERVER, .type = HTTP_REQ_GO_NORMAL };
                xQueueSend(_web_manager_event_queue, &self_evt, 0);
            }
            
            if (s_is_ap_logged_in && ((current_tick - s_login_timestamp_tick) > pdMS_TO_TICKS(LOGIN_TIMEOUT_MS))) {
                APP_LOGI(TAG, "Login session timed out.");
                s_is_ap_logged_in = false;
            }
            
            if (s_ws_fsm_state == FSM_WS_WAITING_FOR_WIFI && (current_tick - s_ws_last_sent_tick) > pdMS_TO_TICKS(3000)) {
                APP_LOGW(TAG, "Handler: Resending 'request_wifi' due to timeout.");
                JsonDocument json_doc;
                json_doc["state"] = "request_wifi";
                send_ws_json_to_client(s_current_ws_client_num, json_doc);
                s_ws_last_sent_tick = current_tick; // Reset bộ đếm giờ
            } else if (s_ws_fsm_state == FSM_WS_WAITING_FOR_ACK && (current_tick - s_ws_last_sent_tick) > pdMS_TO_TICKS(3000)) {
                APP_LOGW(TAG, "Handler: Resending 'send_topic' due to timeout.");
                JsonDocument json_doc;
                json_doc["state"] = "send_topic";
                json_doc["topic"] = _mqtt_topic_pub;
                send_ws_json_to_client(s_current_ws_client_num, json_doc);
                s_ws_last_sent_tick = current_tick;
            }
        }

    }

}


