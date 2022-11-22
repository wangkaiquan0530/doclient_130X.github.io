#ifndef __CWSL_APP_SAMPLE1_H__
#define __CWSL_APP_SAMPLE1_H__

#include "system_msg_deal.h"
#include "cwsl_manage.h"


typedef enum
{
    CWSL_REGISTRATION_WAKE          = 200,      ///< 命令词：学习唤醒词
    CWSL_REGISTRATION_CMD           = 201,      ///< 命令词：学习命令词
    CWSL_REGISTER_AGAIN             = 202,      ///< 命令词：重新学习
    CWSL_EXIT_REGISTRATION          = 203,      ///< 命令词：退出学习
    CWSL_DELETE_FUNC                = 204,      ///< 命令词：我要删除
    CWSL_DELETE_WAKE                = 205,      ///< 命令词：删除唤醒词
    CWSL_DELETE_CMD                 = 206,      ///< 命令词：删除命令词
    CWSL_EXIT_DELETE                = 207,      ///< 命令词：退出删除模式
    CWSL_DELETE_ALL                 = 208,      ///< 命令词：全部删除

    CWSL_DATA_ENTERY_SUCCESSFUL     = 209,      ///< 播报：学习成功
    CWSL_DATA_ENTERY_FAILED         = 210,      ///< 播报：学习失败
    CWSL_REGISTRATION_SUCCESSFUL    = 211,      ///< 播报：学习成功
    CWSL_TEMPLATE_FULL              = 212,      ///< 播报：学习模板超过上限
    CWSL_SPEAK_AGAIN                = 1007,     ///< 播报：请再说一次>
    CWSL_TOO_SHORT                  = 1008,     ///< 播报：语音长度不够，请再说一次>

    CWSL_DELETE_SUCCESSFUL          = 213,      ///< 播报：删除成功
    CWSL_DELETE_FAILED              = 214,      ///< 播报：删除失败
    CWSL_DELETING                   = 215,      ///< 播报：删除中
    CWSL_DONT_FIND_WORD             = 216,      ///< 播报：找不到删除的词
    CWSL_REGISTRATION_ALL           = 217,      ///< 播报：学习完成
    CWSL_REG_FAILED                 = 218,      ///< 播报：学习失败

    CWSL_OPEN_COMMAND               =2000,      ///< 播报：请打说学习打开指令
    CWSL_CLOSE_COMMAND              =2001,      ///< 播报：请打说学习关闭指令
    CWSL_DELETE_OPEN_COMMAND        =2002,      ///< 删除打开指令
    CWSL_DELETE_CLOSE_COMMAND       =2003,      ///< 删除关闭指令


}cicwsl_func_index;


////cwsl process ASR message///////////////////////////////////////////////
/**
 * @brief 命令词自学习消息处理函数
 * 
 * @param asr_msg ASR识别结果消息
 * @param cmd_handle 命令词handle
 * @param cmd_id 命令词ID
 * @retval 1 数据有效,消息已处理
 * @retval 0 数据无效,消息未处理
 */
uint32_t cwsl_app_process_asr_msg(sys_msg_asr_data_t *asr_msg, cmd_handle_t *cmd_handle, uint16_t cmd_id);

// cwsl_manage模块复位，用于系统退出唤醒状态时调用
int cwsl_app_reset();

#endif  // __CWSL_APP_SAMPLE1_H__
