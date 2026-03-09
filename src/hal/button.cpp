#include "button.h"

/* Static variables ---------------------------------------------------------*/
static uint32_t s_press_counter         = 0;
static uint32_t s_last_press_duration   = 0;

/* Public functions ---------------------------------------------------------*/
void button_init(void)
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    s_press_counter = 0;
    s_last_press_duration = 0;
}

void button_process(void)
{
    if (digitalRead(BUTTON_PIN) == LOW)
    {
        s_last_press_duration = 0;
        s_press_counter++;

        // Ngăn tràn
        if (s_press_counter > 10000)
            s_press_counter = 10000;
    }
    else
    {
        s_last_press_duration = s_press_counter;
        s_press_counter = 0;
    }
}

bool button_is_short_pressed(void)
{
    return s_last_press_duration >= BUTTON_SHORT_PRESS_TICK &&
           s_last_press_duration < BUTTON_LONG_PRESS_TICK;
}

bool button_is_long_pressed(void)
{
    return s_last_press_duration >= BUTTON_LONG_PRESS_TICK;
}

uint32_t button_get_press_duration(void)
{
    return s_last_press_duration * BUTTON_DEBOUNCE_MS;
}
