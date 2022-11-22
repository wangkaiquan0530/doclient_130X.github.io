/**
 * @brief buzzer_driver.c header file
 * @file buzzer_driver.h
 * @version 1.0
 * @author zhh
 * @date 5/22/2021 22:50:16
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 5/22/2021 22:50:16
 *   Author      : zhh
 *   Modification: Created file
 */
#ifndef __BUZZER_DRIVER_H__
#define __BUZZER_DRIVER_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */
/*----------------------------------------------*
 * includes                                     *
 *----------------------------------------------*/
#include "ci130x_scu.h"
#include "ci130x_gpio.h"
#include "ci130x_dpmu.h"
#include "ci130x_pwm.h"
#include "ci130x_timer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/*----------------------------------------------*
 * struct										*
 *----------------------------------------------*/
#define BUZZER_PIN_NAME         PWM2_PAD
#define BUZZER_PIN_BASE         GPIO1
#define BUZZER_PIN_NUMBER       gpio_pin_3
#define BUZZER_PIN_FUNCTION     FIRST_FUNCTION

typedef struct
{
    unsigned short tone;
    unsigned short ms;
}stBuff;

typedef struct
{
    PinPad_Name PinName;
    gpio_base_t GpioBase;
    gpio_pin_t PinNum;
    IOResue_FUNCTION PwmFun;
    IOResue_FUNCTION IoFun;
    pwm_base_t PwmBase;
}stBeepCfg;

typedef struct
{
    char bInit;
    stBeepCfg cfg;
    xTimerHandle beepTimer;
    int TimerID;
    stBuff* pBuff;
    int BeepCnt;
    int BeepIndex;
}stBeepInfo;

/*----------------------------------------------*
 * Function declaration                         *
 *----------------------------------------------*/
void cmd_beep(void);
void power_on_beep(void);
void wake_up_beep(void);
void exit_wake_up_beep(void);
void timing_beep(void);

void buzzer_driver_init(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __BUZZER_DRIVER_H__ */
