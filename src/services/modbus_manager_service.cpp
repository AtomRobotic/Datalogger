/* Includes ------------------------------------------------------------------*/
#include "modbus_manager_service.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
HardwareSerial                                  s_serial_port(2);
ModbusMaster                                    s_modbus_node;
static JsonDocument                             doc;
static String                                   mqtt_payload; 
mqtt_message_t                                  mqtt_msg;

TaskHandle_t                    _modbus_manager_handler_t   = NULL;
static const char*              TAG                         = "MODBUS MANAGER";


/* Functions -----------------------------------------------------------------*/
void init_modbus_manager(void)
{
  APP_LOGI(TAG, "Modbus manager init...");

  s_serial_port.begin(BAUD_RATE, SERIAL_8N1, 16, 17);
  pinMode(RS485_EN, OUTPUT);

  s_modbus_node.begin(SLAVE_ID, s_serial_port);
  s_modbus_node.preTransmission(pre_transmission);
  s_modbus_node.postTransmission(post_transmission);
  

  BaseType_t task_result = xTaskCreatePinnedToCore(
    modbus_manager_task,
    "MODBUS MANAGER",
    1024 * 10,
    NULL,
    20,
    &_modbus_manager_handler_t,
    1
  );

  ASSERT_BOOL(task_result, TAG, "Create modbus manager task failed.");
    
}

void modbus_manager_task(void *pvParameters)
{
  APP_LOGI(TAG, "Modbus manager started.");

  
  static uint32_t warning_timeslot = 0;         

  for(;;)
  {    
    APP_LOGI(TAG, "Wait for connecting to broker...");
    xEventGroupWaitBits(_normal_mode_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);      
    APP_LOGI(TAG, "Broker connected and start reading data");
    led_a_on(); 
    vTaskDelay(500);
    APP_LOGI(TAG, "Wait for connecting sync time from server");
    xEventGroupWaitBits(_normal_mode_event_group, NTP_SYNCED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    led_b_on();
    vTaskDelay(500);
    all_led_off();              

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t period = READ_INTERVAL_MS; 

    while (xEventGroupGetBits(_normal_mode_event_group) & MQTT_CONNECTED_BIT)
    {
      APP_LOGI(TAG, "Get data from inverter and send to MQTT broker...");      
      bool success = read_and_store_data(doc);      

      char formatted_date_time[25];
      strncpy(formatted_date_time, ntp_time_get_buffer(), sizeof(formatted_date_time));
      formatted_date_time[sizeof(formatted_date_time) - 1] = '\0';

      APP_LOGI(TAG, "TIMESTAMP: %s", formatted_date_time);

      if(success)
      { 
        char formatted_date_time[25];
        strncpy(formatted_date_time, ntp_time_get_buffer(), sizeof(formatted_date_time));
        formatted_date_time[sizeof(formatted_date_time) - 1] = '\0';
        doc["timestamp"] = formatted_date_time;        

        serializeJson(doc, mqtt_payload);
              
        //        // Cấp phát tĩnh trên Stack (Đảm bảo luôn có đủ RAM cho Modbus)
        // strncpy(mqtt_msg.topic, _mqtt_topic_pub, sizeof(mqtt_msg.topic) - 1);
        // strncpy(mqtt_msg.payload, mqtt_payload.c_str(), sizeof(mqtt_msg.payload) - 1);

        // xQueueSend(_mqtt_outgoing_queue, &mqtt_msg, 0); 



        //         // Cấp phát động để truyền qua Queue
        mqtt_message_t *p_msg = (mqtt_message_t *)malloc(sizeof(mqtt_message_t));
        if (p_msg != NULL)
        {
          strncpy(p_msg->topic, _mqtt_topic_pub, sizeof(p_msg->topic) - 1);
          strncpy(p_msg->payload, mqtt_payload.c_str(), sizeof(p_msg->payload) - 1);
          p_msg->topic[sizeof(p_msg->topic) - 1] = '\0';
          p_msg->payload[sizeof(p_msg->payload) - 1] = '\0';

          if (xQueueSend(_mqtt_outgoing_queue, &p_msg, 0) != pdPASS)
          {
            APP_LOGW(TAG, "Queue full, Modbus data dropped!");
            free(p_msg); // Nếu Queue đầy thì phải giải phóng RAM ngay
          }
        }
        else
        {
          APP_LOGE(TAG, "Malloc failed in Modbus Task!");
        }
        
        // backup_manager_handle_data(mqtt_payload.c_str());
         
        doc.clear();  
        // warning_timeslot = 0; // Reset lại nếu đọc thành công
      }
      else
      {            
        warning_timeslot++;
        APP_LOGW(TAG, "%d time: Sensor doesn't response in this timeslot.", warning_timeslot);
      }

      if(warning_timeslot == ERROR_THRESHOLD)
      {
        APP_LOGE(TAG, "Something wrong with your sensor, please check the data connection");
        break;
      }

      vTaskDelayUntil(&xLastWakeTime, period);    
      
    }

    APP_LOGI(TAG, "MQTT Broker disconnect, start reconnecting...");


    vTaskDelay(pdMS_TO_TICKS(15000));            
  }

  APP_LOGI(TAG, "Self deleting, check the data connection and restart system.");

  _modbus_manager_handler_t = NULL;

  vTaskDelete(NULL);

}

void pre_transmission()
{   
    digitalWrite(RS485_EN, HIGH);
    delay(2);
}

void post_transmission()
{
    delayMicroseconds(800);
    digitalWrite(RS485_EN, LOW);
}

bool read_registers(uint16_t start_address, uint16_t* data, uint16_t quantity, const char* label, bool& success_flag)
{    
  uint8_t result = s_modbus_node.readHoldingRegisters(start_address, quantity);
  led_a_toggle();
  APP_LOGI(TAG, "Error code: %0X", result);    
  if (result == s_modbus_node.ku8MBSuccess)
  {
    led_b_toggle();
    for (uint16_t i = 0; i < quantity; i++)
    {
      data[i] = s_modbus_node.getResponseBuffer(i);
    }          
    success_flag = true;
    return true;
  }
  else
  {
    success_flag = false;
    return false;
  }
}

bool read_and_store_data(JsonDocument& doc)
{
  uint16_t data[64];
  bool     ok;
  bool     hasError = false;

  struct ReadInfo
  {
    int         offset;
    int         size;
    const char* label;
  } reads[] = {
      {100 + 31 * 2, 32, "GridEnergy"},          {100 + 31 * 3, 32, "SolarBasic"},     {100 + 31 * 4, 32, "GridVoltage"},
      {100 + 31 * 5, 40, "BatteryLoadInverter"}, {100 + 31 * 6, 32, "PVPowerBattery"},
  };

  auto set = [&](const char* key, const char* name, const char* unit, float value)
  {
    doc[key]["value"] = value;
    doc[key]["name"]  = name;
    doc[key]["unit"]  = unit;
  };

  for (const auto& r : reads)
  {
    if (read_registers(r.offset, data, r.size, r.label, ok) && ok)
    {
      if (strcmp(r.label, "GridEnergy") == 0)
      {
        set("gridSellToday_kWh", "Grid Sell Today", "kWh", data[13] / 10.0);
        set("gridSellTotal_kWh", "Grid Sell Total", "kWh", data[15] / 10.0);
        set("gridBuyToday_kWh", "Grid Buy Today", "kWh", data[14] / 10.0);
        set("gridBuyTotal_kWh", "Grid Buy Total", "kWh", data[16] / 10.0);
        set("gridFreq", "Grid Frequency", "Hz", data[17] / 100.0);

        set("loadToday", "Load Today", "kWh", data[22] / 10.0);
        set("totalLoad", "Total Load", "kWh", data[23] / 10.0);
      }
      else if (strcmp(r.label, "SolarBasic") == 0)
      {
        set("totalKWhSolar", "Total Solar", "kWh", data[3] / 10.0);
        set("kWhSolarToday", "Solar Today", "kWh", data[15] / 10.0);
        set("pv1Voltage", "PV1 Voltage", "V", data[16] / 10.0);
        set("pv1Current", "PV1 Current", "A", data[17] / 10.0);
        set("pv2Voltage", "PV2 Voltage", "V", data[18] / 10.0);
        set("pv2Current", "PV2 Current", "A", data[19] / 10.0);
        set("pv3Voltage", "PV3 Voltage", "V", data[20] / 10.0);
        set("pv3Current", "PV3 Current", "A", data[21] / 10.0);
      }
      else if (strcmp(r.label, "GridVoltage") == 0)
      {
        set("gridCT1", "CT1", "W", (int16_t)data[13]);
        set("gridCT2", "CT2", "W", (int16_t)data[11]);
        set("gridLD1", "Load 1", "W", (int16_t)data[10]);
        set("gridLD2", "Load 2", "W", (int16_t)data[12]);
        set("gridL1", "Grid L1", "V", data[28] / 10.0);
        set("gridL2", "Grid L2", "V", data[29] / 10.0);

        float gridPower = doc["gridCT1"]["value"].as<float>() + doc["gridCT2"]["value"].as<float>();
        set("gridPower", "Grid Power", "W", gridPower);
      }
      else if (strcmp(r.label, "BatteryLoadInverter") == 0)
      {
        set("batteryTemp", "Battery Temp", "°C", data[27] / 10.0);
        set("batteryVoltage", "Battery Voltage", "V", data[28] / 100.0);
        set("batterySOC", "Battery SOC", "%", data[29]);

        set("loadL1", "Load Phase 1 Voltage", "V", data[2] / 10.0);
        set("loadP1", "Load Phase 1 Power", "W", (int16_t)data[21]);
        set("loadL2", "Load Phase 2 Voltage", "V", data[3] / 10.0);
        set("loadP2", "Load Phase 2 Power", "W", (int16_t)data[22]);

        float loadPower = doc["loadP1"]["value"].as<float>() + doc["loadP2"]["value"].as<float>();
        set("loadPower", "Total Load Power", "W", loadPower);

        set("inverterL1_V", "Inverter L1 Voltage", "V", data[1] / 10.0);
        set("inverterI1", "Inverter L1 Current", "A", data[9] / 100.0);
        set("inverterINV_P1", "Inverter L1 Power", "W", (int16_t)data[18]);
        set("inverterL2_V", "Inverter L2 Voltage", "V", data[2] / 10.0);
        set("inverterI2", "Inverter L2 Current", "A", data[10] / 100.0);
        set("inverterINV_P2", "Inverter L2 Power", "W", (int16_t)data[19]);

        if (doc["inverterI2"]["value"].as<float>() == 0 && doc["inverterINV_P2"]["value"].as<float>() == 0)
          doc["inverterL2_V"]["value"] = 0;

        float inverterPower = doc["inverterINV_P1"]["value"].as<float>() + doc["inverterINV_P2"]["value"].as<float>();
        set("inverterPower", "Inverter Power", "W", inverterPower);
        set("inverterFreq", "Inverter Frequency", "Hz", data[37] / 100.0);
      }
      else if (strcmp(r.label, "PVPowerBattery") == 0)
      {
        set("pv1Power", "PV1 Power", "W", (int16_t)data[0]);
        set("pv2Power", "PV2 Power", "W", (int16_t)data[1]);
        set("pv3Power", "PV3 Power", "W", (int16_t)data[2]);

        float solarPower = doc["pv1Power"]["value"].as<float>() + doc["pv2Power"]["value"].as<float>() + doc["pv3Power"]["value"].as<float>();
        set("solarPower", "Total Solar Power", "W", solarPower);

        set("batteryCurrent", "Battery Current", "A", (int16_t)data[5] / 100.0);
        set("batteryPower", "Battery Power", "W", (int16_t)data[4]);
      }
    }
    else
    {
      hasError = true;
    }
  }

  return !hasError;
}


