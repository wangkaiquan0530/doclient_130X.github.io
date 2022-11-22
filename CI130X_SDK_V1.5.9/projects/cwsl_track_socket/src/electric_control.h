/**
 * @brief  electric header file
 * @file electric_control.h
 * @version 1.0
 * @author wkq
 * @date 08/11/2022 15:57:45
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 08/11/2022 15:57:45
 *   Author      : wkq
 *   Modification: Created file
 */
#ifndef __ELECTRIC_CONTROL_H__
#define __ELECTRIC_CONTROL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * include                                      *
 *----------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ci130x_scu.h"
#include "ci130x_gpio.h"
#include "ci130x_dpmu.h"
#include "ci130x_pwm.h"

#define NVDATA_ID_ELECTRIC_STATUS   0x80000001



typedef enum
{
	ELECTRIC_RELAY_POWER_ON_OFF	    = 0x00, //上电关
	ELECTRIC_RELAY_POWER_ON_ON 	    = 0x01, //上电开
	ELECTRIC_RELAY_POWER_ON_MEMORY  = 0x02, //上电记忆
}electric_relay_power_on_state;

typedef struct
{
	uint8_t power_statue;//上一次开机设置的电源状态 关 开 记忆
	uint8_t onOff;//上一次开机的状态 开OR 关
}electric_relay_status_t;




//继电器
//PA6
#define ELECTRIC_RELAY_PIN6_NAME            PA6
#define ELECTRIC_RELAY_PIN_BASE            HAL_PA_BASE
#define ELECTRIC_RELAY_PIN6_NUMBER          pin_6
#define ELECTRIC_RELAY_PIN6_FUNCTION        FIRST_FUNCTION
//PA5
#define ELECTRIC_RELAY_PIN5_NAME            PA5
#define ELECTRIC_RELAY_PIN5_NUMBER          pin_5
#define ELECTRIC_RELAY_PIN5_FUNCTION        FIRST_FUNCTION

//PA2
#define SCR_ZERO_PIN_NAME     PA2
#define SCR_ZERO_PIN_BASE     HAL_PA_BASE
#define SCR_ZERO_PIN_NUMBER   pin_2
#define SCR_ZERO_IO_FUNCTION  FIRST_FUNCTION
#define SCR_ZERO_IO_IRQ       PA_IRQn

#define DELAY_CTR_TIMER        TIMER2
#define DELAY_CTR_TIMER_IRQn   TIMER2_IRQn
#define DELAY_CTR_ONEUS_COUNT  (get_apb_clk()/1000000)

void electric_relay_init(void);
void touch_control_electric_relay_realonff(void);//过零点操作IO口
void open_electric_relay_power(void);
void close_electric_relay_power(void);
void power_set_deal(uint32_t semantic_id);
static void zero_point_detect_irq_callback(void);
void electric_relay_switch_on(void);
void electric_relay_switch_off(void);
void enable_zero_point_detect_irq(void);
void close_electric_only_poweron_relay_power(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __ELECTRIC_CONTROL_H__ */
