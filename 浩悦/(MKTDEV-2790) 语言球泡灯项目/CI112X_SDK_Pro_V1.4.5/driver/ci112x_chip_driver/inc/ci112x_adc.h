/**
 * @file ci112x_adc.h
 * @brief CI112X芯片ADC模块的驱动程序头文件
 * @version 0.1
 * @date 2019-05-17
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */

#ifndef __CI112X_ADC_H
#define __CI112X_ADC_H


//是否使用操作系统相关的队列，如果不适用则使用回调函数模式
#define ADC_DRIVER_USE_RTOS 0


#include "ci112x_system.h"
#if ADC_DRIVER_USE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif
typedef void (*adc_int_callback_t)(void);



//采样保持时间选择
typedef enum
{
    //采样保持1个cycle
    ADC_CLKCYCLE_1 = 0,
    //采样保持2个cycle
	ADC_CLKCYCLE_2	=1,
    //采样保持3个cycle
	ADC_CLKCYCLE_3	=3,
    //采样保持4个cycle
	ADC_CLKCYCLE_4	=4,
    //采样保持5个cycle
	ADC_CLKCYCLE_5	=5,
}adc_clkcyclex_t;


/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_adc ci112x_adc
 * @brief CI112X芯片ADC驱动，不再需要配置每个寄存器，只要按需选择ADC模式，然后调用相应接口
 * @{
 */
/**
 * @brief ADC通道编号
 * 
 */
typedef enum
{
    ADC_CHANNEL_0	=0,
    ADC_CHANNEL_1	=1,
    ADC_CHANNEL_2	=2,
    ADC_CHANNEL_3	=3,
    ADC_CHANNEL_MAX =4,
}adc_channelx_t;


/**
 * @brief ADC中断条件选择
 * 
 */
typedef enum
{
    //!每次转换完成触发中断
    ADC_INT_MODE_TRANS_END = 0,
    //!采样值出现异常的时候触发中断
    ADC_INT_MODE_VALUE_NOT_MEET = 1,
}adc_int_mode_t;


/**
 * @brief ADC模式选择
 * 
 */
typedef enum
{
    //!什么都不是
    ADC_MODE_NULL = 0,
    //!中断模式
    ADC_MODE_INT,
    //!连续模式
    ADC_MODE_CONTINUE,
    //!周期模式
    ADC_MODE_CYCLE,
    //!校准模式
    ADC_MODE_CALCULATE,
}adc_mode_t;


/**
 * @brief ADC返回结果成功标志
 * 
 */
typedef enum
{
    //!获取结果成功
    ADC_GET_RSLT_SUCCESS = 0,
    //!获取结果失败
    ADC_GET_RSLT_FAIL,
}adc_get_rslt_t;


/**
 * @brief ADC中断模式初始化结构体
 * 
 */
typedef struct
{
    //!通道选择（只能是ADC_CHANNEL_0、ADC_CHANNEL_1、ADC_CHANNEL_2、ADC_CHANNEL_3中的一个）
    adc_channelx_t cha;
    //!上限设置、高于此值进入中断
    float upper_limit;
    //!下限设置、低于此值进入中断
    float lower_limit;
}adc_int_mode_init_t;
/**
 * @}
 */


void adc_power_ctrl(FunctionalState cmd);
void adc_continuons_convert(FunctionalState cmd);
void adc_calibrate(FunctionalState cmd);
void adc_period_monitor(adc_channelx_t channel,FunctionalState cmd);
void adc_period_enable(FunctionalState cmd);
void adc_int_sel(adc_int_mode_t condi);
void adc_channel_min_value_int(adc_channelx_t channel,FunctionalState cmd);
void adc_channel_max_value_int(adc_channelx_t channel,FunctionalState cmd);
void adc_mask_int(FunctionalState cmd);	
uint32_t adc_int_flag(adc_channelx_t channel);
void adc_int_clear(adc_channelx_t channel);	
void adc_soc_soft_ctrl(FunctionalState cmd);
void adc_convert_config(adc_channelx_t channel,adc_clkcyclex_t holdtime);
uint16_t adc_get_result(adc_channelx_t channel);
void adc_chax_period(adc_channelx_t channel,unsigned short period);
void adc_channel_min_value(adc_channelx_t channel,unsigned short val);
void adc_channel_max_value(adc_channelx_t channel,unsigned short val);  
int32_t adc_status(void);	

/**
 * @addtogroup ci112x_adc
 * @{
 */

//新加入的接口，可以方便地使用各种模式的ADC
void adc_int_callback(adc_channelx_t cha);

void adc_init_caculate_mode(adc_channelx_t cha);
void adc_init_continue_mode(adc_channelx_t cha);
#if ADC_DRIVER_USE_RTOS
void adc_init_int_mode(const adc_int_mode_init_t* const ptr);
void adc_init_cycle_mode(adc_channelx_t cha,uint32_t ms);
BaseType_t adc_get_vol_value(adc_channelx_t cha,float* vol_val,uint32_t time);
#else
void adc_init_int_mode(const adc_int_mode_init_t* const ptr,adc_int_callback_t irq_fun);
void adc_init_cycle_mode(adc_channelx_t cha,uint32_t ms,adc_int_callback_t irq_fun);
int8_t adc_get_vol_value(adc_channelx_t cha,float* vol_val);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*__CI100X_ADC_H*/
