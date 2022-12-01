/**
 * @brief 台灯控制源文件
 * @file light_control.c
 * @version 1.0
 * @author zhh
 * @date 12/24/2021 16:19:52
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 12/24/2021 16:19:52
 *   Author      : zhh
 *   Modification: Created file
 */
/*----------------------------------------------*
 * includes 									*
 *----------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "light_control.h"
#include "system_msg_deal.h"
#include "ci_log.h"

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/
extern uint8_t g_ac_power_times;
/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
static void light_timer_irq_handler(void);
//static void light_timer_init(void);

static void light_change_color_timer_callback(TimerHandle_t p);
static void light_status_init(void);

static void light_auto_off_timer_callback(TimerHandle_t xTimer);
static uint8_t light_auto_off_timer_start(uint32_t time);
static uint8_t light_auto_off_timer_stop(void);

static void light_config_invalid_timer_callback(TimerHandle_t xTimer);

static void light_pwm_percent_calc(uint16_t cct, uint16_t level, uint16_t *pwm);
static void light_pwm_output(uint8_t channel, uint32_t pwm);

static void light_regulate_poll(void);
static void light_regulate_update(uint16_t *destPwm, uint16_t fadeCycle, uint16_t keepCycle, bool turnOff, void(*finishCB)(void));

static void light_brightness_update(uint8_t up_down_mode);
static void light_cct_update(uint8_t up_down_mode);
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
light_status_t g_light_status;
static bool g_config_state = true;
static uint8_t sg_blink_state = 0;
static light_regulate_t *light_regulate = NULL;
static uint16_t light_pwm[LIGHT_MAX_PWM_CHANNEL] = {0};


static TimerHandle_t light_auto_off_timer = NULL;
static TimerHandle_t light_config_invalid_timer = NULL;
static TimerHandle_t light_changecolor_timer = NULL;
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/
void light_regulate_periodic_task(void *p_arg)
{
	portTickType xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();

	for (; ;)
	{
		light_regulate_poll();
		vTaskDelayUntil(&xLastWakeTime, (10 / portTICK_RATE_MS));  //每10ms任务执行一次
	}
}


/**
 * @brief 调光硬件定时中断处理
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/24/2021 17:10:21
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_timer_irq_handler(void)
{
	light_regulate_poll();
	
	timer_clear_irq(TIMER3);
}


// /**
//  * @brief 调光定时器初始化
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return void
//  *
//  * @history
//  * 1.Date          : 12/24/2021 16:50:41
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// static void light_timer_init(void)
// {
// 	timer_init_t timer3_init;
	
// 	//配置TIMER3的中断
// 	__eclic_irq_set_vector(TIMER3_IRQn, (int)light_timer_irq_handler);
// 	eclic_irq_enable(TIMER3_IRQn);
// 	scu_set_device_gate(TIMER3, ENABLE);
// 	timer3_init.mode = timer_count_mode_auto; /*one shot mode*/
// 	timer3_init.div = timer_clk_div_0;        /*1us = 50/2 period*/
// 	timer3_init.width = timer_iqr_width_f;
// 	timer3_init.count = (get_apb_clk() / 1000) * 10; //10ms
// 	timer_init(TIMER3, timer3_init);
// 	timer_start(TIMER3);
// }


/**
 * @brief 灯光修改颜色 
 *
 * @param [in] TimerHandle_t p
 * @param [out] None
 *
 * @return static
 *
 * @history
 * 1.Date          : 12/24/2021 16:58:51
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_change_color_timer_callback(TimerHandle_t p)
{

	light_set_duty(LIGHT_W_PWM_NAME, light_pwm[0]);
	xTimerStop(light_changecolor_timer, 0);
}


/**
 * @brief 灯光状态初始化
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/24/2021 16:52:32
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_status_init(void)
{
    uint16_t real_len = 0;

    if (CINV_OPER_SUCCESS != cinv_item_read(NVDATA_ID_LIGHT_STATUS, sizeof(light_status_t), &g_light_status, &real_len))
    {
		g_light_status.wake_group = 0;
		g_light_status.powerOnState = POWER_ON_STATUS_ON;
		g_light_status.mode = LIGHT_MODE_NATURE;
        g_light_status.onoff = LIGHT_STATUS_ON;
        g_light_status.cct = LIGHT_DEFAULT_CCT;
		g_light_status.level = LIGHT_DEFAULT_LEVEL;
        cinv_item_init(NVDATA_ID_LIGHT_STATUS, sizeof(light_status_t), &g_light_status);
    }

	mprintf("mode = %d, onoff = %d,cct = %d, level = %d\n",g_light_status.mode,g_light_status.onoff,g_light_status.cct,g_light_status.level);


	// //创建改变颜色定时器
	light_changecolor_timer = xTimerCreate("light_changecolor_timer", pdMS_TO_TICKS(LIGHT_CHANGE_COLOR_TIME), pdTRUE, (void *)0, light_change_color_timer_callback);
	if (NULL == light_changecolor_timer)
	{
		ci_logerr(CI_LOG_DEBUG,"light_changecolor_timer create fail!\n");
	}
	

	//创建定时关灯定时
	light_auto_off_timer = xTimerCreate("light_auto_off_timer", pdMS_TO_TICKS(LIGHT_AUTO_OFF_1_MINUTE), pdFALSE, (void *)0, light_auto_off_timer_callback);
    if (NULL == light_auto_off_timer)
    {
        ci_logerr(CI_LOG_ERROR,"light_auto_off_timer create fail!\n");
    }

	//创建配置失效定时器
	light_config_invalid_timer = xTimerCreate("light_config_invalid_timer", pdMS_TO_TICKS(LIGHT_CONFIG_INVALID_TIME), pdFALSE, (void *)0, light_config_invalid_timer_callback);
    if (NULL == light_config_invalid_timer)
    {
        ci_logerr(CI_LOG_ERROR,"light_config_invalid_timer create fail!\n");
    }
	else
	{
		xTimerStart(light_config_invalid_timer, 0);	
	}
}


/**
 * @brief 灯光状态保存
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/24/2021 17:0:7
 *   Author       : zhh
 *   Modification : Created function
 */
void light_status_nv_save(void)
{
	uint16_t real_len = 0;
	light_status_t lamp_status;

	if (CINV_OPER_SUCCESS == cinv_item_read(NVDATA_ID_LIGHT_STATUS, sizeof(light_status_t), &lamp_status, &real_len))
	{
		if (0 != memcmp(&lamp_status, &g_light_status, sizeof(light_status_t)))
		{
			// mprintf("light_status_nv_save\n");
			cinv_item_write(NVDATA_ID_LIGHT_STATUS, sizeof(light_status_t), &g_light_status);
		}
	}

}


/**
 * @brief 定时关灯回调
 *
 * @param [in] TimerHandle_t xTimer
 * @param [out] None
 *
 * @return static
 *
 * @history
 * 1.Date          : 9/28/2021 16:52:23
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_auto_off_timer_callback(TimerHandle_t xTimer)
{
	light_turn_off_output();
}


/**
 * @brief 更新并启动定时关灯定时器
 *
 * @param [in] uint32_t time
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 9/28/2021 16:52:23
 *   Author       : zhh
 *   Modification : Created function
 */
static uint8_t light_auto_off_timer_start(uint32_t time)
{
	uint8_t ret = 1;

	if (NULL != light_auto_off_timer)
	{
		//灯闪一下提示
		light_blink_output();
		mprintf("auto off time = %d\n",time);
		xTimerStop(light_auto_off_timer, 0);
		xTimerChangePeriod(light_auto_off_timer, pdMS_TO_TICKS(time), 0);
		xTimerStart(light_auto_off_timer, 0);
	}
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 取消定时关灯定时器
 *
 * @param [in]  void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 9/28/2021 16:52:23
 *   Author       : zhh
 *   Modification : Created function
 */
static uint8_t light_auto_off_timer_stop(void)
{
	uint8_t ret = 1;

	if (NULL != light_auto_off_timer)
	{
		//灯闪一下提示
		light_blink_output();

		xTimerStop(light_auto_off_timer, 0);
	}
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 配置失效定时回调
 *
 * @param [in] TimerHandle_t xTimer
 * @param [out] None
 *
 * @return static
 *
 * @history
 * 1.Date          : 9/28/2021 16:52:23
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_config_invalid_timer_callback(TimerHandle_t xTimer)
{
	g_config_state = false;
}


/**
 * @brief 灯光PWM占空比计算
 *
 * @param [in] uint16_t cct
 * @param [in] uint16_t level
 * @param [in] uint16_t *pwm
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 14:19:22
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_pwm_percent_calc(uint16_t cct, uint16_t level, uint16_t *pwm)
{
	if ((level < DEFAULT_MIN_LEVEL) && (level != 0))
	{
		level = DEFAULT_MIN_LEVEL;
	}

	pwm[LIGHT_W_PWM_CHANNEL] = level;
}


/**
 * @brief 灯光PWM输出
 *
 * @param [in] uint8_t channel
 * @param [in] uint32_t pwm
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 14:21:46
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_pwm_output(uint8_t channel, uint32_t pwm)
{
	if (pwm >= LIGHT_PWM_MAX_DUTY)
	{
		pwm = LIGHT_PWM_MAX_DUTY - 1;
	}

	if (channel == LIGHT_W_PWM_CHANNEL)
	{
		light_set_duty(LIGHT_W_PWM_NAME, pwm);
	}
	else
	{
		mprintf("light pwm channel input error!\r\n");
	}

	light_pwm[channel] = pwm;
}


/**
 * @brief 灯光调节扫描
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 14:29:31
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_regulate_poll(void)
{
	uint8_t i = 0;
	static uint8_t count = 0;
	void (*finishCB)(void) = NULL;

	if (light_regulate)
	{
		if ( true == light_regulate->regulate )
		{
			if (light_regulate->fadeCycle)
			{
				light_regulate->fadeCycle--;
				if (light_regulate->regulate == true)
				{
					if (light_regulate->fadeCycle == 0)
					{
						for (i = 0; i < LIGHT_MAX_PWM_CHANNEL; i++)
						{
							light_regulate->channel[i].current = light_regulate->channel[i].target;
						}
					}
					else
					{
						for (i = 0; i < LIGHT_MAX_PWM_CHANNEL; i++)
						{
							light_regulate->channel[i].current += light_regulate->channel[i].duty;
						}
					}
					for (i = 0; i < LIGHT_MAX_PWM_CHANNEL; i++)
					{
						light_pwm_output(i, (light_regulate->channel[i].current >> LightCTR_REGU_DUTY_BIT));
					}
				}
			}
			else if (light_regulate->turnOff)
			{
				light_regulate->turnOff = false;
				for (i = 0; i < LIGHT_MAX_PWM_CHANNEL; i++)
				{
					light_pwm_output(i, 0);
				}
			}
			else if (light_regulate->keepCycle)
			{
				light_regulate->keepCycle--;
			}
			else if (light_regulate->finishCB)
			{
				finishCB = light_regulate->finishCB;
			}
			else
			{
				vPortFree(light_regulate);
				light_regulate = NULL;
			}
		}
	}

	if (finishCB)
	{
		finishCB();
	}
}


/**
 * @brief 灯光调节更新
 *
 * @param [in] uint16_t *destPwm
 * @param [in] uint16_t fadeCycle
 * @param [in] uint16_t keepCycle
 * @param [in] bool turnOff
 * @param [in] void(*finishCB)(void)
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 14:29:55
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_regulate_update(uint16_t *destPwm, uint16_t fadeCycle, uint16_t keepCycle, bool turnOff, void(*finishCB)(void))
{
	if ((NULL == light_regulate) && (NULL != destPwm))
	{
		light_regulate = (light_regulate_t *)pvPortMalloc(sizeof(light_regulate_t));
	}

	if (NULL != light_regulate)
	{
		light_regulate->regulate = false;
		light_regulate->fadeCycle = fadeCycle;
		light_regulate->keepCycle = keepCycle;
		light_regulate->turnOff = turnOff;
		light_regulate->finishCB = finishCB;

		if (destPwm)
		{
			for (uint8_t ch = 0; ch < LIGHT_MAX_PWM_CHANNEL; ch++)
			{
				if (fadeCycle == 0)
				{
					light_pwm_output(ch, destPwm[ch]);
				}
				else
				{
					uint32_t target = (uint32_t)((destPwm[ch] & LightCTR_REGU_DUTY_BIT_MSK) << LightCTR_REGU_DUTY_BIT);
					uint32_t current = (uint32_t)((light_pwm[ch] & LightCTR_REGU_DUTY_BIT_MSK) << LightCTR_REGU_DUTY_BIT);
					int32_t delta = target - current;

					delta /= (int32_t) fadeCycle;

					light_regulate->channel[ch].current = current;
					light_regulate->channel[ch].target = target;
					light_regulate->channel[ch].duty = delta;
				}
			}

			light_regulate->regulate = true;
		}
	}
}


/**
 * @brief 灯光输出更新
 *
 * @param [in] uint16_t toCct
 * @param [in] uint16_t toLevel
 * @param [in] uint16_t fadeTime
 * @param [in] uint16_t keepTime
 * @param [in] void(*finishCB)(void)
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 14:33:52
 *   Author       : zhh
 *   Modification : Created function
 */
void light_output_update(uint16_t toCct, uint16_t toLevel, uint16_t fadeTime, uint16_t keepTime, void(*finishCB)(void))
{
	uint16_t destPwm[LIGHT_MAX_PWM_CHANNEL] = {0};
	uint16_t *pDestPwm = destPwm;
	bool turnOff = false;

	uint16_t fadeCycle = fadeTime * (BASE_FADE_TIME / LIGHT_CTR_TIMER_PERIOD);
	uint16_t keppCycle = keepTime * (BASE_FADE_TIME / LIGHT_CTR_TIMER_PERIOD);

	if (toLevel > DEFAULT_MAX_LEVEL)
	{
		toLevel = DEFAULT_MAX_LEVEL;
	}

	if ((toLevel <= DEFAULT_MIN_LEVEL) && (toLevel != 0))
	{
		toLevel = DEFAULT_MIN_LEVEL;
		fadeCycle = 0;	
	}

	if (toCct > DEFAULT_COOL_CCT)
	{
		toCct = DEFAULT_COOL_CCT;
	}
	else if (toCct < DEFAULT_WARM_CCT)
	{
		toCct = DEFAULT_WARM_CCT;
	}

	light_pwm_percent_calc(toCct, toLevel, destPwm);

	if (toLevel == 0)
	{
		turnOff = true;
	}
	g_light_status.cct = toCct;

#if LIGHT_OFF_SAVE_EN
	g_light_status.level = toLevel;
#else
	if(toLevel != 0)
	{
		g_light_status.level = toLevel;
	}
#endif

	light_regulate_update(pDestPwm, fadeCycle, keppCycle, turnOff, finishCB);
}


/**
 * @brief 获取灯具开关
 *
 * @param [in] void
 * @param [out] None
 *
 * @return bool
 *
 * @history
 * 1.Date          : 12/24/2021 17:19:22
 *   Author       : zhh
 *   Modification : Created function
 */
bool get_light_onoff(void)
{
    return  g_light_status.onoff;
}


/**
 * @brief 获取灯具色温
 *
 * @param [in] void
 * @param [out] None
 *
 * @return
 *
 * @history
 * 1.Date          : 12/24/2021 17:19:55
 *   Author       : zhh
 *   Modification : Created function
 */
uint16_t get_light_cct(void)
{
    return g_light_status.cct;
}


/**
 * @brief 获取灯具亮度
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint16_t
 *
 * @history
 * 1.Date          : 12/24/2021 17:20:16
 *   Author       : zhh
 *   Modification : Created function
 */
uint16_t get_light_level(void)
{
    return g_light_status.level;
}


/**
 * @brief 获取灯具唤醒词分组
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/24/2021 17:20:16
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t get_light_wake_group(void)
{
    return g_light_status.wake_group;
}


/**
 * @brief 灯光上电输出
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 14:34:49
 *   Author       : zhh
 *   Modification : Created function
 */
void light_power_on_output(void)
{
	if (RETURN_OK != Scu_GetSysResetState())
	{
		/* 本次开机属于异常复位，灯光以之前的状态启动  */
		if (g_light_status.onoff == LIGHT_STATUS_ON)
		{
			light_output_update(get_light_cct(), get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
		}
		else
		{
			light_output_update(get_light_cct(), 0, LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
		}
	}
	else
	{
		switch (g_light_status.powerOnState)
		{
			case POWER_ON_STATUS_OFF :
				g_light_status.onoff = LIGHT_STATUS_OFF;
				light_output_update(get_light_cct(), 0, LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
				break;

			case POWER_ON_STATUS_ON :
				g_light_status.onoff = LIGHT_STATUS_ON;
				light_output_update(get_light_cct(), get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
				break;

			case POWER_ON_STATUS_MEMORY :
				light_output_update(get_light_cct(), get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
				break;

			default:
				break;
		}
	}
}


/**
 * @brief 开灯
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:27:13
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_turn_on_output(void)
{
	uint8_t ret = 1;
	uint16_t fadeTim = LIGHT_DEFAULT_ON_FADE_TIME;
    if (get_light_onoff() == LIGHT_STATUS_OFF)
    {
        g_light_status.onoff = LIGHT_STATUS_ON;
		if (get_light_level())
        {
			if (get_light_level() <= LIGHT_2_GEAR_LEVEL)
			{
				fadeTim = 0;
			}
        	light_output_update(get_light_cct(), get_light_level(), fadeTim, LIGHT_DEFAULT_KEEP_TIME, NULL);
        }
        else
        {
        	light_output_update(get_light_cct(), LIGHT_DEFAULT_LEVEL, LIGHT_DEFAULT_ON_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
        }
    }
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 关灯
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:26:6
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_turn_off_output(void)
{
	uint8_t ret = 1;
 	uint16_t fadeTim = LIGHT_DEFAULT_FADE_TIME;
    if (get_light_onoff() == LIGHT_STATUS_ON)
    {
        g_light_status.onoff = LIGHT_STATUS_OFF;
		if (get_light_level() <= LIGHT_2_GEAR_LEVEL)
		{
			fadeTim = 0;
		}
        light_output_update(get_light_cct(), 0, fadeTim, LIGHT_DEFAULT_KEEP_TIME, NULL);
    }
	else
	{
		ret = 0;
	}

	//关灯后，暂停自动关灯定时器
	if (NULL != light_auto_off_timer) 
	{
		xTimerStop(light_auto_off_timer, 0);
	}

	return ret;
}


/**
 * @brief 最高亮度
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:33:48
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_brightness_max_output(void)
{
	uint8_t ret = 1;

    if (get_light_onoff() == LIGHT_STATUS_ON)
    {
        light_output_update(get_light_cct(), DEFAULT_MAX_LEVEL, LIGHT_DEFAULT_OFF_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
    }
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 中等亮度
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:38:56
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_brightness_mid_output(void)
{
	uint8_t ret = 1;

    if (get_light_onoff() == LIGHT_STATUS_ON)
    {
        light_output_update(get_light_cct(), DEFAULT_MID_LEVEL, LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
    }
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 最低亮度
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:39:02
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_brightness_min_output(void)
{
	uint8_t ret = 1;

    if (get_light_onoff() == LIGHT_STATUS_ON)
    {
        light_output_update(get_light_cct(), DEFAULT_MIN_LEVEL, LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
    }
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 亮度调节
 *
 * @param [in] uint8_t up_down_mode
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 15:47:28
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_brightness_update(uint8_t up_down_mode)
{
	uint16_t cur_level = 0;
	uint16_t dest_level = 0;

    cur_level =  get_light_level();
	if (up_down_mode == LIGHT_BRIGHTNESS_UP)
	{
		switch (cur_level)
		{
		case LIGHT_1_GEAR_LEVEL:
			dest_level = LIGHT_2_GEAR_LEVEL;
			break;

		case LIGHT_2_GEAR_LEVEL:
			dest_level = LIGHT_3_GEAR_LEVEL;
			break;

		case LIGHT_3_GEAR_LEVEL:
			dest_level = LIGHT_4_GEAR_LEVEL;
			break;

		case LIGHT_4_GEAR_LEVEL:
			dest_level = LIGHT_4_GEAR_LEVEL;
			break;
		
		default:
			break;
		}
	}
	else
	{
		switch (cur_level)
		{
		case LIGHT_1_GEAR_LEVEL:
			dest_level = LIGHT_1_GEAR_LEVEL;
			break;

		case LIGHT_2_GEAR_LEVEL:
			dest_level = LIGHT_1_GEAR_LEVEL;
			break;

		case LIGHT_3_GEAR_LEVEL:
			dest_level = LIGHT_2_GEAR_LEVEL;
			break;

		case LIGHT_4_GEAR_LEVEL:
			dest_level = LIGHT_3_GEAR_LEVEL;
			break;
		
		default:
			break;
		}
	}

	light_output_update(get_light_cct(), dest_level, LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
}


/**
 * @brief 增加亮度
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:52:16
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_brightness_up_output(void)
{
	uint8_t ret = 1;

    if (get_light_onoff() == LIGHT_STATUS_ON)
    {
        if ((get_light_level() >= DEFAULT_MAX_LEVEL))
        {
            //已最亮
			ret = 0;
        }
        else
        {
            light_brightness_update(LIGHT_BRIGHTNESS_UP);
        }
    }
    else
    {	
		//灯未开
		ret = 0;
    }
	return ret;
}


/**
 * @brief 减小亮度
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:53:2
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_brightness_down_output(void)
{
	uint8_t ret = 1;

    if (get_light_onoff() == LIGHT_STATUS_ON)
    {
        if ((get_light_level() <= DEFAULT_MIN_LEVEL))
        {
            //已最暗
			ret = 0;
        }
        else
        {
            light_brightness_update(LIGHT_BRIGHTNESS_DOWN);
        }
    }
    else
    {	
		//灯未开
		ret = 0;
    }
	return ret;
}


// /**
//  * @brief 减小亮度
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 12/27/2021 15:53:2
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_brightness_percentage_output(uint32_t dest_level)
// {
// 	uint8_t ret = 1;

//     if (get_light_onoff() == LIGHT_STATUS_ON)
//     {
// 		light_output_update(get_light_cct(), dest_level, LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
//     }
//     else
//     {	
// 		//灯未开
// 		ret = 0;
//     }

// 	return ret;
// }


/**
 * @brief 色温调节
 *
 * @param [in] uint8_t up_down_mode
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/27/2021 15:47:28
 *   Author       : zhh
 *   Modification : Created function
 */
static void light_cct_update(uint8_t up_down_mode)
{
	int16_t cur_cct = 0;
    int16_t dest_cct = 0;

    cur_cct =  get_light_cct();

    if (up_down_mode == LIGHT_CCT_UP)
    {
        dest_cct = cur_cct + 1000;
        if (dest_cct >= DEFAULT_COOL_CCT)
        {
            dest_cct = DEFAULT_COOL_CCT;
        }
    }
    else
    {
        dest_cct = cur_cct - 1000;
        if (dest_cct <= DEFAULT_WARM_CCT)
        {
            dest_cct = DEFAULT_WARM_CCT;
        }
    }

    light_output_update(dest_cct, get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
}


// /**
//  * @brief 白一点
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 4/21/2021 10:58:25
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_cct_up_output(void)
// {
//     uint8_t ret = 1;

//     if (get_light_onoff() == LIGHT_STATUS_ON)
//     {
//         if (get_light_cct() == DEFAULT_COOL_CCT)
//         {
// 			//已最白
//             ret = 0;
//         }
//         else
//         {
//             light_cct_update(LIGHT_CCT_UP);
//         }
//     }
//     else
//     {
// 		//灯未开
//         ret = 0;
//     }
//     return ret;
// }


// /**
//  * @brief 黄一点
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 4/21/2021 10:58:36
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_cct_down_output(void)
// {
//     uint8_t ret = 1;

//     if (get_light_onoff() == LIGHT_STATUS_ON)
//     {
//         if (get_light_level() == DEFAULT_WARM_CCT)
//         {
// 			//已最黄
//             ret = 0;
//             return ret;
//         }
//         else
//         {
//             light_cct_update(LIGHT_CCT_DOWN);
//         }
//     }
//     else
//     {
// 		//灯未开
// 		ret = 0;
//     }
//     return ret;
// }


// /**
//  * @brief 白光/冷光输出
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 1/7/2022 14:7:5
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_white_output(void)
// {
// 	uint8_t ret = 1;

// 	if (get_light_onoff() == LIGHT_STATUS_ON)
// 	{
// 		g_light_status.mode = LIGHT_MODE_WHITE;
// 		light_output_update(DEFAULT_COOL_CCT, get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
// 	}
// 	else
//     {	
// 		//灯未开
// 		ret = 0;
//     }
// 	return ret;
// }


// /**
//  * @brief 黄光/暖光输出
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 1/7/2022 14:7:5
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_yellow_output(void)
// {
// 	uint8_t ret = 1;

// 	if (get_light_onoff() == LIGHT_STATUS_ON)
// 	{
// 		g_light_status.mode = LIGHT_MODE_YELLOW;
// 		light_output_update(DEFAULT_WARM_CCT, get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
// 	}
// 	else
//     {	
// 		//灯未开
// 		ret = 0;
//     }
// 	return ret;
// }


// /**
//  * @brief 自然光/中性光输出
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 1/7/2022 14:7:5
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_nature_output(void)
// {
// 	uint8_t ret = 1;

// 	if (get_light_onoff() == LIGHT_STATUS_ON)
// 	{
// 		g_light_status.mode = LIGHT_MODE_NATURE;
// 		light_output_update(DEFAULT_MID_CCT, get_light_level(), LIGHT_DEFAULT_FADE_TIME, LIGHT_DEFAULT_KEEP_TIME, NULL);
// 	}
// 	else
//     {	
// 		//灯未开
// 		ret = 0;
//     }
// 	return ret;
// }


/**
 * @brief 改变灯光
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date          : 12/27/2021 15:26:6
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_change_output(void)
{
	uint8_t ret = 1;
	
	if (g_light_status.onoff == LIGHT_STATUS_ON)
	{
		light_set_duty(LIGHT_W_PWM_NAME, 0);
		xTimerStart(light_changecolor_timer, 0);	
	}
	else
	{
		ret = 0;
	}


	return ret;
}


// /**
//  * @brief 墙壁开关模式切换
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 12/27/2021 15:26:6
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_mode_output(void)
// {
// 	uint8_t ret = 1;
	
// 	if (g_light_status.onoff == LIGHT_STATUS_ON)
// 	{
// 		if (g_light_status.mode == LIGHT_MODE_NATURE)
// 		{
// 			g_light_status.mode = LIGHT_MODE_WHITE;
// 			g_light_status.onoff = LIGHT_STATUS_ON;
// 			light_output_update(DEFAULT_COOL_CCT, LIGHT_DEFAULT_LEVEL, 0, 0, NULL);
// 		}
// 		else if (g_light_status.mode == LIGHT_MODE_WHITE)
// 		{
// 			g_light_status.mode = LIGHT_MODE_YELLOW;
// 			g_light_status.onoff = LIGHT_STATUS_ON;
// 			light_output_update(DEFAULT_WARM_CCT, LIGHT_DEFAULT_LEVEL, 0, 0, NULL);
// 		}
// 		else if (g_light_status.mode == LIGHT_MODE_YELLOW)
// 		{
// 			g_light_status.mode = LIGHT_MODE_NATURE;
// 			g_light_status.onoff = LIGHT_STATUS_ON;
// 			light_output_update(DEFAULT_MID_CCT, LIGHT_DEFAULT_LEVEL, 0, 0, NULL);
// 		}
// 	}
// 	else
// 	{
// 		g_light_status.onoff = LIGHT_STATUS_ON;	
// 		light_output_update(g_light_status.cct, g_light_status.level, 0, LIGHT_DEFAULT_KEEP_TIME, NULL);
// 	}


// 	return ret;
// }


// /**
//  * @brief 复位灯光呼吸回调
//  *
//  * @param [in] TimerHandle_t p
//  * @param [out] None
//  *
//  * @return static
//  *
//  * @history
//  * 1.Date          : 12/24/2021 16:58:51
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// static void light_breathe_callback(void)
// {
// 	static uint8_t g_breathe_state = 0;

// 	if (g_breathe_state == 0)
// 	{
// 		g_breathe_state = 1;
// 		light_output_update(LIGHT_DEFAULT_CCT, DEFAULT_MIN_LEVEL, 5, 0, light_breathe_callback);	
// 	}
// 	else if(g_breathe_state == 1)
// 	{
// 		g_breathe_state = 2;
// 		light_output_update(LIGHT_DEFAULT_CCT, LIGHT_DEFAULT_LEVEL, 5, 0, light_breathe_callback);		
// 	}
// 	else if(g_breathe_state == 2)
// 	{
// 		g_breathe_state = 3;
// 		light_output_update(LIGHT_DEFAULT_CCT, DEFAULT_MIN_LEVEL, 5, 0, light_breathe_callback);		
// 	}
// 	else if(g_breathe_state == 3)
// 	{
// 		g_breathe_state = 4;
// 		light_output_update(LIGHT_DEFAULT_CCT, LIGHT_DEFAULT_LEVEL, 5, 0, light_breathe_callback);		
// 	}
// 	else if(g_breathe_state == 4)
// 	{
// 		g_breathe_state = 5;
// 		light_output_update(LIGHT_DEFAULT_CCT, DEFAULT_MIN_LEVEL, 5, 0, light_breathe_callback);		
// 	}
// 	else if(g_breathe_state == 5)
// 	{
// 		g_breathe_state = 0;
// 		g_ac_power_times = 0;
// 		light_output_update(LIGHT_DEFAULT_CCT, LIGHT_DEFAULT_LEVEL, 5, 0, NULL);		
// 	}
// }


// /**
//  * @brief 灯光复位恢复出厂状态
//  *
//  * @param [in] void
//  * @param [out] None
//  *
//  * @return uint8_t
//  *
//  * @history
//  * 1.Date          : 12/27/2021 15:26:6
//  *   Author       : zhh
//  *   Modification : Created function
//  */
// uint8_t light_reset_factory(void)
// {
// 	g_light_status.powerOnState = POWER_ON_STATUS_ON;
// 	g_light_status.mode = LIGHT_MODE_NATURE;
// 	g_light_status.onoff = LIGHT_STATUS_ON;
// 	g_light_status.cct = LIGHT_DEFAULT_CCT;
// 	g_light_status.level = LIGHT_DEFAULT_LEVEL;
	
// 	light_output_update(LIGHT_DEFAULT_CCT, LIGHT_DEFAULT_LEVEL, 0, 0, light_breathe_callback);
// }


/**
 * @brief 灯光控制初始化
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date          : 12/24/2021 16:48:32
 *   Author       : zhh
 *   Modification : Created function
 */
void light_control_init(void)
{
	// 灯驱动初始化
	light_driver_init();

	#if 0
	// 调光定时器初始化
	light_timer_init();
	#else
	// 创建调光周期性任务(10ms)
	xTaskCreate(light_regulate_periodic_task,"light_regulate_periodic_task",200,NULL,4,NULL);
	#endif

	// 灯状态初始化
	light_status_init();

	// 灯光上电输出
	light_power_on_output();

	// AC检测初始化
	//light_ac_detect_init();
}


static void light_blink_callback(void)
{
	static uint16_t light_pre_level = 0;

	if (sg_blink_state == 0)
	{
		sg_blink_state = 1;
		light_output_update(get_light_cct(), 0, 5, 0, light_blink_callback);	
	}
	else if(sg_blink_state == 1)
	{
		sg_blink_state = 0;
		light_output_update(get_light_cct(), get_light_level(), 5, 0, NULL);		
	}
	else if(sg_blink_state == 2)
	{
		sg_blink_state = 3;
		light_pre_level = get_light_level();
		light_output_update(get_light_cct(), DEFAULT_MIN_LEVEL, 0, 2, light_blink_callback);		
	}
	else if(sg_blink_state == 3)
	{
		sg_blink_state = 0;
		light_output_update(get_light_cct(), 0, 0, 0, NULL);	
		g_light_status.level = 	light_pre_level;
	}
}


void light_blink_output(void)
{
	if (get_light_onoff() == LIGHT_STATUS_ON)
	{
		sg_blink_state = 0;	
	}
	else
	{
		sg_blink_state = 2;		
	}
	light_output_update(get_light_cct(), get_light_level(), 0, 0, light_blink_callback);
}


/**
 * @brief 灯光控制处理
 *
 * @param [in] uint32_t semantic_id
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date         : 12/24/2021 16:48:32
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_control_deal(uint32_t semantic_id)
{
	uint8_t ret = 1;

	switch (semantic_id)
    {
        case 1008: //大开灯
            ret = light_turn_on_output();
            break;

        case 1009: //关闭灯
            ret = light_turn_off_output();
            break;

        case 1010: //调亮一点
		
            ret = light_brightness_up_output();
            break;

        case 1011: //调暗一点
            ret = light_brightness_down_output();
            break;

        case 1012: //最高亮度
            ret = light_brightness_max_output();
            break;

        case 1013: //中等亮度
            ret = light_brightness_mid_output();
            break;

        case 1014: //最低亮度
		case 1015: //小夜灯
            ret = light_brightness_min_output();
            break;

		case 1016: //改变灯光
		case 1017: //改变颜色
			ret = light_change_output();
			break;

        default:
            ret = 0;
            break;
    }
	sys_msg_t send_msg;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    send_msg.msg_type = SYS_MSG_TYPE_NV_SAVE;
    send_msg_to_sys_task(&send_msg, &xHigherPriorityTaskWoken);  //软件定时中断回调中，发送消息到任务去进行nv保存操作，不直接进行flash读写操作，防止死机

	return ret;
}



/**
 * @brief 灯光定时处理
 *
 * @param [in] uint32_t semantic_id
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date         : 12/24/2021 16:48:32
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_timing_deal(uint32_t semantic_id)
{
	uint8_t ret = 1;
	
	if (g_light_status.onoff == LIGHT_STATUS_ON)
	{
		switch (semantic_id)
		{
			case 1018: //一分钟后关灯
				ret = light_auto_off_timer_start(LIGHT_AUTO_OFF_1_MINUTE);
				break;

			case 1019: //五分钟后关灯
				ret = light_auto_off_timer_start(LIGHT_AUTO_OFF_5_MINUTE);
				break;

			case 1020: //十分钟后关灯
				ret = light_auto_off_timer_start(LIGHT_AUTO_OFF_10_MINUTE);
				break;

			case 1021: //取消定时
				ret = light_auto_off_timer_stop();
				break;

			default:
				ret = 0;
				break;
		}
	}
	else
	{
		ret = 0;
	}

	return ret;
}


/**
 * @brief 上电状态处理
 *
 * @param [in] uint32_t semantic_id
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date         : 12/24/2021 16:48:32
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_poweron_deal(uint32_t semantic_id)
{
	uint8_t ret = 1;

	if (g_config_state)
	{
		switch (semantic_id)
		{
			case 1023://来电开
				mprintf("config poweron on\n");
				g_light_status.powerOnState = POWER_ON_STATUS_ON;
				light_status_nv_save();
				light_blink_output();
				break;

			case 1024://来电关
				mprintf("config poweron off\n");
				g_light_status.powerOnState = POWER_ON_STATUS_OFF;
				light_status_nv_save();
				light_blink_output();
				break;

			case 1025://来电记忆
				mprintf("config poweron memory\n");
				g_light_status.powerOnState = POWER_ON_STATUS_MEMORY;
				light_status_nv_save();
				light_blink_output();
				break;

			default:
				ret = 0;
				break;
		}
	}
	else
	{
		mprintf("config invalid due to timeout\n");
		ret = 0;
	}

	return ret;
}



/**
 * @brief 灯具名称配置处理
 *
 * @param [in] uint32_t semantic_id
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date         : 12/24/2021 16:48:32
 *   Author       : zhh
 *   Modification : Created function
 */
uint8_t light_name_config_deal(uint32_t semantic_id)
{
	uint8_t ret = 1;

	if (g_config_state)
	{
		switch (semantic_id)
		{
			case 1026://设为客厅灯
				g_light_status.wake_group = 1;
				light_status_nv_save();
				light_blink_output();
				break;

			case 1027://设为卧室灯
				g_light_status.wake_group = 2;
				light_status_nv_save();
				light_blink_output();
				break;

			case 1028://设为厨房灯
				g_light_status.wake_group = 3;
				light_status_nv_save();
				light_blink_output();
				break;

			case 1029://设为主卧灯
				g_light_status.wake_group = 4;
				light_status_nv_save();
				light_blink_output();
				break;	

			case 1030://设为餐厅灯
				g_light_status.wake_group = 5;
				light_status_nv_save();
				light_blink_output();
				break;	

			case 1031://设为镜前灯
				g_light_status.wake_group = 6;
				light_status_nv_save();
				light_blink_output();
				break;	

			case 1032://设为默认灯
				g_light_status.wake_group = 7;
				light_status_nv_save();
				light_blink_output();
				break;	

			default:
				ret = 0;
				break;
		}
	}
	else
	{
		mprintf("config invalid due to timeout\n");
		ret = 0;
	}

	return ret;
}