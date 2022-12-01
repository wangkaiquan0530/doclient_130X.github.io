#ifndef __CI_KEY_H
#define __CI_KEY_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"


#include "system_msg_deal.h"

#ifdef __cplusplus
extern "C"
{
#endif

//!按键消抖时间为2*10ms
#define KEY_SHORTPRESS_TIMER    (2)
//!按键长按间隔时间为150*10ms
#define KEY_LONGPRESS_TIMER     (100)

/**
 * @addtogroup key
 * @{
 */
void ci_key_gpio_init(void);
void ci_key_gpio_isr_handle(void);

void ci_key_adc_init(void);
void ci_adc_key_isr_handle(void);

void ci_key_touch_init(void);
void ci_key_touch_gpio_isr_handle(void);
void ci_key_touch_adc_isr_handle(void);

void ci_key_init(void);
void key_info_show(sys_msg_key_data_t *key_msg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif

