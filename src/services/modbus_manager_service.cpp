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

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t period = READ_INTERVAL_MS; 
  static uint32_t warning_timeslot = 0;         

  // --- THÊM BIẾN ĐẾM CHO TÍNH TOÁN TRUNG BÌNH ---
  int cycle_count = 0;
  int success_count = 0;
  // Tính số lần đọc cần thiết để đạt 30 giây (Ví dụ: 30000ms / 5000ms = 6 lần)
  const int TARGET_CYCLES = 30000 / READ_INTERVAL_MS; 

  for(;;)
  {    
    APP_LOGI(TAG, "Wait for connecting to broker...");
    xEventGroupWaitBits(_normal_mode_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);      
    APP_LOGI(TAG, "Broker connected and start reading data"); 
    vTaskDelay(500);
    APP_LOGI(TAG, "Wait for connecting sync time from server");
    xEventGroupWaitBits(_normal_mode_event_group, NTP_SYNCED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    // led_b_on();
    vTaskDelay(500);
    // all_led_off();              

    while (MQTT_CONNECTED_BIT)
    {
      cycle_count++;
      APP_LOGI(TAG, "Reading data from inverter... (Cycle %d/%d)", cycle_count, TARGET_CYCLES);      
      
      // Nếu là lần đọc thành công đầu tiên trong chu kỳ 30s, is_first = true để khởi tạo doc
      bool is_first = (success_count == 0);
      bool success = read_and_store_data(doc, is_first);      

      if(success)
      { 
        success_count++; // Tăng biến đếm số lần đọc thành công
        warning_timeslot = 0; 
        EventBits_t bits = xEventGroupGetBits(_normal_mode_event_group);
        if ((bits & MQTT_CONNECTED_BIT) != 0) 
        {
            // Nếu có cờ mạng -> Hệ thống hoàn hảo
            set_system_led_state(STATE_NORMAL); 
        } 
        else 
        {
            // Nếu mất cờ mạng -> Lỗi Server nhưng Modbus vẫn chạy
            set_system_led_state(STATE_ERROR2); 
        }
      }
      else
      {            
        set_system_led_state(STATE_ERROR1);
        warning_timeslot++;
        APP_LOGW(TAG, "%d time: Sensor doesn't response in this timeslot.", warning_timeslot);
      }

      // --- KIỂM TRA ĐÃ ĐỦ 30 GIÂY CHƯA ---
      if (cycle_count >= TARGET_CYCLES) 
      {
        if (success_count > 0) 
        {
          APP_LOGI(TAG, "30s elapsed. Calculating average and sending data...");
          
          // 1. Chia trung bình tất cả các giá trị đã cộng dồn trong doc
          for (JsonPair kv : doc.as<JsonObject>()) {
              if (kv.value().is<JsonObject>()) {
                  JsonObject obj = kv.value().as<JsonObject>();
                  if (!obj["value"].isNull()) {
                      float sum = obj["value"].as<float>();
                      obj["value"] = sum / success_count; // Chia cho số lần đọc THÀNH CÔNG thực tế
                  }
              }
          }

          // 2. Gắn Timestamp tại thời điểm gửi đi
          char formatted_date_time[25];
          strncpy(formatted_date_time, ntp_time_get_buffer(), sizeof(formatted_date_time));
          formatted_date_time[sizeof(formatted_date_time) - 1] = '\0';
          doc["timestamp"] = formatted_date_time;        
          APP_LOGI(TAG, "TIMESTAMP: %s", formatted_date_time);

          // 3. Đóng gói và đẩy dữ liệu
          serializeJson(doc, mqtt_payload);
          backup_manager_handle_data(mqtt_payload.c_str());
          
          // 4. Xóa bộ đệm JSON chuẩn bị cho chu kỳ 30s tiếp theo
          doc.clear();  
        }
        else 
        {
          APP_LOGW(TAG, "All reads in 30s failed. Nothing to send.");
        }
        
        // Reset lại biến đếm chu kỳ
        cycle_count = 0;
        success_count = 0;
      }

      if(warning_timeslot == ERROR_THRESHOLD)
      {
        APP_LOGE(TAG, "Something wrong with your sensor, please check the data connection");
        break;
      }

      vTaskDelayUntil(&xLastWakeTime, period);    
    }

    APP_LOGI(TAG, "MQTT Broker disconnect, start reconnecting...");
    set_system_led_state(STATE_ERROR2);
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
  // led_a_toggle();
  APP_LOGI(TAG, "Error code: %0X", result);    
  if (result == s_modbus_node.ku8MBSuccess)
  {
    // led_b_toggle();
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

bool read_and_store_data(JsonDocument& doc, bool is_first)
{
  uint16_t data[64];
  bool     ok;
  bool     hasError = false;

  struct ReadInfo {
    int         offset;
    int         size;
    const char* label;
  } reads[] = {
      {100 + 31 * 2, 32, "GridEnergy"},          {100 + 31 * 3, 32, "SolarBasic"},     {100 + 31 * 4, 32, "GridVoltage"},
      {100 + 31 * 5, 40, "BatteryLoadInverter"}, {100 + 31 * 6, 32, "PVPowerBattery"},
  };

  // Lambda nâng cấp: Nếu is_first thì gán đè, nếu không thì CỘNG DỒN
  auto set = [&](const char* key, const char* name, const char* unit, float value)
  {
    if (is_first) {
      doc[key]["value"] = value;
      doc[key]["name"]  = name;
      doc[key]["unit"]  = unit;
    } else {
      float current_val = doc[key]["value"].as<float>();
      doc[key]["value"] = current_val + value; // Cộng dồn giá trị
    }
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

        // Lấy giá trị data thô hiện tại để tính toán, tránh lỗi cộng dồn kép
        float current_gridPower = (int16_t)data[13] + (int16_t)data[11];
        set("gridPower", "Grid Power", "W", current_gridPower);
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

        float current_loadPower = (int16_t)data[21] + (int16_t)data[22];
        set("loadPower", "Total Load Power", "W", current_loadPower);

        // Xử lý điều kiện riêng cho Inverter L2 Voltage
        float current_invL2_V = data[2] / 10.0;
        float current_invI2 = data[10] / 100.0;
        float current_invP2 = (int16_t)data[19];
        
        if (current_invI2 == 0 && current_invP2 == 0) {
            current_invL2_V = 0;
        }

        set("inverterL1_V", "Inverter L1 Voltage", "V", data[1] / 10.0);
        set("inverterI1", "Inverter L1 Current", "A", data[9] / 100.0);
        set("inverterINV_P1", "Inverter L1 Power", "W", (int16_t)data[18]);
        set("inverterL2_V", "Inverter L2 Voltage", "V", current_invL2_V);
        set("inverterI2", "Inverter L2 Current", "A", current_invI2);
        set("inverterINV_P2", "Inverter L2 Power", "W", current_invP2);

        float current_inverterPower = (int16_t)data[18] + (int16_t)data[19];
        set("inverterPower", "Inverter Power", "W", current_inverterPower);
        set("inverterFreq", "Inverter Frequency", "Hz", data[37] / 100.0);
      }
      else if (strcmp(r.label, "PVPowerBattery") == 0)
      {
        set("pv1Power", "PV1 Power", "W", (int16_t)data[0]);
        set("pv2Power", "PV2 Power", "W", (int16_t)data[1]);
        set("pv3Power", "PV3 Power", "W", (int16_t)data[2]);

        float current_solarPower = (int16_t)data[0] + (int16_t)data[1] + (int16_t)data[2];
        set("solarPower", "Total Solar Power", "W", current_solarPower);

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


