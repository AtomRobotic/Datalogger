/* Includes ------------------------------------------------------------------*/
#include "memory_config.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
uint16_t         _address_version                        = 0x0000;
uint16_t         _address_id                             = 0x0000;
uint16_t         _address_reset_count                    = 0x0000;

uint16_t         _address_station_code                   = 0x0000;

uint16_t         _address_wifi_ssid                      = 0x0000;
uint16_t         _address_wifi_password                  = 0x0000;

uint16_t         _address_ap_ssid                        = 0x0000;
uint16_t         _address_ap_password                    = 0x0000;

uint16_t         _address_mqtt_host                      = 0x0000;
uint16_t         _address_mqtt_username                  = 0x0000;
uint16_t         _address_mqtt_password                  = 0x0000;
uint16_t         _address_mqtt_topic_sub                 = 0x0000;
uint16_t         _address_mqtt_topic_pub                 = 0x0000;
uint16_t         _address_mqtt_port                      = 0x0000;
uint16_t         _address_mqtt_token                     = 0x0000;
uint16_t         _address_mqtt_auth_method               = 0x0000;

/* Functions -----------------------------------------------------------------*/
void set_address(uint16_t* startAddr, uint16_t* sourceAddr, uint16_t numOfAddr, uint16_t sizeOfPara)
{
  uint16_t _i;
  uint16_t _addr = *startAddr;

  for (_i = 0; _i < numOfAddr; _i++)
  {
    *(sourceAddr + _i) = _addr;
    _addr              = _addr + sizeOfPara;
  }
  *startAddr = _addr;
}


// Initialization & update
void memory_init(void)
{
  uint16_t _addr = 0x0000;

  // device setting
  set_address(&_addr, &_address_version, 1, sizeof(_version));
  set_address(&_addr, &_address_id, 1, sizeof(_id));
  set_address(&_addr, &_address_reset_count, 1, sizeof(reset_count));
  set_address(&_addr, &_address_station_code, 1, sizeof(_station_code));

  // wifi setting
  set_address(&_addr, &_address_wifi_ssid, 1, sizeof(_wifi_ssid));
  set_address(&_addr, &_address_wifi_password, 1, sizeof(_wifi_password));

  // mqtt setting
  set_address(&_addr, &_address_mqtt_host, 1, sizeof(_mqtt_host));
  set_address(&_addr, &_address_mqtt_username, 1, sizeof(_mqtt_username));
  set_address(&_addr, &_address_mqtt_password, 1, sizeof(_mqtt_password));
  set_address(&_addr, &_address_mqtt_topic_sub, 1, sizeof(_mqtt_topic_sub));
  set_address(&_addr, &_address_mqtt_topic_pub, 1, sizeof(_mqtt_topic_pub));
  set_address(&_addr, &_address_mqtt_token, 1, sizeof(_mqtt_token));
  set_address(&_addr, &_address_mqtt_auth_method, 1, sizeof(_mqtt_auth_method));
  set_address(&_addr, &_address_mqtt_port, 1, sizeof(_mqtt_port));  

  eeprom_init();
}

void memory_update_version(void)
{
  EepromWrite16b(_address_version, _version);
}

// WiFi
void memory_save_wifi_config(void)
{
  EepromWriteString(_address_wifi_ssid, _wifi_ssid, sizeof(_wifi_ssid));
  EepromWriteString(_address_wifi_password, _wifi_password, sizeof(_wifi_password));
}

void memory_load_wifi_config(void)
{  
  memset(_wifi_ssid, 0, sizeof(_wifi_ssid));
  memset(_wifi_password, 0, sizeof(_wifi_password));

  
  EepromReadString(_address_wifi_ssid, _wifi_ssid, sizeof(_wifi_ssid) - 1);
  EepromReadString(_address_wifi_password, _wifi_password, sizeof(_wifi_password) - 1);
  
  
  _wifi_ssid[sizeof(_wifi_ssid) - 1] = '\0';
  _wifi_password[sizeof(_wifi_password) - 1] = '\0';

  
  _wifi_ssid_length     = strlen(_wifi_ssid);
  _wifi_password_length = strlen(_wifi_password);
}


// MQTT
void memory_save_mqtt_config(void)
{
  EepromWriteString(_address_mqtt_host, _mqtt_host, sizeof(_mqtt_host));
  EepromWriteString(_address_mqtt_username, _mqtt_username, sizeof(_mqtt_username));
  EepromWriteString(_address_mqtt_password, _mqtt_password, sizeof(_mqtt_password));
  EepromWriteString(_address_mqtt_topic_sub, _mqtt_topic_sub, sizeof(_mqtt_topic_sub));
  EepromWriteString(_address_mqtt_topic_pub, _mqtt_topic_pub, sizeof(_mqtt_topic_pub));
  EepromWriteString(_address_mqtt_token, _mqtt_token, sizeof(_mqtt_token));
  EepromWriteString(_address_mqtt_auth_method, _mqtt_auth_method, sizeof(_mqtt_auth_method));
  EepromWrite32b(_address_mqtt_port, _mqtt_port);
}

void memory_load_mqtt_config(void)
{
  // Dọn sạch các biến global trước khi ghi đè
  memset(_mqtt_host, 0, sizeof(_mqtt_host));
  memset(_mqtt_username, 0, sizeof(_mqtt_username));
  memset(_mqtt_password, 0, sizeof(_mqtt_password));
  memset(_mqtt_topic_sub, 0, sizeof(_mqtt_topic_sub));
  memset(_mqtt_topic_pub, 0, sizeof(_mqtt_topic_pub));

  // Đọc trực tiếp vào các biến toàn cục
  EepromReadString(_address_mqtt_host, _mqtt_host, sizeof(_mqtt_host) - 1);
  EepromReadString(_address_mqtt_username, _mqtt_username, sizeof(_mqtt_username) - 1);
  EepromReadString(_address_mqtt_password, _mqtt_password, sizeof(_mqtt_password) - 1);
  EepromReadString(_address_mqtt_topic_sub, _mqtt_topic_sub, sizeof(_mqtt_topic_sub) - 1);
  EepromReadString(_address_mqtt_topic_pub, _mqtt_topic_pub, sizeof(_mqtt_topic_pub) - 1);
  EepromReadString(_address_mqtt_token, _mqtt_token, sizeof(_mqtt_token) - 1);
  EepromReadString(_address_mqtt_auth_method, _mqtt_auth_method, sizeof(_mqtt_auth_method) - 1);

  // Đọc giá trị số
  _mqtt_port = EepromRead32b(_address_mqtt_port);
}

// Access Point
void memory_save_ap_config(void)
{
  EepromWriteString(_address_ap_password, _ap_password, sizeof(_ap_password));
}

void memory_load_ap_config(void)
{
  char read_ap_password[65];
  EepromReadString(_address_ap_password, read_ap_password, 65);

  strcpy(_ap_password, read_ap_password);

}

// Device ID & Station Code
void memory_save_device_id(void)
{ 
  EepromWriteString(_address_id, _id, 11);
}

void memory_load_device_id(void)
{
    memset(_id, 0, sizeof(_id));
    // Đọc tối đa (sizeof(_id) - 1) byte để chừa chỗ cho ký tự null
    EepromReadString(_address_id, _id, sizeof(_id) - 1); 
    // Không cần strncpy nữa vì đã đọc trực tiếp
}

void memory_save_station_code(void)
{
  EepromWriteString(_address_station_code, _station_code, sizeof(_station_code));
}

void memory_load_station_code(void)
{
  memset(_station_code, 0, sizeof(_station_code));
  EepromReadString(_address_station_code, _station_code, sizeof(_station_code) - 1);
}


// Reset Count
void memory_save_reset_count(void)
{
  EepromWrite16b(_address_reset_count, reset_count);
}

void memory_load_reset_count(void)
{
  reset_count = EepromRead16b(_address_reset_count);
}