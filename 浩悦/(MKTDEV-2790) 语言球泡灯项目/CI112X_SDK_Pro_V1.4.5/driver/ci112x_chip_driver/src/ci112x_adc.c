/**
 * @file ci112x_adc.c
 * @brief CI112X芯片ADC模块的驱动程序
 * @version 0.1
 * @date 2019-05-17
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */


#include "ci112x_adc.h"
#include "ci_assert.h"

//ADC的时钟频率默认设置为15MHz
#define ADC_DIV_PARA (13U)

/**
 * @brief ADC电路电源使能
 * 
 * @param cmd 打开或者关闭
 */
void adc_power_ctrl(FunctionalState cmd)
{
    if(cmd != ENABLE)
    {
        ADC->ADCCTRL &= ~(1<<0);
	}
    else
    {
		ADC->ADCCTRL |= (1<<0);

	}
}


/**
 * @brief ADC连续转换使能
 * 
 * @param cmd 打开或关闭
 */
void adc_continuons_convert(FunctionalState cmd)
{
    if(cmd != ENABLE)
    {
        ADC->ADCCTRL &= ~(1 << 1);
	}
    else
    {
		ADC->ADCCTRL |= (1 << 1);
	}	
}


/**
 * @brief ADC校准使能
 * 
 * @param cmd 打开或关闭
 */
void adc_calibrate(FunctionalState cmd)
{
    if(cmd == ENABLE)
    {
		ADC->ADCCTRL |= (1 << 3);
	}
    else
    {
		ADC->ADCCTRL &= ~(1 << 3);
	}
}


/**
 * @brief ADC通道周期监测使能(只支持ADC_CHANNEL_0-ADC_CHANNEL_3)
 * 
 * @param channel adc通道选择
 * @param cmd 使能或关闭
 */
void adc_period_monitor(adc_channelx_t channel,FunctionalState cmd)
{
    if(cmd != DISABLE)
    {
        ADC->ADCCTRL |= (1 << (channel + 4));
	}
    else
    {
		ADC->ADCCTRL &= ~(1 << (channel + 4));
	}
}


/**
 * @brief ADC周期监测使能(只能工作于单次采样模式下）
 * 
 * @param cmd 打开或关闭
 */
void adc_period_enable(FunctionalState cmd)
{
    if(cmd != DISABLE)
    {
        ADC->ADCCTRL |= (1 << 8);
	}
    else
    {
		ADC->ADCCTRL &= ~(1 << 8);
	}
}


/**
 * @brief ADC中断产生条件选择
 * 
 * @param condi ADC_Int_Condition_Sample_Abnormal：采样值异常时产生中断
 *            ADC_INT_CONDITION_SAMPLE_END：每次采样结束都产生中断
 */
void adc_int_sel(adc_int_mode_t condi)
{
    if(ADC_INT_MODE_VALUE_NOT_MEET == condi)
    {
        ADC->ADCCTRL |= (1 << 12);
	}
    else
    {
		ADC->ADCCTRL &= ~(1 << 12);
	}
}


/**
 * @brief ADC通道采样结果低于阀值下限中断使能（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * 
 * @param channel 通道选择
 * @param cmd 使能或关闭
 */
void adc_channel_min_value_int(adc_channelx_t channel,FunctionalState cmd)
{
    if(cmd != DISABLE)
    {
        ADC->ADCCTRL |= (1 << (channel + 16));
	}
    else
    {
		ADC->ADCCTRL &= ~(1 << (channel + 16));
	}
}


/**
 * @brief ADC通道采样结果超过阀值上限中断使能（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * 
 * @param channel 通道选择
 * @param cmd 使能或关闭
 */
void adc_channel_max_value_int(adc_channelx_t channel,FunctionalState cmd)
{
    if(cmd != DISABLE)
    {
        ADC->ADCCTRL |= (1 << (channel + 20));
	}
    else
    {
		ADC->ADCCTRL &= ~(1 << (channel + 20));
	}
}


/**
 * @brief ADC中断屏蔽设置
 * 
 * @param cmd ENABLE：屏蔽ADC中断（不会产生ADC中断）
 *            DISABLE：不屏蔽ADC中断（会产生ADC中断）
 */
void adc_mask_int(FunctionalState cmd)
{
    if(cmd != DISABLE)
    {
        ADC->ADCINTMASK |= (1 << 0);
	}
    else
    {
		ADC->ADCINTMASK &= ~(1 << 0);
	}
}


/**
 * @brief 读取某个通道的ADC中断标志（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * 
 * @param channel 通道选择
 * @return uint32_t 0：此通道没有ADC中断
 *                非0：此通道有ADC中断
 */
uint32_t adc_int_flag(adc_channelx_t channel)
{
	return ADC->ADCINTFLG & (1 << channel);
}



/**
 * @brief ADC中断标志清除（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * 
 * @param channel 通道选择
 */
void adc_int_clear(adc_channelx_t channel)
{
	ADC->ADCINTCLR |= (1 << channel);
}


/**
 * @brief ADC软件触发（软件强制开始转换）
 * 
 * @param cmd 使能或者不使能
 */
void adc_soc_soft_ctrl(FunctionalState cmd)
{
    if(ENABLE == cmd)
    {
        ADC->ADCSOFTSOC |= 1;
    }
	else
    {
        ADC->ADCSOFTSOC &= ~(1 << 0);
    }
}


/**
 * @brief :ADC 转换通道和采样保持时间配置（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * 
 * @param channel 通道选择
 * @param holdtime 采样保持时间选择
 */
void adc_convert_config(adc_channelx_t channel,adc_clkcyclex_t holdtime)
{
	ADC->ADCSOCCTRL = (channel << 5)|(holdtime << 12);
}


/**
 * @brief 获取ADC某个通道的转换结果（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * 
 * @param channel 通道选择
 * @return unsigned short 某个通道的ADC转换结果（12位）
 */
uint16_t adc_get_result(adc_channelx_t channel)
{
	return ADC->ADCRESULT[channel] & 0xFFF;
}


/**
 * @brief ADC通道采样周期配置（配置之后采样周期为（period + 1）*512）
 * （只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * @param channel 通道选择
 * @param period 周期配置,单位ms(当ADC时钟为15MHz的时候)
 */
void adc_chax_period(adc_channelx_t channel,uint16_t period)
{
	ADC->CHxPERIOD[channel] = ((period*30) - 1) & 0XFFFF;
}


/**
 * @brief ADC 通道下限阀值配置（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * （只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * @param channel 通道选择
 * @param val 下限值
 */
void adc_channel_min_value(adc_channelx_t channel,uint16_t val)
{
    if(ADC_CHANNEL_0 == channel)
    {
        ADC->CH0MINVALUE = val & 0x0FFF;
    }
    else if(ADC_CHANNEL_1 == channel)
    {
        ADC->CH1MINVALUE = val & 0x0FFF;
    }
    else if(ADC_CHANNEL_2 == channel)
    {
        ADC->CH2MINVALUE = val & 0x0FFF;
    }
    else if(ADC_CHANNEL_3 == channel)   
    {
        ADC->CH3MINVALUE = val & 0x0FFF;
	}
    else
    {
        CI_ASSERT(0,"adc channel err!\n");
	}
}


/**
 * @brief ADC 通道上限阀值配置（只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * （只支持ADC_CHANNEL_0-ADC_CHANNEL_3）
 * @param channel 通道选择
 * @param val 上限值
 */
void adc_channel_max_value(adc_channelx_t channel,uint16_t val)
{
    if(ADC_CHANNEL_0 == channel)
    {
        ADC->CH0MAXVALUE = val & 0xFFFF;
    }else if(ADC_CHANNEL_1 == channel)
    {
        ADC->CH1MAXVALUE = val & 0xFFFF;
    }else if(ADC_CHANNEL_2 == channel)
    {
        ADC->CH2MAXVALUE = val & 0xFFFF;
    }else if(ADC_CHANNEL_3 == channel)
    {
        ADC->CH3MAXVALUE = val & 0xFFFF;
	}
    else
    {
        CI_ASSERT(0,"adc channel err!\n");
	}
}


/**
 * @brief 获取ADC的状态
 * 
 * @return int 0：空闲状态
 *           非0：busy状态
 */
int32_t adc_status(void)
{
	return ADC->ADCSTAT & (1 << 0);
}


#include "ci112x_core_eclic.h"
#include "ci112x_scu.h"

#if ADC_DRIVER_USE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#endif


/**
 * @brief 将浮点的电压值转换为寄存器值
 * 
 * @param vol 浮点电压值
 * @return uint32_t 寄存器值
 */
static uint32_t float_vol_to_int(float vol)
{
    float real_vol = 0.0f;
    if(vol > 3.3f)
    {
        real_vol = 3.3f;
    }
    else if(vol < 0.0f)
    {
        real_vol = 0.0f;
    }
    else
    {
        real_vol = vol;
    }
    uint32_t real_vol_reg = (uint32_t)(real_vol*4096.0f/3.3f);
    return real_vol_reg;
}


//ADC初始化结构体
typedef struct
{
    //每个通道初始化模式选择
    adc_mode_t cha_mode;
}adc_init_tmp_t;


//ADC转换完成，通过队列发送的结构体
typedef struct
{
    uint16_t val;
}adc_trans_done_t;



//ADC值的队列
#if ADC_DRIVER_USE_RTOS
//adc转换完成的队列是否初始化过的标志
static adc_init_tmp_t init_tmp_str[ADC_CHANNEL_MAX];
QueueHandle_t adc_trans_done_queue[ADC_CHANNEL_MAX] = {NULL}; 
#else
adc_int_callback_t adc_interrupt_callback[ADC_CHANNEL_MAX] = {NULL};
#endif

/**
 * @brief adc中断回调函数
 * 
 * @param cha ADC通道选择
 */
void adc_int_callback(adc_channelx_t cha)
{
    #if ADC_DRIVER_USE_RTOS
    uint16_t val = adc_get_result(cha);
    adc_trans_done_t val_str;
    val_str.val = val;
    if(NULL != adc_trans_done_queue[cha])
    {
        xQueueOverwriteFromISR(adc_trans_done_queue[cha],&val_str,NULL);
    }
    #else
    if(NULL != adc_interrupt_callback[cha])
    {
        adc_interrupt_callback[cha]();
    }
    #endif
}


/**
 * @brief 创建ADC转换完成的队列
 * 
 * @param cha ADC的通道
 */
#if ADC_DRIVER_USE_RTOS
static void adc_trans_done_queue_creat(adc_channelx_t cha)
{
    if( (ADC_MODE_INT == init_tmp_str[cha].cha_mode) || (ADC_MODE_CYCLE == init_tmp_str[cha].cha_mode) )
    {
        if(NULL == adc_trans_done_queue[cha])
        {
            adc_trans_done_queue[cha] = xQueueCreate(1, sizeof(adc_trans_done_t));
            if(NULL == adc_trans_done_queue[cha])
            {
                mprintf("adc_trans_done_queue creat fail\r\n");    
            }
        }
    }
} 
#endif



#if ADC_DRIVER_USE_RTOS
/**
 * @brief 获取ADC检测到的电压的值
 * 
 * @param cha ADC通道
 * @param vol_val 存储电压值的float指针
 * @param time 超时等待最长的时间
 * @return BaseType_t 是否成功获取ADC的值
 *                中断模式：电压值未异常：返回pdFALSE
 *                          电压值异常：返回pdTRUE
 *                连续模式：只返回pdTRUE，不会有队列操作，time测试无用
 *                周期模式：在规定时间等待ADC转换完成，如果没有转换完成，则返回pdFALSE
 */
BaseType_t adc_get_vol_value(adc_channelx_t cha,float* vol_val,uint32_t time)
{
    adc_mode_t mode = init_tmp_str[cha].cha_mode;
    if((ADC_MODE_INT == mode) || (ADC_MODE_CYCLE == mode))
    {
        adc_trans_done_t val_str;
        BaseType_t rslt = xQueueReceive(adc_trans_done_queue[cha],&val_str,pdMS_TO_TICKS(time));
        if(pdTRUE == rslt)
        {
            uint32_t val = val_str.val;
            *vol_val = (float)val*3.3f/4096.0f;
            return pdTRUE;
        }
        else
        {
            uint32_t val = adc_get_result(cha);
            *vol_val = (float)val*3.3f/4096.0f;
            return pdFALSE;
        }
    }
    else if(ADC_MODE_CONTINUE == mode)
    {
        uint32_t val = adc_get_result(cha);
        *vol_val = (float)val*3.3f/4096.0f;
        return pdTRUE;
    }
    else if(ADC_MODE_CALCULATE == mode)
    {
        while(adc_status());
        uint32_t val = adc_get_result(cha);
        *vol_val = (float)val*3.3f/4096.0f;
        return pdTRUE;
    }
    return pdFALSE;
}
#else
/**
 * @brief 获取ADC转换的电压值
 * 
 * @param cha ADC通道选择
 * @param vol_val 存储转换之后的电压值的指针
 * @return int8_t RETURN_OK
 */
int8_t adc_get_vol_value(adc_channelx_t cha,float* vol_val)
{
    uint32_t val = adc_get_result(cha);
    *vol_val = (float)val*3.3f/4096.0f;
    return RETURN_OK;
}
#endif


/**
 * @brief ADC预初始化的一些操作，包括IO初始化、ADC全局开关等
 * 
 * @param cha 
 */
static void adc_pre_init(adc_channelx_t cha)
{
    #if ADC_DRIVER_USE_RTOS
    adc_trans_done_queue_creat(cha);
	#endif
    eclic_irq_enable(ADC_IRQn);
	
    //设置模拟IO成模拟功能
    SCU->AD_IO_REUSE0_CFG |= (1 << cha);
    
    //模拟引脚配置成模拟功能后，IO复用功能配置无效。
    //Scu_SetIOReuse((PinPad_Name)((uint16_t)AIN0_PAD + (uint16_t)cha),FIRST_FUNCTION);

    //禁止内部上下拉
    Scu_SetIOPull((PinPad_Name)((uint16_t)AIN0_PAD + (uint16_t)cha),DISABLE);
    
    Scu_Setdiv_Parameter((unsigned int)ADC,ADC_DIV_PARA);
	
    Scu_SetDeviceGate((uint32_t)ADC,ENABLE);
    adc_power_ctrl(ENABLE);	
    adc_mask_int(ENABLE);//屏蔽中断
}


#if ADC_DRIVER_USE_RTOS
/**
 * @brief ADC中断模式初始化
 * 中断模式会每一次转换完成，或者采样值异常（可设置）之后进入一次中断，如果只是每一次转换完成之后进入中断的话，
 * 相当频繁，会严重影响系统运行，此驱动不开放这种使用方式。
 * 
 * @param ptr adc_int_mode_init_t结构体指针
 */
void adc_init_int_mode(const adc_int_mode_init_t* const ptr)
{
    init_tmp_str[ptr->cha].cha_mode = ADC_MODE_INT;
#else
/**
 * @brief ADC中断模式初始化
 * 中断模式会每一次转换完成，或者采样值异常（可设置）之后进入一次中断，如果只是每一次转换完成之后进入中断的话，
 * 相当频繁，会严重影响系统运行，此驱动不开放这种使用方式。
 * 
 * @param ptr adc_int_mode_init_t结构体指针
 * @param irq_irq_fun 中断回调函数
 */
void adc_init_int_mode(const adc_int_mode_init_t* const ptr,adc_int_callback_t irq_fun)
{
    adc_interrupt_callback[ptr->cha] = irq_fun;
#endif
    
    adc_pre_init(ptr->cha);
    volatile uint32_t tmp = 1000;
    adc_continuons_convert(ENABLE);//连续采样，这时才支持周期采样
    adc_period_enable(DISABLE);//ENABLE 周期采样
    adc_calibrate(DISABLE);

    //上下限设置
    adc_int_sel(ADC_INT_MODE_VALUE_NOT_MEET);//DISABLE 每次采样均产生中断
    adc_channel_min_value(ptr->cha,float_vol_to_int(ptr->lower_limit));
    adc_channel_max_value(ptr->cha,float_vol_to_int(ptr->upper_limit));
    adc_channel_max_value_int(ptr->cha,ENABLE);
    adc_channel_min_value_int(ptr->cha,ENABLE);

    adc_convert_config((ptr->cha),ADC_CLKCYCLE_1);//ADC_CLKCYCLE_5
    mprintf("start ADC!\n");
    while(tmp--);
    adc_mask_int(DISABLE);//不屏蔽中断
    adc_soc_soft_ctrl(ENABLE);
}


/**
 * @brief ADC连续模式初始化
 * 
 * @param cha ADC通道选择
 */
void adc_init_continue_mode(adc_channelx_t cha)
{
    #if ADC_DRIVER_USE_RTOS
    init_tmp_str[cha].cha_mode = ADC_MODE_CONTINUE;
    #else
    adc_interrupt_callback[cha] = NULL;
    #endif
    adc_pre_init(cha);
    volatile uint32_t tmp = 1000;
    
    adc_continuons_convert(ENABLE);//DISABLE
    adc_calibrate(DISABLE);//ENABLE
    adc_period_enable(DISABLE);
    adc_int_sel(ADC_INT_MODE_TRANS_END);
    adc_convert_config(cha,ADC_CLKCYCLE_1);
    while(tmp--);
    adc_soc_soft_ctrl(ENABLE);
}


/**
 * @brief ADC周期模式初始化
 * 
 * @param cha ADC通道选择
 * @param ms 每隔多少ms ADC完成一次转化
 */
#if ADC_DRIVER_USE_RTOS
void adc_init_cycle_mode(adc_channelx_t cha,uint32_t ms)
{
    init_tmp_str[cha].cha_mode = ADC_MODE_CYCLE;
#else
/**
 * @brief ADC周期模式初始化
 * 
 * @param cha ADC通道选择
 * @param ms 每隔多少ms ADC完成一次转化
 * @param irq_fun 中断回调函数
 */
void adc_init_cycle_mode(adc_channelx_t cha,uint32_t ms,adc_int_callback_t irq_fun)
{
    adc_interrupt_callback[cha] = irq_fun;
#endif
    adc_pre_init(cha);
    volatile uint32_t tmp = 1000;

    adc_continuons_convert(DISABLE);//关闭连续采样，这时才支持周期采样
    adc_period_enable(ENABLE);//ENABLE 周期采样
    adc_period_monitor(cha,ENABLE);//enable ch0的周期采样
    adc_chax_period(cha,ms);//设定ch0的采样间隔
    
    adc_calibrate(DISABLE);
    adc_int_sel(ADC_INT_MODE_TRANS_END);//DISABLE 每次采样均产生中断
    adc_convert_config(cha,ADC_CLKCYCLE_1);
    while(tmp--);
    adc_soc_soft_ctrl(ENABLE);
    adc_mask_int(DISABLE);//不屏蔽中断
}


/**
 * @brief ADC校准模式初始化
 * 
 * @param cha ADC通道选择
 */
void adc_init_caculate_mode(adc_channelx_t cha)
{
    #if ADC_DRIVER_USE_RTOS
    init_tmp_str[cha].cha_mode = ADC_MODE_CYCLE;
    #else
    adc_interrupt_callback[cha] = NULL;
    #endif
    adc_pre_init(cha);
    volatile uint32_t tmp = 1000;

    adc_calibrate(ENABLE);  //ADC校准使能
    adc_convert_config(cha,ADC_CLKCYCLE_1);//ADC_CLKCYCLE_5
    while(tmp--);
    adc_soc_soft_ctrl(ENABLE);
}




/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
