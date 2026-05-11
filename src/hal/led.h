#ifndef __LED_H_
#define __LED_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_assert.h"
#include "app_log.h"

/* Define --------------------------------------------------------------------*/
#define LED_A_PIN 33
#define LED_B_PIN 32
#define LED_ON    0
#define LED_OFF   1

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Enum --------------------------------------------------------------------*/
enum SystemLedState {
    STATE_AP_CONFIG,
    STATE_NORMAL,
    STATE_ERROR1,
    STATE_ERROR2
};

/* Functions -----------------------------------------------------------------*/
void led_init(void);
void led_a_on(void);
void led_b_on(void);
void led_a_off(void);
void led_b_off(void);
void led_a_toggle(void);
void led_b_toggle(void);
void all_led_on(void);
void all_led_off(void);
void set_system_led_state(SystemLedState new_state);



#endif // __LED_H_