#ifndef __INPUT_PROCESSING_H_
#define __INPUT_PROCESSING_H_
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_log.h"
#include "app_assert.h"
#include "button.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void init_input_processing(void);
void task_input_processing(void *pvParameters);

#endif // __INPUT_PROCESSING_H_
