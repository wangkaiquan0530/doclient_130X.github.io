#ifndef __VOICEPRINT_PORT_H
#define __VOICEPRINT_PORT_H

#include "stdint.h"
#include "FreeRTOS.h"
#include "command_info.h"
#include "ci_nvdata_manage.h"
#include "flash_rw_process.h"


#ifdef __cplusplus
extern "C" {
#endif

extern int16_t g_vp_mould_voice_num;
extern int16_t g_vp_match_dnn_cmpt_min_time;//最少需要这个次数的完整的VP DNN运算
extern int16_t g_vp_match_dnn_cmpt_max_time;
extern int16_t g_vp_mould_max_num;//声纹模板最多有多少个
extern int16_t g_vp_get_threshold_time;
extern int16_t g_fbank_outbuf_num_add_by_vp;

typedef enum
{
    VP_DNN_CMPT_FLAG_NULL = 0,
    VP_DNN_CMPT_FLAG_NOT_END,
    VP_DNN_CMPT_FLAG_IS_END,
    VP_DNN_CMPT_FLAG_TOO_SHORT,//一笔DNN运算，最后一笔的FE不足1个slice
}vp_dnn_cmpt_flag_t;


vp_dnn_cmpt_flag_t vp_dnn_cmpt_one_time_port(uint16_t remain_fe,vp_dnn_cmpt_flag_t is_dnn_vp_vad_end,uint32_t addr);
void vp_manage_init_port(void);
void vp_task_creat_port(void);
void vp_deal_with_end_port(void);
vp_dnn_cmpt_flag_t send_vp_dnn_done_to_vp_manage_port(uint32_t addr,uint32_t slice,uint16_t cmpt_time,
                                                      vp_dnn_cmpt_flag_t is_end,uint16_t remain_fe);
BaseType_t send_vp_dnn_mtx_done_msg_isr_port(uint32_t ptr,uint32_t slice_num);
int32_t vp_req_dnn_compute_port(uint32_t fe_buf_start_addr,uint32_t fe_buf_size,int32_t fe_out_frms,int line);
void vp_compute_clear_port(void);
void dnn_switch_vp_init_port(void);
void send_asr_result_to_vp_port(cmd_handle_t handle,int16_t start_frm,int16_t valid_frm);
cmd_handle_t get_asr_result_to_vp_port(void);
void send_vp_delete_msg_port(void);
void send_vp_registe_msg_port(void);
uint32_t get_vp_template_size(void);

/**
 * @brief 发送打开vp开关的消息
 * 
 */
extern void send_vp_open_msg(void);

/**
 * @brief 发送关闭vp开关的消息
 * 
 */
extern void send_vp_close_msg(void);

/**
 * @brief 打开vp注册功能
 */
extern void send_vp_registe_msg(void);

/**
 * @brief 退出vp注册功能
 */
extern void send_vp_exit_registe_msg(void);

/**
 * @brief ASR告诉声纹,识别结果是注册时使用的词
 * 
 */
extern void asr_tell_vp_cmd_is_registe_word(void);


#endif //__VOICEPRINT_MANAGE_H



