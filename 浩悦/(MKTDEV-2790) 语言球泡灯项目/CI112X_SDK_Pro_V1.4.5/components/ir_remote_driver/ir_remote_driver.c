/**
 * @file ir_remote_driver.c
 * @brief  红外驱动 need one timer,one pwm, pwm 38khz
 * @version 1.0
 * @date 2017-08-22
 *
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 *
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "ci112x_pwm.h"
#include "ci112x_timer.h"
#include "ci112x_spiflash.h"
#include "ci112x_gpio.h"
#include "ci112x_core_eclic.h"

#include "sdk_default_config.h"
#include "ir_remote_driver.h"

/**************************************************************************
                    type define
****************************************************************************/

#define IR_CODE_BUSY_STATE 1
#define IR_CODE_IDLE_STATE 0
#define IR_CARRIER_FREQUENCY (38000)    //载波频率
#define IR_CARRIER_DUTY (500)           //占空比
#define IR_CARRIER_DUTY_MAX (1000)           //最大占空比
#define IR_MAX_DATA_COUNT (1024)        //最大接收长度

#define TIMER0_ONEUS_COUNT (205/10)/*FPCLK/FEQDIV*/

#define IR_RECEIVE_DATA_TIMEOUT         (60000L)
#define IR_RECEIVE_DATA_TIMEOUT_USED    (100000L)
#define IR_RECEIVE_FIRST_LEVEL          (0)

typedef enum
{
    IR_RECEIVE_STATE_IDLE = 0,
    IR_RECEIVE_STATE_INIT,
    IR_RECEIVE_STATE_NOINPUT,/*timeout 5s*/
    IR_RECEIVE_STATE_START,
    IR_RECEIVE_STATE_DATA,
    IR_RECEIVE_STATE_END,/*timeout 20000us, ir code datebase used this value*/
    IR_RECEIVE_STATE_ERR,/*now if receive too short will error*/
}ir_receive_state_t;



/**************************************************************************
                    global
****************************************************************************/
static uint32_t ir_time_function = 1;/*1:send , 0:receive*/
static bool ir_code_busy = false;

static uint32_t ir_code_total_count = 0;
static uint32_t ir_code_send_count = 0;

static uint16_t *ir_level_code = NULL;
static uint32_t ir_level_code_size = 0;

static uint32_t ir_receive_state = IR_RECEIVE_STATE_IDLE;
static uint32_t ir_receive_level_flag = IR_RECEIVE_FIRST_LEVEL;/*1 high ,0 low*/
uint32_t ir_receive_level_count = 0;

static void ir_receive_process(void);
static gpio_irq_callback_list_t ir_gpio_callback = {ir_receive_process,NULL};

stIrPinInfo ir_driver_info =
{
    .outPin =
    {
        .PinName = IR_OUT_PWM_PIN_NAME,
        .GpioBase = IR_OUT_GPIO_PIN_BASE,
        .PinNum = IR_OUT_PWM_PIN_NUMBER,
        .PwmFun = IR_OUT_PWM_FUNCTION,
        .IoFun = IR_OUT_GPIO_FUNCTION,
        .PwmBase = IR_OUT_PWM_NAME,
    },
    .revPin =
    {
        .PinName = IR_REV_IO_PIN_NAME,
        .GpioBase = IR_REV_IO_PIN_BASE,
        .PinNum = IR_REV_IO_PIN_NUMBER,
        .IoFun = IR_REV_IO_FUNCTION,
        .GpioIRQ = IR_REV_IO_IRQ,
    },

    .irTimer =
    {
        .ir_use_timer = IR_USED_TIMER_NAME,
        .ir_use_timer_IRQ = IR_USED_TIMER_IRQ,
    }
};


/**
 * @brief pwm管脚输出使能
 *
 */
static void ir_pwm_out_pad_enable(void)
{
    Scu_SetIOReuse(ir_driver_info.outPin.PinName,ir_driver_info.outPin.PwmFun);
    pwm_start(ir_driver_info.outPin.PwmBase);
}


/**
 * @brief pwm管脚输出关闭
 *
 */
static void ir_pwm_out_pad_disable(void)
{
    pwm_stop(ir_driver_info.outPin.PwmBase);
    Scu_SetIOReuse(ir_driver_info.outPin.PinName,ir_driver_info.outPin.IoFun);/*gpio function*/
    gpio_set_output_level_single(ir_driver_info.outPin.GpioBase, ir_driver_info.outPin.PinNum, 0);
}


/**
 * @brief 数据发送底半部
 *
 */
static void send_ir_code_continue(void)
{
    if(ir_code_send_count>=ir_code_total_count)
    {
        ir_pwm_out_pad_disable();
        ir_code_busy = false;
        return;
    }
    if(0 == ir_level_code[ir_code_send_count])
    {
        ci_logerr(LOG_IR,"ir data error,wave value is 0\n");
        ir_pwm_out_pad_disable();
        ir_code_busy = false;
    }
    timer_stop(ir_driver_info.irTimer.ir_use_timer);
    timer_set_count(ir_driver_info.irTimer.ir_use_timer,(uint32_t)ir_level_code[ir_code_send_count]*TIMER0_ONEUS_COUNT*IR_DATA_DIV_COEFFICIENT);
    timer_start(ir_driver_info.irTimer.ir_use_timer);
    if(0 == (ir_code_send_count&0x1))/*high level*/
    {
        ir_pwm_out_pad_enable();
    }
    else
    {
        ir_pwm_out_pad_disable();
    }
    ir_code_send_count++;
}


/**
 * @brief 数据接收结束
 *
 */
void ir_receive_end(void)
{
    uint32_t i;

    ir_code_busy = false;
    ir_time_function = 1;
    eclic_irq_disable(ir_driver_info.revPin.GpioIRQ);
    gpio_set_output_mode(ir_driver_info.revPin.GpioBase, ir_driver_info.revPin.PinNum);
    //start_lowpower_cout();/*act a driver function, so this move to application control*/

    #if 1
    for(i=0;i<ir_receive_level_count;i++)
    {
        if(0 == (i%16))
        {
            ci_loginfo(LOG_IR,"\n");
        }
        ci_loginfo(LOG_IR,"%d ",ir_level_code[i]*IR_DATA_DIV_COEFFICIENT);
    }
    #endif
    if(16 > ir_receive_level_count)
    {
        ci_loginfo(LOG_IR,"rev error, too short IR_RECEIVE_STATE_ERR\n");
        ir_receive_state = IR_RECEIVE_STATE_ERR;
    }
}

/**
 * @brief 数据接收超时处理
 *
 */
static void ir_receive_timeout_deal(void)
{
//    int msg = 0;
    switch(ir_receive_state)
    {
        case IR_RECEIVE_STATE_INIT:
            ci_logerr(LOG_IR,"no input signal\n");
            ir_receive_state = IR_RECEIVE_STATE_NOINPUT;
            ir_receive_end();
            /*send a meassge,play error voice*/
            break;
        case IR_RECEIVE_STATE_DATA:
            ir_receive_state = IR_RECEIVE_STATE_END;
            /*ir receive done*/
            ir_level_code[ir_receive_level_count] = IR_RECEIVE_DATA_TIMEOUT/IR_DATA_DIV_COEFFICIENT;
            ir_receive_level_count++;
            ir_receive_end();
            break;
        default:
            break;
    }
}


/**
 * @brief timer中断处理函数
 *
 */
static void ir_timer_timeout_deal(void)
{
    if(1 == ir_time_function)
    {
        send_ir_code_continue();
    }
    else
    {
        ir_receive_timeout_deal();
    }
    timer_clear_irq(ir_driver_info.irTimer.ir_use_timer);
}


/**
 * @brief ir硬件初始化
 *
 * @retval RETURN_OK 初始化成功
 * @retval RETURN_ERR 初始化失败
 */
static int32_t ir_hw_init(void)
{
    pwm_init_t pwm_38k_init;
    timer_init_t timer0_init;

    Scu_SetDeviceGate(ir_driver_info.outPin.PwmBase,ENABLE);
    Scu_SetIOReuse(ir_driver_info.outPin.PinName,ir_driver_info.outPin.PwmFun);
    pwm_38k_init.clk_sel = 0;
    pwm_38k_init.freq = IR_CARRIER_FREQUENCY;
    pwm_38k_init.duty = IR_CARRIER_DUTY;
    pwm_38k_init.duty_max = IR_CARRIER_DUTY_MAX;
    pwm_init(ir_driver_info.outPin.PwmBase,pwm_38k_init);
    ir_pwm_out_pad_disable();

    //配置TIMER0的中断
    __eclic_irq_set_vector(ir_driver_info.irTimer.ir_use_timer_IRQ,(int)ir_timer_timeout_deal);
    eclic_irq_enable(ir_driver_info.irTimer.ir_use_timer_IRQ);
    Scu_SetDeviceGate(ir_driver_info.irTimer.ir_use_timer,ENABLE);
    timer0_init.mode  = timer_count_mode_single; /*one shot mode*/
    timer0_init.div   = timer_clk_div_4;/*1us = 50/2 period*/
    timer0_init.width    = timer_iqr_width_2;
    timer0_init.count = 20500000*5;
    timer_init(ir_driver_info.irTimer.ir_use_timer,timer0_init);

    /* IR receive IO */
    eclic_irq_disable(ir_driver_info.revPin.GpioIRQ);
    Scu_SetDeviceGate(ir_driver_info.revPin.GpioBase,ENABLE);
    Scu_SetIOPull(ir_driver_info.revPin.PinName,DISABLE);
    Scu_SetIOReuse(ir_driver_info.revPin.PinName,ir_driver_info.revPin.IoFun);/*gpio function*/
    gpio_set_input_mode(ir_driver_info.revPin.GpioBase, ir_driver_info.revPin.PinNum);
    gpio_irq_unmask(ir_driver_info.revPin.GpioBase, ir_driver_info.revPin.PinNum);
    gpio_irq_trigger_config(ir_driver_info.revPin.GpioBase,ir_driver_info.revPin.PinNum,both_edges_trigger);
    gpio_clear_irq_single(ir_driver_info.revPin.GpioBase,ir_driver_info.revPin.PinNum);
    registe_gpio_callback(ir_driver_info.revPin.GpioBase,&ir_gpio_callback);

    return RETURN_OK;
}



/**
 * @brief gpio中断处理函数
 *
 */
static void ir_receive_process(void)
{
    uint32_t count;
    if(gpio_get_irq_mask_status_single(ir_driver_info.revPin.GpioBase,ir_driver_info.revPin.PinNum))
    {
        //ci_loginfo(LOG_IR,"gpio int\n");

        timer_get_count(ir_driver_info.irTimer.ir_use_timer,(unsigned int*)(&count));

        timer_stop(ir_driver_info.irTimer.ir_use_timer);

        timer_set_count(ir_driver_info.irTimer.ir_use_timer,TIMER0_ONEUS_COUNT*IR_RECEIVE_DATA_TIMEOUT_USED);
        timer_start(ir_driver_info.irTimer.ir_use_timer);

        if(IR_RECEIVE_STATE_INIT == ir_receive_state)
        {
            ir_receive_state = IR_RECEIVE_STATE_DATA;
            ir_receive_level_flag = IR_RECEIVE_FIRST_LEVEL;
        }
        else
        {
            if(ir_receive_level_count >= IR_MAX_DATA_COUNT)
            {
                ir_receive_state = IR_RECEIVE_STATE_END;
                ir_receive_end();
            }
            else
            {

                ir_receive_level_flag ^= 1;

                //count = TIMER0_ONEUS_COUNT*IR_RECEIVE_DATA_TIMEOUT - count;
                count = TIMER0_ONEUS_COUNT*IR_RECEIVE_DATA_TIMEOUT_USED - count;
                //mprintf("%d%\t",count/TIMER0_ONEUS_COUNT);
                /*处理连接码持续时间超过65535us的电平*/
                ir_level_code[ir_receive_level_count] = count/TIMER0_ONEUS_COUNT/IR_DATA_DIV_COEFFICIENT;
                ir_receive_level_count ++;
            }
        }

        if(ir_receive_level_flag != gpio_get_input_level_single(ir_driver_info.revPin.GpioBase,ir_driver_info.revPin.PinNum))
        {
            ci_logerr(LOG_IR,"receive error,need retryaaaaa\n");
            ir_receive_state = IR_RECEIVE_STATE_ERR;
            ir_receive_level_count = 0;
            ir_receive_end();
        }

    }

}


/**
 * @brief 红外收发硬件管脚配置，定时器配置
 *
 * @param pIrPinInfo
 * @retval RETURN_OK 配置成功
 * @retval RETURN_ERR 配置失败
 */
int32_t ir_setPinInfo(stIrPinInfo* pIrPinInfo)
{
    if (NULL == pIrPinInfo)
    {
        return RETURN_ERR;
    }

    memcpy((void*)&ir_driver_info,(void*)pIrPinInfo,sizeof(stIrPinInfo));
    return RETURN_OK;
}


/**
 * @brief 配置数据buf地址
 *
 * @param addr buf地址
 * @param size 大小
 * @retval RETURN_OK 配置成功
 * @retval RETURN_ERR 配置失败
 */
int32_t set_ir_level_code_addr(uint32_t addr, uint32_t size)
{
    if (size < (1024*2))
    {
        return RETURN_ERR;
    }

    ir_level_code_size = size;
    ir_level_code = (uint16_t *)addr;

    return RETURN_OK;
}


/**
 * @brief 获取数据buf地址
 *
 * @return uint16_t* buf地址
 */
uint16_t *get_ir_level_code_addr(void)
{
    return ir_level_code;
}


/**
 * @brief 数据发送初始化
 *
 */
void ir_send_init(void)
{
    static bool hw_Init = false;
    if (false == hw_Init)
    {
        ir_hw_init();
        hw_Init = true;
    }

    ir_code_send_count = 0;
    ir_code_total_count = 0;
	ir_time_function = 1;
    ir_code_busy = false;
    memset(ir_level_code,0,ir_level_code_size);/*mask for debug*/
}


/**
 * @brief 获取ir数据buf地址
 *
 * @return uint16_t* buf地址
 */
uint16_t * get_ir_driver_buf(void)
{
    if(true == ir_code_busy)/*busy state*/
    {
        return NULL;
    }
    else
    {
        return ir_level_code;
    }
}


/**
 * @brief 红外数据发送
 * @note no support multi thread,used this only one task!
 * @param count uint32_t freq,uint32_t dutycycle  now just 38khz 50%
 *
 * @retval RETURN_OK 发送成功
 * @retval RETURN_ERR 发送失败
 */
int32_t send_ir_code_start(uint32_t count)
{
    /*if busy just discard message, demo code no return error*/
    if(true == ir_code_busy)
    {
        return RETURN_ERR;
    }

    ir_code_busy = true;
    ir_code_send_count = 0;
    ir_code_total_count = count;

    ci_loginfo(LOG_IR,"read ir ok\n");
    send_ir_code_continue();
    return RETURN_OK;
}


/**
 * @brief 红外数据开始接收
 *
 */
void ir_receive_start(int time_out_seconds)
{
    ir_code_busy = true;
    ir_time_function = 0;

    /*io input mode,interrupt enable*/
    gpio_set_input_mode(ir_driver_info.revPin.GpioBase, ir_driver_info.revPin.PinNum);
    eclic_irq_enable(ir_driver_info.revPin.GpioIRQ);

    timer_stop(ir_driver_info.irTimer.ir_use_timer);
    timer_set_count(ir_driver_info.irTimer.ir_use_timer,time_out_seconds*20500000);
    timer_start(ir_driver_info.irTimer.ir_use_timer);

    ir_receive_state = IR_RECEIVE_STATE_INIT;
    ir_receive_level_flag = IR_RECEIVE_FIRST_LEVEL;
    ir_receive_level_count = 0;
}


/**
 * @brief 检查红外接收是否正确
 *
 * @retval RETURN_OK 接收正确
 * @retval RETURN_ERR 接收出错
 */
int32_t check_ir_receive(void)
{
    if((IR_RECEIVE_STATE_NOINPUT == ir_receive_state)\
    ||(IR_RECEIVE_STATE_ERR == ir_receive_state)\
    ||(IR_RECEIVE_STATE_INIT == ir_receive_state))
    {
        ci_loginfo(LOG_IR,"check receive error\n");
        return RETURN_ERR;
    }
    ci_loginfo(LOG_IR,"check receive ok\n");
    return RETURN_OK;
}


/**
 * @brief 检查红外发送状态是否空闲
 *
 * @retval RETURN_OK 空闲
 * @retval RETURN_ERR 非空闲
 */
int32_t check_ir_busy_state(void)
{
    if(true == ir_code_busy)
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}

uint32_t get_receive_level_count(void)
{
    return ir_receive_level_count;
}
