#ifndef __LED_H_
#define __LED_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_assert.h"
#include "app_log.h"

/* Define --------------------------------------------------------------------*/
#define LED_A_PIN 32
#define LED_B_PIN 33
#define LED_ON    0
#define LED_OFF   1

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

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




#endif // __LED_H_