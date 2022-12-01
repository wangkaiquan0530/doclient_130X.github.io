/**
 * @brief light_drive.c header file
 * @file light_drive.h
 * @version 1.0
 * @author zhh
 * @date 12/23/2021 10:30:22
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 12/23/2021 10:30:22
 *   Author      : zhh
 *   Modification: Created file
 */
#ifndef __LIGHT_DRIVE_H__
#define __LIGHT_DRIVE_H__
/*----------------------------------------------*
 * includes                                     *
 *----------------------------------------------*/
#include "ci112x_pwm.h"
#include "ci112x_scu.h"
#include "ci112x_gpio.h"
#include "ci112x_scu.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
//IO口：Pin16脚
// 1.GPIO1_2
// 2.PWM1 Output
// 3.EXT_INT[1]
#define LIGHT_W_PWM_PIN_NAME       PWM1_PAD
#define LIGHT_W_PWM_PIN_BASE       HAL_GPIO1_BASE
#define LIGHT_W_PWM_FUNCTION       SECOND_FUNCTION
#define LIGHT_W_PWM_NAME           PWM1

//wkq 测试
// #define LIGHT_W_PWM_PIN_NAME       PWM3_PAD
// #define LIGHT_W_PWM_PIN_BASE       HAL_GPIO1_BASE
// #define LIGHT_W_PWM_FUNCTION       SECOND_FUNCTION
// #define LIGHT_W_PWM_NAME           PWM3
//wkq 测试



#define LIGHT_PWM_FREQ_HZ          (2*1000)    //PWM频率，单位HZ
#define LIGHT_PWM_MAX_DUTY         (1000)      //PWM最大占空比

/*----------------------------------------------*
 * enum                                         *
 *----------------------------------------------*/
typedef enum
{
	LIGHT_W_PWM_CHANNEL = 0,
	LIGHT_MAX_PWM_CHANNEL,
}light_pwm_channel_e;

/*----------------------------------------------*
 * struct                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * function declaration                         *
 *----------------------------------------------*/
void light_driver_init(void);
void light_set_duty(pwm_base_t pwm_base, unsigned int duty);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __LIGHT_DRIVE_H__ */
