#ifndef __MODBUS_MANAGER_SERVICE_
#define __MODBUS_MANAGER_SERVICE_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_log.h"
#include "app_assert.h"
#include "led.h"
#include "ntp_sync_time.h"
#include "backup_data.h"

/* Define --------------------------------------------------------------------*/
#define SLAVE_ID            1
#define BAUD_RATE           9600
#define READ_INTERVAL_MS    pdMS_TO_TICKS(10000);
#define RS485_EN            13
#define ERROR_THRESHOLD     500     

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void pre_transmission();
void post_transmission();
bool read_registers(uint16_t start_address, uint16_t* data, uint16_t quantity, const char* label, bool& success_flag);
bool read_and_store_data(JsonDocument& doc);

void modbus_manager_task(void *pvParameters);
void init_modbus_manager(void);



#endif // __MODBUS_MANAGER_SERVICE_