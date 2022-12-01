#include "ci_key.h"
#include "ci112x_scu.h"
#include "ci112x_adc.h"
#include "ci112x_gpio.h"
#include "ci112x_core_eclic.h"


/**************************************************************************
                   user define                    
****************************************************************************/
//*touchIC XW09B 示例代码，检测到中断后，读取电压值*//
#define VOLTAGE_DEVIATION       (0.05f)          //电压误差范围，单位v
#define REFERENCE_VOLTAGE       (3.3f)           //参考电压,单位v

//ADC 通道
#define ADC_KEY_CHANNEL         ADC_CHANNEL_1

//ADC引脚
#define ADC_KEY_PAD             AIN1_PAD

//GPIO 中断触发配置
#define TOUCH_ISR_PAD           UART1_TX_PAD
#define TOUCH_ISR_GPIO_GROUP    GPIO2        
#define TOUCH_ISR_PIN_NUM       gpio_pin_1
#define TOUCH_ISR_TYPE          GPIO2_IRQn        

/**************************************************************************
                   
****************************************************************************/
#ifndef F_ABS
    #define F_ABS(x) ((x)>0 ? (x) : -(x))
#endif

TimerHandle_t xTimer_touchkey = NULL; 
char touch_key_flag = 0; 


/**
 * @brief touch 按键硬件初始化，初始化ADC和GPIO中断
 * 
 */
void ci_key_touch_hw_init(void)
{
	eclic_irq_enable(ADC_IRQn);
    Scu_Setdiv_Parameter(HAL_ADC_BASE,11);
    
    Scu_SetDeviceGate(HAL_ADC_BASE,1);
    Scu_SetIOReuse(ADC_KEY_PAD,FIRST_FUNCTION);     
    Scu_SetADIO_REUSE(ADC_KEY_PAD,ANALOG_MODE);
    Scu_SetIOPull(ADC_KEY_PAD,DISABLE);
    adc_power_ctrl(ENABLE);  

    adc_continuons_convert(DISABLE);//关闭连续采样，这时才支持单次周期采样
    adc_period_enable(ENABLE);//ENABLE 周期采样
    adc_period_monitor(ADC_KEY_CHANNEL,ENABLE);//enable ch0的周期采样
    adc_chax_period(ADC_KEY_CHANNEL,10);//设定ch0的采样间隔
        
    adc_calibrate(DISABLE);
    adc_int_sel(ADC_INT_MODE_TRANS_END);//DISABLE 每次采样均产生中断
    adc_mask_int(DISABLE);//不屏蔽中断
    adc_convert_config(ADC_KEY_CHANNEL,ADC_CLKCYCLE_1);//ADC_CLKCYCLE_5
    adc_soc_soft_ctrl(ENABLE);

    Scu_SetDeviceGate((uint32_t)TOUCH_ISR_GPIO_GROUP,ENABLE);
    //配置管脚使用GPIO功能
    if(TOUCH_ISR_GPIO_GROUP>GPIO4)
    {
        Scu_SetIOReuse(TOUCH_ISR_PAD,SECOND_FUNCTION);
    }
    else
    {
        Scu_SetIOReuse(TOUCH_ISR_PAD,FIRST_FUNCTION);
    }
    //配置GPIO为输入模式
    gpio_set_input_mode(TOUCH_ISR_GPIO_GROUP,TOUCH_ISR_PIN_NUM);
    //配置GPIO下降沿触发
    gpio_irq_trigger_config(TOUCH_ISR_GPIO_GROUP,TOUCH_ISR_PIN_NUM,down_edges_trigger);

    eclic_irq_enable(TOUCH_ISR_TYPE);
}


/**
 * @brief touch 按键初始化，使用ADC和GPIO中断，修改ADC_KEY_CHANNEL，TOUCH_ISR_PAD相关宏定义即可
 * 
 */
void ci_key_touch_init(void)
{
    ci_key_touch_hw_init();
}


/**
 * @brief touch 按键GPIO中断处理函数，需要添加到GPIO中断内
 * 
 */
void ci_key_touch_gpio_isr_handle(void)
{
    if(gpio_get_irq_mask_status_single(TOUCH_ISR_GPIO_GROUP,TOUCH_ISR_PIN_NUM))
    {
        touch_key_flag = 1;
        gpio_clear_irq_single(TOUCH_ISR_GPIO_GROUP,TOUCH_ISR_PIN_NUM);
    }
    
}


/**
*@函数名称:ci_key_touch_deal
*@功能:touch按键获取按键值
*@注意:
*@参数:
*@返回值:int 按键值
*@日期:2019-7-4
*/
static uint8_t ci_key_touch_deal(void)
{    
    float adc_voltage_value = (((float)adc_get_result(ADC_KEY_CHANNEL)/4096.0)*REFERENCE_VOLTAGE);
    int key_index = KEY_NULL;

    for(int i=5;i<12;i++)
    {
        if((F_ABS(adc_voltage_value-(((float)i/16.0)*(REFERENCE_VOLTAGE))))<VOLTAGE_DEVIATION)
        {
            key_index = i-4;
            break;
        }
    }
    return key_index;
    
}


/**
 * @brief touch 按键ADC中断处理函数，需要添加到ADC中断内
 * 
 */
void ci_key_touch_adc_isr_handle(void)
{
    if(adc_int_flag(ADC_KEY_CHANNEL))
    {
        adc_int_clear(ADC_KEY_CHANNEL);
        BaseType_t xHigherPriorityTaskWoken;
        static int key_time = 0;
        static uint8_t key_index_pre = KEY_NULL;

        static uint32_t freed_time = 0;
        static uint32_t first_press_time = 0;
        static uint32_t long_time = 0;
        
        uint8_t key_num = KEY_NULL;
        sys_msg_key_data_t send_key_data ;
        send_key_data.key_index = KEY_NULL;
        
        if(touch_key_flag != 1)
        {
            return;
        }
        key_num = ci_key_touch_deal();
        if(key_num != KEY_NULL)
        {
            if(key_num == key_index_pre)
            {
                key_time++;
            }
            else
            {
                key_index_pre = key_num;
                key_time = 1;
            }
            
            if(key_time == KEY_SHORTPRESS_TIMER)
            {
                send_key_data.key_index = key_num;
                send_key_data.key_status = MSG_KEY_STATUS_PRESS;
                send_key_data.key_time_ms = 0;
                first_press_time = xTaskGetTickCountFromISR();
            }
            else if(key_time%KEY_LONGPRESS_TIMER == 0)
            {
                send_key_data.key_index= key_num;
                send_key_data.key_status = MSG_KEY_STATUS_PRESS_LONG;
                long_time = xTaskGetTickCountFromISR();
                send_key_data.key_time_ms = (long_time - first_press_time)*(1000/configTICK_RATE_HZ);
            }
        }
        else
        {
            if(key_index_pre != KEY_NULL)
            {
                freed_time = xTaskGetTickCountFromISR();

                send_key_data.key_index =key_index_pre;
                send_key_data.key_status =MSG_KEY_STATUS_RELEASE;
                send_key_data.key_time_ms = (freed_time - first_press_time)*(1000/configTICK_RATE_HZ);
                if(first_press_time == 0)
                {
                    send_key_data.key_index = KEY_NULL;
                }
                key_time = 0;
                key_index_pre = KEY_NULL;
                touch_key_flag = 0;
                long_time = 0;
            }

        }
        if(send_key_data.key_index != KEY_NULL)
        {
            sys_msg_t send_msg;
            send_msg.msg_type = SYS_MSG_TYPE_KEY;
            memcpy(&send_msg.msg_data,&send_key_data,sizeof(sys_msg_key_data_t));
            send_msg_to_sys_task(&send_msg,&xHigherPriorityTaskWoken);
            if( pdFALSE != xHigherPriorityTaskWoken )
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

