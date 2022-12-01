/**
 * @brief 台灯驱动源文件
 * @file light_drive.c
 * @version 1.0
 * @author zhh
 * @date 3/21/2022 10:29:33
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        :  3/21/2022 10:29:33
 *   Author      : zhh
 *   Modification: Created file
 */
/*----------------------------------------------*
 * includes							            *
 *----------------------------------------------*/
#include "light_drive.h"
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/
/**
* @brief light_driver_init
*
* @param [in] void
* @param [out] None
*
* @return void
*
* @history
* 1.Date		 : 3/21/2022 10:48:31
*	Author		 : zhh
*	Modification : Created function
*/
void light_driver_init(void)
{
	pwm_init_t pwm_config;

    Scu_SetDeviceGate(LIGHT_W_PWM_PIN_BASE,ENABLE);

    //W
    Scu_SetIOReuse(LIGHT_W_PWM_PIN_NAME,LIGHT_W_PWM_FUNCTION);
    Scu_SetDeviceGate(LIGHT_W_PWM_NAME, ENABLE);
    pwm_config.clk_sel = 0;
    pwm_config.freq = LIGHT_PWM_FREQ_HZ;
    pwm_config.duty = LIGHT_PWM_MAX_DUTY - 1;
    pwm_config.duty_max = LIGHT_PWM_MAX_DUTY;
    pwm_init(LIGHT_W_PWM_NAME, pwm_config);
    pwm_stop(LIGHT_W_PWM_NAME);
}


/**
* @brief light_set_duty
*
* @param [in] pwm_base_t pwm_base
* @param [in] unsigned int duty
* @param [out] None
*
* @return void
*
* @history
* 1.Date		 : 3/21/2022 10:49:10
*	Author		 : zhh
*	Modification : Created function
*/
void light_set_duty(pwm_base_t pwm_base, unsigned int duty)
{
	pwm_stop(pwm_base);
	pwm_set_duty(pwm_base, duty, LIGHT_PWM_MAX_DUTY);
	pwm_start(pwm_base);
}
