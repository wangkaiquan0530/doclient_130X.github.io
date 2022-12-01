#include "ci_key.h"
#include "ci_log.h"


/**
 * @brief a sample code key initial
 * 
 */
void ci_key_init(void)
{
    /*gpio 按键初始化*/
    ci_key_gpio_init();
    /*adc 按键初始化*/
    ci_key_adc_init();
    /*touch 按键初始化*/
    //ci_key_touch_init();
}


/**
 * @brief a sample code key infomation printf
 * 
 * @param key_msg 
 */
void key_info_show(sys_msg_key_data_t *key_msg)
{    
    if(key_msg->key_index != KEY_NULL)
    {
        ci_loginfo(LOG_USER,"key_value is 0x%x ",key_msg->key_index);
        if(MSG_KEY_STATUS_PRESS == key_msg->key_status)
        {
            ci_loginfo(LOG_USER,"status : press down\n");
        }
        else if(MSG_KEY_STATUS_PRESS_LONG == key_msg->key_status)
        {
            ci_loginfo(LOG_USER,"status : long press\n");
        }
        else if(MSG_KEY_STATUS_RELEASE == key_msg->key_status)
        {
            ci_loginfo(LOG_USER,"status : release\n");
        }
        if(key_msg->key_status == MSG_KEY_STATUS_RELEASE)
        {
            ci_loginfo(LOG_USER,"按键时间为：%d ms\n",key_msg->key_time_ms);
        }
        /*user code*/                
    }
}


