/**
 * @brief 继电器控制源文件
 * @file electric_control.c
 * @version 1.0
 * @author wkq
 * @date 7/14/2022 21:24:45
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 7/14/2022 10:57:56
 *   Author      : wkq
 *   Modification: Created file
 */
/*----------------------------------------------*
 * includes							            *
 *----------------------------------------------*/
#include "electric_control.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "prompt_player.h"
#include "system_msg_deal.h"
#include "ci_nvdata_manage.h"
#include "ci_log.h"
#include "ci130x_timer.h"

uint8_t g_electric_relay_onoff = 0 ;//0关闭 1打开 状态
electric_relay_status_t g_electric_relay_status;
static gpio_irq_callback_list_t zero_point_detect_irq_callback_node = {zero_point_detect_irq_callback, NULL};


/**
 * @brief 继电器状态信息NV初始化
 *
 * @param [in] void
 * @param [out] None
 *
 * @return void
 *
 * @history
 * 1.Date		   : 9/11/2022 10:43:12
 *	 Author 	  : wkq
 *	 Modification : Created function
 */
void electric_relay_status_nv_init(void)
{
	uint16_t real_len = 0;
	cinv_item_ret_t ret = CINV_ITEM_UNINIT;

	/* 从nvdata里读取继电器状态信息 */
	if (CINV_OPER_SUCCESS != cinv_item_read(NVDATA_ID_ELECTRIC_STATUS, sizeof(electric_relay_status_t), &g_electric_relay_status, &real_len))
	{
		/* nvdata内无继电器息则配置为初始值并写入nv */
		g_electric_relay_status.power_statue = ELECTRIC_RELAY_POWER_ON_ON;
        g_electric_relay_onoff=1;
		g_electric_relay_status.onOff = g_electric_relay_onoff;

		ret = cinv_item_init(NVDATA_ID_ELECTRIC_STATUS, sizeof(electric_relay_status_t), &g_electric_relay_status);
		if ((ret != CINV_OPER_SUCCESS) && (ret != CINV_ITEM_UNINIT))
		{
			ci_logerr(CI_LOG_ERROR, "NVDATA_ID_ELECTRIC_STATUS init fail!\r\n");
		}
	}
}

/**
 * @brief 继电器状态信息NV保存
 *
 * @param [in] void
 * @param [out] None
 *
 * @return uint8_t
 *
 * @history
 * 1.Date		   : 9/11/2022 10:43:12
 *	 Author 	  : wkq
 *	 Modification : Created function
 */
uint8_t electric_relay_status_nv_save(void)
{
	uint16_t real_len = 0;
	electric_relay_status_t electric_status;
	cinv_item_ret_t ret = CINV_OPER_SUCCESS;

	if (CINV_OPER_SUCCESS == cinv_item_read(NVDATA_ID_ELECTRIC_STATUS, sizeof(electric_relay_status_t), &electric_status, &real_len))
	{
		if (0 != memcmp(&electric_status, &g_electric_relay_status, sizeof(electric_relay_status_t)))
		{
			ret = cinv_item_write(NVDATA_ID_ELECTRIC_STATUS, sizeof(electric_relay_status_t), &g_electric_relay_status);
			if (ret != CINV_OPER_SUCCESS)
			{
				ci_logerr(CI_LOG_ERROR, "NVDATA_ID_ELECTRIC_STATUS write fail! FUNCTION = %s, LINE = %d\r\n", __FUNCTION__, __LINE__);
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}

	return 1;
}

/**
 * @brief HW延时定时器初始化
 *
 * @param [in] void
 * @param [out] None
 *
 * @return
 *
 * @history
 * 1.Date          : 11/10/2021 15:48:4
 *   Author       : zhh
 *   Modification : Created function
 */
static void hw_delay_timer_irq_handler(void)
{
	timer_clear_irq(DELAY_CTR_TIMER);

    if((1 == g_electric_relay_onoff)&&(0 == g_electric_relay_status.onOff))
    {
        //mprintf("open power\n");
        sys_msg_t send_msg;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        send_msg.msg_type = SYS_MSG_TYPE_SET_POWERONOFF;
        send_msg_to_sys_task(&send_msg, &xHigherPriorityTaskWoken); 
    }
    else
    {
       //wkq
        enable_zero_point_detect_irq();
        mprintf("power had on\n");
    }


}

/**
 * @brief HW TIMER UPDATE
 *
 * @param [in] void
 * @param [out] None
 *
 * @return
 *
 * @history
 * 1.Date          : 11/10/2021 15:48:4
 *   Author       : zhh
 *   Modification : Created function
 */
void hw_delay_relay_ctr_timer_update(uint32_t period)
{
    timer_stop(DELAY_CTR_TIMER);
    timer_set_count(DELAY_CTR_TIMER, DELAY_CTR_ONEUS_COUNT*period);
    timer_start(DELAY_CTR_TIMER);
}


/**
 * @brief hw延时定时器初始化
 *
 * @param [in] void
 * @param [out] None
 *
 * @return
 *
 * @history
 * 1.Date          : 11/10/2021 15:48:4
 *   Author       : zhh
 *   Modification : Created function
 */
void hw_delay_relay_ctr_init(void)
{
    timer_init_t delay_timer_init;

    __eclic_irq_set_vector(DELAY_CTR_TIMER_IRQn, (int)hw_delay_timer_irq_handler);
    eclic_irq_enable(DELAY_CTR_TIMER_IRQn);
    scu_set_device_gate(DELAY_CTR_TIMER,ENABLE);
    delay_timer_init.mode  = timer_count_mode_single; /* one shot mode */
    delay_timer_init.div   = timer_clk_div_0;   /* 1us = 50/2 period */
    delay_timer_init.width = timer_iqr_width_f;
    delay_timer_init.count = 20500000*5;
    timer_init(DELAY_CTR_TIMER, delay_timer_init);
}

/**
 * @brief 过零检测中断处理回调
 *
 * @param [in] void
 * @param [out] None
 *
 * @return static
 *
 * @history
 * 1.Date          : 9/11/2022 10:43:12
 *   Author       : wkq
 *   Modification : Created function
 */
static void zero_point_detect_irq_callback(void)
{
	static uint8_t times = 0;
	static uint8_t trigger_mode = 0;

	if (gpio_get_irq_mask_status_single(SCR_ZERO_PIN_BASE, SCR_ZERO_PIN_NUMBER))
	{
		//清除中断
		gpio_clear_irq_single(SCR_ZERO_PIN_BASE,SCR_ZERO_PIN_NUMBER);

		//屏蔽中断
		gpio_irq_mask(SCR_ZERO_PIN_BASE,SCR_ZERO_PIN_NUMBER);

		//设置继电器工作
        if(!trigger_mode)
        {
        	gpio_only_set_irq_trigger_config(SCR_ZERO_PIN_BASE, SCR_ZERO_PIN_NUMBER, high_level_trigger);
        }
        else
        {
        	gpio_only_set_irq_trigger_config(SCR_ZERO_PIN_BASE, SCR_ZERO_PIN_NUMBER, low_level_trigger);
        }

        touch_control_electric_relay_realonff();


       trigger_mode = ~trigger_mode;        
        
		//取消中断屏蔽
        //不在此打开中断使能,在电源已打开状态使能中断
		//gpio_irq_unmask(SCR_ZERO_PIN_BASE,SCR_ZERO_PIN_NUMBER);
	}
}


/**
 * @brief 过零检测初始化
 *
 * @param [in] void
 * @param [out] None
 *
 * @return
 *
 * @history
 * 1.Date          : 11/11/2022 16:29:3
 *   Author       : wkq
 *   Modification : Created function
 */
void zero_point_detect_init(void)
{
	/* 可控硅零点检测脚初始化 */
    scu_set_device_gate(SCR_ZERO_PIN_BASE,ENABLE);
    dpmu_set_io_reuse(SCR_ZERO_PIN_NAME,FIRST_FUNCTION); 
    gpio_set_input_mode(SCR_ZERO_PIN_BASE,SCR_ZERO_PIN_NUMBER);
    gpio_irq_trigger_config(SCR_ZERO_PIN_BASE,SCR_ZERO_PIN_NUMBER,low_level_trigger);
    registe_gpio_callback(SCR_ZERO_PIN_BASE, &zero_point_detect_irq_callback_node);

}

//使能中断 
void enable_zero_point_detect_irq(void)
{
	gpio_irq_unmask(SCR_ZERO_PIN_BASE,SCR_ZERO_PIN_NUMBER);//检测到电源是关闭状态,已经按时序打开电源之后,才使能中断引脚
}

//根据语音设置电源模式
void power_set_deal(uint32_t semantic_id)
{

		if (semantic_id == 4)
		{
			g_electric_relay_status.power_statue = ELECTRIC_RELAY_POWER_ON_ON;
		}
		else if (semantic_id == 5)
		{
			g_electric_relay_status.power_statue = ELECTRIC_RELAY_POWER_ON_OFF;
		}
		else if (semantic_id == 6)
		{
			g_electric_relay_status.power_statue = ELECTRIC_RELAY_POWER_ON_MEMORY;
		}

        sys_msg_t send_msg;
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	    send_msg.msg_type = SYS_MSG_TYPE_NV_SAVE;
	    send_msg_to_sys_task(&send_msg, &xHigherPriorityTaskWoken); 
}


//打开继电器开关 只记录状态
void electric_relay_switch_on(void)
{
    g_electric_relay_onoff=1;
}

//关闭继电器开关 只记录状态
void electric_relay_switch_off(void)
{
    g_electric_relay_onoff=0;
}


//触摸控制继电器开关 实际控制开和关,需要过零检测到才执行
void touch_control_electric_relay_realonff()
{

    //单独控制开
   // mprintf("g_electric_relay_onoff==%d\n",g_electric_relay_onoff);
  //  mprintf("g_electric_relay_status onOff==%d\n",g_electric_relay_status.onOff); 

    hw_delay_relay_ctr_timer_update(1200);//1500 1550 //1450



}


//续电器IO口初始化
static void electric_relay_io_init(void)
{
   
	scu_set_device_gate(ELECTRIC_RELAY_PIN_BASE, ENABLE);
     //PA6
	dpmu_set_io_reuse(ELECTRIC_RELAY_PIN6_NAME, ELECTRIC_RELAY_PIN6_FUNCTION);
    dpmu_set_io_direction(ELECTRIC_RELAY_PIN6_NAME,DPMU_IO_DIRECTION_OUTPUT);
	dpmu_set_io_pull(ELECTRIC_RELAY_PIN6_NAME, DPMU_IO_PULL_DISABLE);
	gpio_set_output_mode(ELECTRIC_RELAY_PIN_BASE, ELECTRIC_RELAY_PIN6_NUMBER);
    //PA5
    scu_set_device_gate(ELECTRIC_RELAY_PIN_BASE, ENABLE);
	dpmu_set_io_reuse(ELECTRIC_RELAY_PIN5_NAME, ELECTRIC_RELAY_PIN5_FUNCTION);
    dpmu_set_io_direction(ELECTRIC_RELAY_PIN5_NAME,DPMU_IO_DIRECTION_OUTPUT);
	dpmu_set_io_pull(ELECTRIC_RELAY_PIN5_NAME, DPMU_IO_PULL_DISABLE);
	gpio_set_output_mode(ELECTRIC_RELAY_PIN_BASE, ELECTRIC_RELAY_PIN5_NUMBER);


}
//打开继电器电源  只有开机初始化的时候调用
void open_electric_relay_power(void)
{
    g_electric_relay_onoff=1;
    g_electric_relay_status.onOff = g_electric_relay_onoff;   
    gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN6_NUMBER,0);
    gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN5_NUMBER,1);
    vTaskDelay(pdMS_TO_TICKS(300));
    gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN5_NUMBER,0);

    sys_msg_t send_msg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    send_msg.msg_type = SYS_MSG_TYPE_NV_SAVE;
    send_msg_to_sys_task(&send_msg, &xHigherPriorityTaskWoken); 


}

//关闭继电器电源 只有开机初始化的时候调用 //wkq语音播报也不用过零,直接关闭电源
void close_electric_relay_power(void)
{
 
    if((1 == g_electric_relay_onoff)&& (1==g_electric_relay_status.onOff) )
    {
        mprintf("close power\n");
        g_electric_relay_onoff=0;
        g_electric_relay_status.onOff = g_electric_relay_onoff; 
        gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN6_NUMBER,1);
        gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN5_NUMBER,0);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN6_NUMBER,0);
        sys_msg_t send_msg;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        send_msg.msg_type = SYS_MSG_TYPE_NV_SAVE;
        send_msg_to_sys_task(&send_msg, &xHigherPriorityTaskWoken);  
    }
    else
    {
        mprintf("close_electric_relay_power power had down\n");
    }
   
}

//只有开机初始化的时候调用 
void close_electric_only_poweron_relay_power(void)
{
 
    {
        mprintf("close power\n");
        g_electric_relay_onoff=0;
        g_electric_relay_status.onOff = g_electric_relay_onoff; 
        gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN6_NUMBER,1);
        gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN5_NUMBER,0);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN6_NUMBER,0);
        sys_msg_t send_msg;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        send_msg.msg_type = SYS_MSG_TYPE_NV_SAVE;
        send_msg_to_sys_task(&send_msg, &xHigherPriorityTaskWoken);  
    }
   
}



//上电初始化继电器 
void electric_relay_init(void)
{
    zero_point_detect_init();
    	// 继电器控制硬件延时定时初始化
	hw_delay_relay_ctr_init();

    electric_relay_io_init();

    electric_relay_status_nv_init();

    switch (g_electric_relay_status.power_statue)
    {
    case ELECTRIC_RELAY_POWER_ON_OFF:
        close_electric_only_poweron_relay_power();
        break;

    case ELECTRIC_RELAY_POWER_ON_ON:
        open_electric_relay_power();
        break;

    case ELECTRIC_RELAY_POWER_ON_MEMORY:
        if(g_electric_relay_status.onOff == 1)
        {
            open_electric_relay_power();
        }
        else
        {
            close_electric_only_poweron_relay_power();
        }
        break;
    
    default:
        break;
    }
}

