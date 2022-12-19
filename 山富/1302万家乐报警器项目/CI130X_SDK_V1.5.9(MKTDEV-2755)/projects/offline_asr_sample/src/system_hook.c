#include <stdlib.h>

#include "command_info.h"
#include "voice_module_uart_protocol.h"
#include "i2c_protocol_module.h"
#include "ci130x_core_misc.h"
#include "baudrate_calibrate.h"
#include "ci_log.h"

/**
 * @brief 系统启动事件钩子，系统启动时会调用此函数
 * 
 */
__WEAK void sys_power_on_hook(void)
{

}

/**
 * @brief 系统唤醒事件钩子，系统进入唤醒状态时会调用此函数
 * 
 */
__WEAK void sys_weakup_hook(void)
{
    #if MSG_COM_USE_UART_EN
    #if UART_BAUDRATE_CALIBRATE
    baudrate_calibrate_stop();
    #endif
    #endif
}

/**
 * @brief 系统退出唤醒事件钩子，系统退出唤醒状态时会调用此函数
 * 
 */
__WEAK void sys_sleep_hook(void)
{
    #if MSG_COM_USE_UART_EN
    #if UART_BAUDRATE_CALIBRATE
    baudrate_calibrate_stop();
    #endif
    #endif
    
    #if MSG_USE_I2C_EN
    i2c_sleep_deal();
    #endif
}

/**
 * @brief 语音识别事件钩子，语音模块输出识别结果时会调用此函数
 * 
 */
__WEAK void sys_asr_result_hook(cmd_handle_t cmd_handle, uint8_t asr_score)
{
    #if MSG_COM_USE_UART_EN
    #if UART_BAUDRATE_CALIBRATE
    baudrate_calibrate_stop();
    #endif
    #endif
    
    #if MSG_USE_I2C_EN
    i2c_product_send_msg(cmd_handle);
    #endif
}


