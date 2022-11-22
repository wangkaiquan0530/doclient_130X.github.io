/**
 * @brief 蜂鸣器驱动源文件
 * @file buzzer_driver.c
 * @version 1.0
 * @author zhh
 * @date 5/22/2021 22:49:33
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 5/22/2021 22:49:33
 *   Author      : zhh
 *   Modification: Created file
 */
/*----------------------------------------------*
 * includes 					                *
 *----------------------------------------------*/
#include "string.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include "buzzer_driver.h"
#include "ci_log.h"

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
static stBeepInfo beepinfo = {0};

//tone define
typedef unsigned short stToneType;

//	 index					0	 1	  2    3	4	 5	  6    7	8	 9	  10   11	12	 13
//						        低7   1	  2	   3	4    5	  6	   7	  高1	 高2	高3   高4   高5	 不发音
static stToneType tone[] = {2470,2000,2940,3300,3490,3920,4400,2940,5230,5870,6590,6980,7840,10000};//音频数据表

//check Freq
static int BeepVoiceCheckFreq(int freq)
{
	int i = 0;
	int toneNum = sizeof(tone)/sizeof(stToneType);

	for (i = 0; i < toneNum; i++)
	{
		if (freq == tone[i])
		{
			return RETURN_OK;
		}
	}

	return RETURN_ERR;
}

//func
static int BeepVoicePwmStop(void)
{
	if (1 != beepinfo.bInit)
	{
		goto out;
	}

	pwm_stop(beepinfo.cfg.PwmBase);
	dpmu_set_io_reuse(beepinfo.cfg.PinName,beepinfo.cfg.IoFun);/*gpio function*/
	gpio_set_output_level_single(beepinfo.cfg.GpioBase, beepinfo.cfg.PinNum, 0);

	return RETURN_OK;

out:
	return RETURN_ERR;
}

static int BeepVoiceCfgFreq(int freq)
{
	pwm_init_t BeePwmInit;

	if ((1 != beepinfo.bInit) ||(RETURN_OK != BeepVoiceCheckFreq(freq)))
	{
		goto out;
	}

	if (tone[13] == freq) //10000 为不发音，所以需要关掉PWM
	{
		BeepVoicePwmStop();
		return RETURN_OK;
	}

	scu_set_device_gate(beepinfo.cfg.PwmBase,ENABLE);
	dpmu_set_io_reuse(beepinfo.cfg.PinName,beepinfo.cfg.PwmFun);
	BeePwmInit.clk_sel=0;
	BeePwmInit.freq = freq;
	BeePwmInit.duty = 500;//700;
	BeePwmInit.duty_max = 1000;
	pwm_init(beepinfo.cfg.PwmBase,BeePwmInit);
	pwm_set_duty(beepinfo.cfg.PwmBase,BeePwmInit.duty,BeePwmInit.duty_max);
	pwm_start(beepinfo.cfg.PwmBase);

	return RETURN_OK;

out:
	return RETURN_ERR;
}

static void BeepVoiceCallBack(TimerHandle_t xTimer)
{
	if ((1 != beepinfo.bInit) || (NULL == beepinfo.pBuff))
	{
		goto out;
	}

	beepinfo.BeepIndex++;

	if (beepinfo.BeepIndex > beepinfo.BeepCnt)
	{
		xTimerStop(beepinfo.beepTimer, 100);
		xTimerDelete(beepinfo.beepTimer, 100);
		beepinfo.beepTimer = NULL;
		beepinfo.BeepCnt = 0;
		beepinfo.BeepIndex = 0;
		beepinfo.pBuff = NULL;
		beepinfo.TimerID = 0;

		BeepVoicePwmStop();
		goto out;
	}

	if (0 == beepinfo.pBuff->ms)
	{
		goto out;
	}

	BeepVoiceCfgFreq(tone[beepinfo.pBuff->tone]);

	xTimerChangePeriod(beepinfo.beepTimer, pdMS_TO_TICKS(beepinfo.pBuff->ms), 0);

	beepinfo.pBuff++;

out:
	return;
}

//
int BeepVoiceStart(char* buff, int len)
{
	if ((NULL == buff) || (0 != (len%(sizeof(stBuff)))))
	{
		goto out;
	}

	if (NULL != beepinfo.beepTimer)
	{
		xTimerStop(beepinfo.beepTimer, 100);
		xTimerDelete(beepinfo.beepTimer, 100);
		beepinfo.beepTimer = NULL;
		beepinfo.BeepCnt = 0;
		beepinfo.BeepIndex = 0;
		beepinfo.pBuff = NULL;
		beepinfo.TimerID = 0;

		BeepVoicePwmStop();
	}

	beepinfo.pBuff = (stBuff*)buff;
	beepinfo.beepTimer = xTimerCreate("beepTimer", pdMS_TO_TICKS(100),pdTRUE, &beepinfo.TimerID, BeepVoiceCallBack);
	if(NULL == beepinfo.beepTimer)
	{
		mprintf("beepTimer error\n");
	}

	beepinfo.BeepCnt = len/(sizeof(stBuff));
	beepinfo.BeepIndex = 0;
	xTimerStart(beepinfo.beepTimer, 100);

	return RETURN_OK;

out:
	return RETURN_ERR;
}

int BeepVoiceInit(stBeepCfg* cfg)
{
	if ((NULL == cfg) || (1 == beepinfo.bInit))
	{
		goto out;
	}

	memcpy(&(beepinfo.cfg), cfg, sizeof(stBeepCfg));

	beepinfo.bInit = 1;

	return RETURN_OK;

out:
	return RETURN_ERR;
}

//第一个数对应数组tone[]第几个 后面数字是时间
unsigned short PowerOnbeepBuff[] =
{
	1,1000,
};

unsigned short wakeupbeepBuff[] =
{
	1,1000,
};

unsigned short exitwakeupbeepBuff[] =
{
	1,200,
};

unsigned short cmdbeepBuff[] =
{
	1,200,
};

unsigned short timingbeepBuff[] =
{
	1,200, 13,100, 1,200, 13,100, 1,500,
};


void power_on_beep(void)
{
	mprintf("power_on_beep\n");
	BeepVoiceStart((char*)PowerOnbeepBuff, sizeof(PowerOnbeepBuff));//后续调用这个接口
}

void wake_up_beep(void)
{
	mprintf("wake_up_beep\n");
	BeepVoiceStart((char*)wakeupbeepBuff, sizeof(wakeupbeepBuff));//后续调用这个接口
}

void exit_wake_up_beep(void)
{
	mprintf("exit_wake_up_beep\n");
	BeepVoiceStart((char*)exitwakeupbeepBuff, sizeof(exitwakeupbeepBuff));//后续调用这个接口
}

void cmd_beep(void)
{
	mprintf("cmd_beep\n");
	BeepVoiceStart((char*)cmdbeepBuff, sizeof(cmdbeepBuff));//后续调用这个接口
}

void timing_beep(void)
{
	mprintf("timing_beep\n");
	BeepVoiceStart((char*)timingbeepBuff, sizeof(timingbeepBuff));//后续调用这个接口
}

void  buzzer_driver_init(void)
{
	stBeepCfg beepconfig;

    beepconfig.PinName    = PA4;
    beepconfig.PinNum     = pin_4;
    beepconfig.PwmBase    = PWM2;

    beepconfig.GpioBase   = PA;
    beepconfig.PwmFun     = FIFTH_FUNCTION;
    beepconfig.IoFun      = FIRST_FUNCTION;

    BeepVoiceInit(&beepconfig);

	power_on_beep();
}
