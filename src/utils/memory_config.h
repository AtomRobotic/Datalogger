#ifndef __MEMORY_CONFIG_H_
#define __MEMORY_CONFIG_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
// Initialization & update
void memory_init(void);
void memory_update_version(void);

// WiFi
void memory_save_wifi_config(void);
void memory_load_wifi_config(void);

// MQTT
void memory_save_mqtt_config(void);
void memory_load_mqtt_config(void);

// Access Point
void memory_save_ap_config(void);
void memory_load_ap_config(void);

// Device ID & Station Code
void memory_save_device_id(void);
void memory_load_device_id(void);
void memory_save_station_code(void);
void memory_load_station_code(void);

// Reset Count
void memory_save_reset_count(void);
void memory_load_reset_count(void);

// Supported function
static void set_address(uint16_t* startAddr, uint16_t* sourceAddr, uint16_t numOfAddr, uint16_t sizeOfPara);

#endif // __MEMORY_CONFIG_H_