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
    gas_alarm_communicate_init();
    vmup_communicate_init();

    #endif

    #if MSG_USE_I2C_EN
    i2c_communicate_init();
    #endif

    ///tag-gpio-init
}

/**
 * @brief 处理按键消息（目前未实现该demo）
 *
 * @param key_msg 按键消息
 */
void userapp_deal_key_msg(sys_msg_key_data_t  *key_msg)
{
    (void)(key_msg);
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
        // case TURN_ON_VOICE_BROADCAST:    //开启语音播报
        //     prompt_player_enable(ENABLE);
        //     break;
        // case TURN_OFF_VOICE_BROADCAST:    //关闭语音播报
        //     prompt_player_enable(DISABLE);
        //     break;
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
uint32_t deal_asr_msg_by_cmd_id(sys_msg_asr_data_t *asr_msg, cmd_handle_t cmd_handle, uint16_t cmd_id,uint32_t semantic_id)
{
    uint32_t ret = 1;
    int select_index = -1;
    uint16_t vol = 0;
    ci_loginfo(LOG_USER," semantic_id %x \r\n",semantic_id);
    if((semantic_id == AWAKEN_INDEX_CMD)||(semantic_id >=INCREASE_VOICE_INDEX_CMD && semantic_id <= MIN_VOICE_INDEX_CMD))//唤醒词
    {
    	 switch(semantic_id)
         {
             case INCREASE_VOICE_INDEX_CMD:     //增大音量
                vol = vol_set(vol_get() + 1);
                select_index = (vol == VOLUME_MAX) ? 1:0;
                break;
            case DECREASE_VOCE_INDEX_CMD:      //减小音量
                vol = vol_set(vol_get() - 1);
                select_index = (vol == VOLUME_MIN) ? 1:0;
                break;
            case MAX_VOICE_INDEX_CMD:          //最大音量
                vol_set(VOLUME_MAX);
                break;
            case MIN_VOICE_INDEX_CMD:         //最小音量
                vol_set(VOLUME_MIN);
                break;
           ///tag-asr-msg-deal-by-cmd-id-end
           default:
                ret = 0;
                break;
         }
		//lme add
        lme_asr_cmd_com_send(UART_HEADER_NON, cmd_id,semantic_id);
		
		if (ret && select_index >= -1)
		{
			#if PLAY_OTHER_CMD_EN
			pause_voice_in();
			ci_loginfo(LOG_USER," prompt_play_by_cmd_handle \r\n");
			prompt_play_by_cmd_handle(cmd_handle, select_index, default_play_done_callback,true);
			#endif
		 }

    }
    else
    {
	  userapp_set_timeout_function_index(INVALID_FUNCTION_INDEX);
	  userapp_set_current_function_index(cmd_id,semantic_id);
      lme_asr_cmd_com_send(UART_HEADER_CON, cmd_id,semantic_id);
	  enter_uart_resend_timer(UART_RESEND_TIME);  
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
    #if MSG_COM_USE_UART_EN
    /* CI串口协议消息 */
    case SYS_MSG_TYPE_COM:
    {
    	sys_com_msg_data_t *com_rev_data;
        //com_rev_data = &msg->msg_data.com_data;
		com_rev_data = &msg->msg_data.lme_com_data;
        ci_loginfo(LOG_USER," userapp_deal_com_msg \r\n");
        userapp_deal_com_msg(com_rev_data);
        break;
    }

    case SYS_MSG_TYPE_GAS_ALARM_COM:
    {
    	sys_gas_alarm_com_data_t *gas_alarm_com_rev_data;
        //com_rev_data = &msg->msg_data.com_data;
		gas_alarm_com_rev_data = &msg->msg_data.gas_alarm_com_data;
        ci_loginfo(LOG_USER," userapp_gas_alarm_deal_com_msg \r\n");
        userapp_gas_alarm_deal_com_msg(gas_alarm_com_rev_data);
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

