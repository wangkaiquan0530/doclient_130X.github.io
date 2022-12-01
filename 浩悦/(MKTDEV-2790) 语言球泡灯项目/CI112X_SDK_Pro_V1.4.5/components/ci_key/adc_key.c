#include "ci_key.h"
#include "ci112x_scu.h"
#include "ci112x_adc.h"
#include "ci112x_uart.h"
#include "ci112x_core_eclic.h"


/**************************************************************************
                   user define                    
****************************************************************************/

//ADC 通道
#define ADC_KEY_CHANNEL         ADC_CHANNEL_1 

//ADC引脚
#define ADC_KEY_PAD             AIN1_PAD

//电压误差范围，单位v
#define VOLTAGE_DEVIATION       (0.4f)

//参考电压,单位v
#define REFERENCE_VOLTAGE       (3.3f)

typedef struct
{
    uint8_t adc_key_index;              /*!< 按键值 */
    float  voltage_quantification;      /*!< 电压中间值 */
}adc_key_info;

//按键电压值，下面数组从大到小，用户根据实际值填写
static adc_key_info keyadc_buff[]=
{
    //4000表示无按键
    {KEY_NULL,3.25f},
    //2800表示按键一被按下
    {1,2.25f}, 
    //2200表示按键二被按下
    {2,1.77f},
};

/**************************************************************************
                   
****************************************************************************/

#ifndef F_ABS
    #define F_ABS(x) ((x)>0 ? (x) : -(x)) /*here used float type, M4F hardware float*/
#endif


/**
 * @brief 初始化按键，主要初始化ADC
 * 
 */
static void ci_key_adc_hw_init(void)
{
    Scu_Setdiv_Parameter(HAL_ADC_BASE,11);
    
    Scu_SetDeviceGate(HAL_ADC_BASE,1);
    Scu_SetIOReuse(ADC_KEY_PAD,SECOND_FUNCTION);     
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
    eclic_irq_enable(ADC_IRQn);
    adc_soc_soft_ctrl(ENABLE);
}


/**
 * @brief ADC按键初始化，初始化ADC硬件，如需修改按键，修改adc_key_info，ADC_KEY_CHANNEL，ADC_KEY_PAD
 * 
 */
void ci_key_adc_init(void)
{
    ci_key_adc_hw_init();
}


/**
 * @brief ADC 按键中断处理函数，如果使用ADC按键，需要将该函数放到adc中断处理函数里面，处理去抖，按键按下，长按，短按
 * 
 */
void ci_adc_key_isr_handle(void)
{
    if(adc_int_flag(ADC_KEY_CHANNEL))
    {
        adc_int_clear(ADC_KEY_CHANNEL);
        BaseType_t xHigherPriorityTaskWoken;
        sys_msg_key_data_t key_send_data;
        static uint32_t freed_time = 0;
        static uint32_t first_press_time = 0;
        static uint32_t long_time = 0;
        
        key_send_data.key_index = KEY_NULL;
        int key_num = KEY_NULL;
        float adc_voltage_value = (((float)adc_get_result(ADC_KEY_CHANNEL)/4096.0)*REFERENCE_VOLTAGE);
        static int key_time = 0;
        static int key_num_pre = KEY_NULL;

        for(int i=0;i<(sizeof(keyadc_buff)/sizeof(keyadc_buff[0]));i++)
        {
            if(F_ABS(adc_voltage_value - keyadc_buff[i].voltage_quantification)<VOLTAGE_DEVIATION)
            {
                if(i==0)
                {
                    key_num = KEY_NULL;
                }
                else
                {
                    key_num = keyadc_buff[i].adc_key_index;
                }
                
                break;
            }
        }

        if(key_num != KEY_NULL)
        {
            if(key_num == key_num_pre)
            {
                key_time++;
            }
            else
            {
                key_num_pre = key_num;
                key_time = 1;
            }
            
            if(key_time == KEY_SHORTPRESS_TIMER)
            {
                key_send_data.key_index =  key_num;
                key_send_data.key_status = MSG_KEY_STATUS_PRESS;
                key_send_data.key_time_ms = 0;
                first_press_time = xTaskGetTickCountFromISR();
            }
            else if(key_time%KEY_LONGPRESS_TIMER == 0)
            {
                key_send_data.key_index =  key_num;
                key_send_data.key_status = MSG_KEY_STATUS_PRESS_LONG;
                long_time = xTaskGetTickCountFromISR();
                key_send_data.key_time_ms = (long_time - first_press_time)*(1000/configTICK_RATE_HZ);
            }
        }
        else
        {
            if(key_num_pre != KEY_NULL)
            {    
                freed_time = xTaskGetTickCountFromISR();
                key_send_data.key_index =  key_num_pre;
                key_send_data.key_status = MSG_KEY_STATUS_RELEASE;
                key_send_data.key_time_ms = (freed_time - first_press_time)*(1000/configTICK_RATE_HZ);
                if(first_press_time == 0)
                {
                    key_send_data.key_index = KEY_NULL;
                }
                key_time = 0;
                key_num_pre = KEY_NULL;
                first_press_time = 0;
                freed_time = 0;
                long_time = 0;
            }
        }
        if(key_send_data.key_index != KEY_NULL )
        {    
            sys_msg_t send_msg;
            send_msg.msg_type = SYS_MSG_TYPE_KEY;
            memcpy(&send_msg.msg_data,&key_send_data,sizeof(sys_msg_key_data_t));
            send_msg_to_sys_task(&send_msg,&xHigherPriorityTaskWoken);

            if( pdFALSE != xHigherPriorityTaskWoken )
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

