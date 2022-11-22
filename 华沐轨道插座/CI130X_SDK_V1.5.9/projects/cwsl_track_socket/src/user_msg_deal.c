#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "system_msg_deal.h"
#include "prompt_player.h"
#include "voice_module_uart_protocol.h"
#include "i2c_protocol_module.h"
#include "ci_nvdata_manage.h"
#include "ci_log.h"
#include "ci130x_gpio.h"
// #include "all_cmd_statement.h"

///tag-insert-code-pos-1
#include "electric_control.h"
#include "ci_key.h"
#include "buzzer_driver.h"

extern uint8_t g_electric_relay_onoff;
extern electric_relay_status_t g_electric_relay_status;

/**
 * @brief 用户初始化
 *
 */
void userapp_initial(void)
{
    #if CPU_RATE_PRINT
    init_timer3_getresource();
    #endif

    #if MSG_COM_USE_UART_EN
    #if (UART_PROTOCOL_VER == 1)
    uart_communicate_init();
    #elif (UART_PROTOCOL_VER == 2)
    vmup_communicate_init();
    #elif (UART_PROTOCOL_VER == 255)
    UARTInterruptConfig((UART_TypeDef *)UART_PROTOCOL_NUMBER, UART_PROTOCOL_BAUDRATE);
    #endif
    #endif

    #if MSG_USE_I2C_EN
    i2c_communicate_init();
    #endif

    ///tag-gpio-init
    //继电器初始化
   electric_relay_init();
    //上电时候 蜂鸣器 //1S
   buzzer_driver_init();

    ci_key_init();// 测试硬件把IO相关的关闭  在PA3和PA2 都做中断,在key这边使能PA中断 eclic_irq_enable   

   
}

/**
 * @brief 处理按键消息（目前未实现该demo）
 *
 * @param key_msg 按键消息
 */
void userapp_deal_key_msg(sys_msg_key_data_t  *key_msg)
{

  	if (key_msg->key_index != KEY_NULL)
    {
        ci_loginfo(LOG_USER,"key_value is 0x%x ",key_msg->key_index);

		if (MSG_KEY_STATUS_PRESS == key_msg->key_status)
		{
            ci_loginfo(LOG_USER,"status : press down\n");

			if (key_msg->key_index == 0x01)
			{

                if(0 == g_electric_relay_status.onOff)//关机状态
                {
                    //需要过零检测
                    electric_relay_switch_on();
                }
                else//开机状态
                {
                    close_electric_relay_power();
                }
			}

		}
		// else if (MSG_KEY_STATUS_PRESS_LONG == key_msg->key_status)
		// {
		// 	ci_loginfo(LOG_USER,"status : long press\n");
		// }
		// else if (MSG_KEY_STATUS_RELEASE == key_msg->key_status)
		// {
		// 	ci_loginfo(LOG_USER,"status : release\n");
		// }

		if (key_msg->key_status == MSG_KEY_STATUS_RELEASE)
		{
			ci_loginfo(LOG_USER,"按键时间为：%d ms\n",key_msg->key_time_ms);
			if (key_msg->key_index == 0x01)
			{
                //释放按键,如果没有过零点发生,需要还原状态
                if(0 == g_electric_relay_status.onOff)
                {
                    g_electric_relay_onoff=0;
                }
                else
                {
                    g_electric_relay_onoff=1;
                }
			}
		}
    }
}



/**
 * @brief 按语义ID响应asr消息处理
 *
 * @param asr_msg
 * @param cmd_handle
 * @param semantic_id
 * @return uint32_t
 */
uint32_t deal_asr_msg_by_semantic_id(sys_msg_asr_data_t *asr_msg, cmd_handle_t cmd_handle, uint32_t semantic_id)
{
    uint32_t ret = 1;
    if (PRODUCT_GENERAL == get_product_id_from_semantic_id(semantic_id))
    {
        uint8_t vol;
        int select_index = -1;
        switch(get_function_id_from_semantic_id(semantic_id))
        {
        case VOLUME_UP:        //增大音量
            vol = vol_set(vol_get() + 1);
            select_index = (vol == VOLUME_MAX) ? 1:0;
            break;
        case VOLUME_DOWN:      //减小音量
            vol = vol_set(vol_get() - 1);
            select_index = (vol == VOLUME_MIN) ? 1:0;
            break;
        case MAXIMUM_VOLUME:   //最大音量
            vol_set(VOLUME_MAX);
            break;
        case MEDIUM_VOLUME:  //中等音量
            vol_set(VOLUME_MID);
            break;
        case MINIMUM_VOLUME:   //最小音量
            vol_set(VOLUME_MIN);
            break;
        case TURN_ON_VOICE_BROADCAST:    //开启语音播报
            prompt_player_enable(ENABLE);
            break;
        case TURN_OFF_VOICE_BROADCAST:    //关闭语音播报
            prompt_player_enable(DISABLE);
            break;
        default:
            ret = 0;
            break;
        }
        if (ret)
        {
            #if PLAY_OTHER_CMD_EN
            pause_voice_in();
            prompt_play_by_cmd_handle(cmd_handle, select_index, default_play_done_callback,true);
            #endif
        }
    }
    else
    {
        ret = 0;
    }
    return ret;
}


/**
 * @brief 按命令词id响应asr消息处理
 *
 * @param asr_msg
 * @param cmd_handle
 * @param cmd_id
 * @return uint32_t
 */
uint32_t deal_asr_msg_by_cmd_id(sys_msg_asr_data_t *asr_msg, cmd_handle_t cmd_handle, uint16_t cmd_id)
{
    uint32_t ret = 1;
    int select_index = -1;
    switch(cmd_id)
    {
        ///tag-asr-msg-deal-by-cmd-id-start
        case 2://“打开空调”
        {
            break;
        }
        case 3://“关闭空调”
        {
            break;
        }
        case 14://“除湿模式”
        {
            break;
        }
        case 15://"关闭除湿"
        {
            break;
        }
        case 22://"关闭睡眠模式"
        {
            break;
        }
        ///tag-asr-msg-deal-by-cmd-id-end
        default:
            ret = 0;
            break;
    }

    if (ret && select_index >= -1)
    {
        #if PLAY_OTHER_CMD_EN
        pause_voice_in();
        prompt_play_by_cmd_handle(cmd_handle, select_index, default_play_done_callback,true);
        #endif
    }

    return ret;
}

/**
 * @brief 用户自定义消息处理
 *
 * @param msg
 * @return uint32_t
 */
uint32_t deal_userdef_msg(sys_msg_t *msg)
{
    uint32_t ret = 1;
    switch(msg->msg_type)
    {
    /* 按键消息 */
    case SYS_MSG_TYPE_KEY:
    {
        sys_msg_key_data_t *key_rev_data;
        key_rev_data = &msg->msg_data.key_data;
        userapp_deal_key_msg(key_rev_data);
        break;
    }

     // /* 操作继电器电源 */
    case SYS_MSG_TYPE_SET_POWERONOFF:
    {
        if((1 == g_electric_relay_onoff)&&(0==g_electric_relay_status.onOff))//切为开机状态,并且现在是关机状态则执行开机
        {
            gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN5_NUMBER,1);//需要过零检测
            gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN6_NUMBER,0);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_set_output_level_single(PA,ELECTRIC_RELAY_PIN5_NUMBER,0);  
            g_electric_relay_status.onOff = g_electric_relay_onoff;
            electric_relay_status_nv_save();
            mprintf("wkq open power\n");
            cmd_beep();
            
        }
       enable_zero_point_detect_irq();

        break;
    }   

    // /* 继电器状态保存 */
    case SYS_MSG_TYPE_NV_SAVE:
    {
        electric_relay_status_nv_save();
        //cmd_beep();
        break;
    }

    #if MSG_COM_USE_UART_EN
    /* CI串口协议消息 */
    case SYS_MSG_TYPE_COM:
    {
		#if ((UART_PROTOCOL_VER == 1) || (UART_PROTOCOL_VER == 2))
    	sys_msg_com_data_t *com_rev_data;
        com_rev_data = &msg->msg_data.com_data;
        userapp_deal_com_msg(com_rev_data);
        #endif
        break;
    }
    #endif
    /* CI IIC 协议消息 */
    #if MSG_USE_I2C_EN
    case SYS_MSG_TYPE_I2C:
    {
        sys_msg_i2c_data_t *i2c_rev_data;
        i2c_rev_data = &msg->msg_data.i2c_data;
        userapp_deal_i2c_msg(i2c_rev_data);
        break;
    }
    #endif
    default:
        break;
    }
    return ret;
}

