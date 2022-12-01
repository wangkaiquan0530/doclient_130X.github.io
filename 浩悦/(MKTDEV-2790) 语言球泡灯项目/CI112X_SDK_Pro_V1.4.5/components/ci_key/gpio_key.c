#include "ci_key.h"
#include "ci112x_gpio.h"
#include "string.h"
#include "ci112x_scu.h"
#include "ci_log.h"
#include "ci112x_core_eclic.h"

/**************************************************************************
                   user define                    
****************************************************************************/

//GPIO 按键示例代码，
typedef struct 
{
    uint8_t key_index;          /*!< 按键值 */
    PinPad_Name pinpad_name;    /*!< 管脚名称 */
    gpio_base_t gpio_group;     /*!< gpio组 */
    gpio_pin_t pin_num;         /*!< gpio num */
}gpio_key_info;

static gpio_key_info gpio_key_list[]=
{
    //按键一
    {4,I2C0_SCL_PAD,GPIO3,gpio_pin_0},
    //按键二
    {5,I2C0_SDA_PAD,GPIO3,gpio_pin_1},
};

//GPIO_KEY_EFFECTIVE 定义为0 低电平有效；定义为1 高电平有效。
#define GPIO_KEY_EFFECTIVE      (0)


/**************************************************************************
                    
****************************************************************************/
#define GPIO_KEY_NUMBER (sizeof(gpio_key_list)/sizeof(gpio_key_list[0]))

TimerHandle_t xTimer_gpio_key = NULL; 

static void vtimer_gpio_callback(xTimerHandle pxTimer);


/**
 * @brief GPIO按键,硬件初始化
 * 
 */
void ci_key_gpio_hw_init(void)
{
    for(int i=0;i<GPIO_KEY_NUMBER;i++)
    {
        Scu_SetDeviceGate((uint32_t)gpio_key_list[i].gpio_group,ENABLE);
        //配置管脚使用GPIO功能
        if(gpio_key_list[i].gpio_group>GPIO4)
        {
            Scu_SetIOReuse(gpio_key_list[i].pinpad_name,SECOND_FUNCTION);
        }
        else
        {
            Scu_SetIOReuse(gpio_key_list[i].pinpad_name,FIRST_FUNCTION);
        }
        
        //配置GPIO为输入模式
        gpio_set_input_mode(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num);

#if (GPIO_KEY_EFFECTIVE == 0)
        //配置GPIO下降沿触发
        gpio_irq_trigger_config(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num,down_edges_trigger);
#else
        //配置GPIO上升沿触发
        gpio_irq_trigger_config(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num,up_edges_trigger);
#endif
        //使能中断
        if(gpio_key_list[i].gpio_group == GPIO0)
        {
        	eclic_irq_enable(GPIO0_IRQn);
        }
        else if(gpio_key_list[i].gpio_group == GPIO1)
        {
        	eclic_irq_enable(GPIO1_IRQn);
        }
        else if(gpio_key_list[i].gpio_group == GPIO2)
        {
        	eclic_irq_enable(GPIO2_IRQn);
        }
        else if(gpio_key_list[i].gpio_group == GPIO3)
        {
        	eclic_irq_enable(GPIO3_IRQn);
        }
        else
        {
        	eclic_irq_enable(GPIO4_IRQn);
        }
    }
}


/**
 * @brief GPIO按键创建软件timer
 * 
 */
static void ci_key_gpio_timer_init(void)
{
    xTimer_gpio_key = xTimerCreate("gpio_key_check",
                        /*定时溢出周期， 单位是任务节拍数*/
                        pdMS_TO_TICKS(10),   
                        /*是否自动重载， 此处设置周期性执行*/
                        pdTRUE,
                        /*定时器ID*/
                        (void *) 1,
                        /*回调函数*/
                        vtimer_gpio_callback);
}


/**
 * @brief gpio 按键初始化，初始化IO和软件定时器，如需修改使用不同的IO，修改gpio_key_list即可
 * 
 */
void ci_key_gpio_init(void)
{
    ci_key_gpio_timer_init();
    ci_key_gpio_hw_init();
}


/**
 * @brief gpio 按键中断处理函数，如果使用GPIO按键功能，需要添加到gpio中断内
 * 
 */
void ci_key_gpio_isr_handle(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    for(int i = 0;i<GPIO_KEY_NUMBER;i++)
    {
        if(gpio_get_irq_mask_status_single(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num))
        {
            //清除中断
            gpio_clear_irq_single(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num);
            //屏蔽中断
            gpio_irq_mask(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num);
            xTimerResetFromISR(xTimer_gpio_key,&xHigherPriorityTaskWoken);
            if(xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR(pdTRUE);
            }
        }
    }

}


void ci_key_gpio_deal(sys_msg_key_data_t* key_buff)
{
    sys_msg_key_data_t *key_buff_temp = key_buff;

    //低电平按键有效
    for(int i = 0;i<GPIO_KEY_NUMBER;i++)
    {

        if(gpio_get_input_level_single(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num)==GPIO_KEY_EFFECTIVE)
        {
            key_buff_temp[i].key_index = gpio_key_list[i].key_index;
        }
        else
        {
            key_buff_temp[i].key_index = KEY_NULL;
        }
    }
}


/**
 * @brief gpio按键，timer 回调函数，处理按键去抖，按下，短按，长按
 * 
 * @param pxTimer timer handle
 */
static void vtimer_gpio_callback(xTimerHandle pxTimer)
{
    //第一次按键按下时间
    static TickType_t first_press_time[GPIO_KEY_NUMBER] = {0};
    //按键释放时间
    static TickType_t freed_time[GPIO_KEY_NUMBER] = {0};
    static TickType_t long_time[GPIO_KEY_NUMBER] = {0};
    static uint8_t  init_flag = 1;
    static uint8_t vaild_key_number = 0;
    static int key_time[GPIO_KEY_NUMBER] = {0};
    static uint8_t key_index_pre[GPIO_KEY_NUMBER];
    sys_msg_key_data_t key_buff[GPIO_KEY_NUMBER]={MSG_KEY_STATUS_PRESS,KEY_NULL,0};
    sys_msg_key_data_t send_key_buff[ GPIO_KEY_NUMBER]={MSG_KEY_STATUS_PRESS,KEY_NULL,0};
    
    if(init_flag == 1)
    {
        init_flag = 0;
        memset(key_index_pre,KEY_NULL,sizeof(key_index_pre));
    }
    
    for(int i=0;i<GPIO_KEY_NUMBER;i++)
    {
        gpio_irq_unmask(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num);
        key_buff[i].key_index =  KEY_NULL;
        send_key_buff[i].key_index = KEY_NULL;
    }
    
    ci_key_gpio_deal(key_buff);
    for(int i=0;i<GPIO_KEY_NUMBER;i++)
    {
        if(key_buff[i].key_index != KEY_NULL)
        {
            if(key_buff[i].key_index == key_index_pre[i])
            {
                key_time[i]++;
            }
            else
            {
                key_index_pre[i] = key_buff[i].key_index;
                key_time[i] = 1;
            }
            
            if(key_time[i] == KEY_SHORTPRESS_TIMER)//消抖处理
            {
                send_key_buff[i].key_index = key_buff[i].key_index;
                send_key_buff[i].key_status = MSG_KEY_STATUS_PRESS;
                send_key_buff[i].key_time_ms = 0;
                first_press_time[i] = xTaskGetTickCount();
                vaild_key_number++;
            }
            else if(key_time[i]%KEY_LONGPRESS_TIMER == 0)
            {
                send_key_buff[i].key_index = key_buff[i].key_index;
                send_key_buff[i].key_status = MSG_KEY_STATUS_PRESS_LONG;
                long_time[i] = xTaskGetTickCount();
                send_key_buff[i].key_time_ms = (long_time[i] - first_press_time[i])*(1000/configTICK_RATE_HZ);
            }
        }
        else
        {
            if(key_index_pre[i] != KEY_NULL)
            { 
                
                freed_time[i] = xTaskGetTickCount();
                send_key_buff[i].key_index = key_index_pre[i];
                send_key_buff[i].key_status = MSG_KEY_STATUS_RELEASE;
                send_key_buff[i].key_time_ms = (freed_time[i] - first_press_time[i])*(1000/configTICK_RATE_HZ);
                
                key_time[i] = 0;
                key_index_pre[i] = KEY_NULL;
                vaild_key_number--;
                if(vaild_key_number == 0)
                {
                    xTimerStop(xTimer_gpio_key,0);
                }
                if(first_press_time[i] == 0)
                {
                    send_key_buff[i].key_index = KEY_NULL;
                }
                freed_time[i] = 0;
                first_press_time[i]= 0;
                long_time[i]  = 0;
            }
        }
        gpio_irq_unmask(gpio_key_list[i].gpio_group,gpio_key_list[i].pin_num);
        if(send_key_buff[i].key_index != KEY_NULL)
        {
            sys_msg_t send_msg;
            send_msg.msg_type = SYS_MSG_TYPE_KEY;
            memcpy(&send_msg.msg_data,&send_key_buff[i],sizeof(sys_msg_key_data_t));
            send_msg_to_sys_task(&send_msg,0);
        }

    }
}
