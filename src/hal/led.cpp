/* Includes ------------------------------------------------------------------*/
#include "led.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static uint8_t led_a_state = 0;
static uint8_t led_b_state = 0;

/* Functions -----------------------------------------------------------------*/
void led_init(void)
{
    pinMode(LED_A_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    led_a_state = LED_OFF;
    led_b_state = LED_OFF;
}

void led_a_on(void)
{
    led_a_state = LED_ON;
    digitalWrite(LED_A_PIN, led_a_state);
}

void led_b_on(void)
{
    led_b_state = LED_ON;
    digitalWrite(LED_B_PIN, led_b_state);
}

void led_a_off(void)
{
    led_a_state = LED_OFF;
    digitalWrite(LED_A_PIN, led_a_state);
}

void led_b_off(void)
{
    led_b_state = LED_OFF;
    digitalWrite(LED_B_PIN, led_b_state);
}

void led_a_toggle(void)
{
    led_a_state = (led_a_state + 1) % 2;
    digitalWrite(LED_A_PIN, led_a_state);
}

void led_b_toggle(void)
{
    led_b_state = (led_b_state + 1) % 2;
    digitalWrite(LED_B_PIN, led_b_state);
}

void all_led_on(void)
{
    led_a_on();
    led_b_on();
}

void all_led_off(void)
{
    led_a_off();
    led_b_off();
}
