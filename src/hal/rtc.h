#ifndef RTC_API_H
#define RTC_API_H

/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>  // hoặc thay bằng ErriezDS3231 nếu thích

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void rtc_init();
bool rtc_get_time_buffer(char* out_buffer, size_t max_len); 
bool rtc_set_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);
bool rtc_set_time_epoch(uint32_t epoch); // Hàm mới: Nạp giờ trực tiếp bằng Epoch time từ NTP
float rtc_get_temperature();

#endif // RTC_API_H


