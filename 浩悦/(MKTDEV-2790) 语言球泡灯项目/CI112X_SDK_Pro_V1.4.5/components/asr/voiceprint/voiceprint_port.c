#include "voiceprint_port.h"
#include "sdk_default_config.h"
#include "ci_log.h"
#include "ci112x_system.h"
#include "task.h"
#include "flash_rw_process.h"

#if (1 == USE_VP)
int16_t g_fbank_outbuf_num_add_by_vp = 40*3;
#else
int16_t g_fbank_outbuf_num_add_by_vp = 0;
#endif

//声纹取阈值如果总是大于这个值，强行等于这个值，免得匹配效果差。
float g_vp_threshold_must_down_value = -90.0f;
//计算阈值的时候，要是阈值低于某个值，就不承认这段语音的有效性，匹配失败
float g_vp_cas_dis_threshold_min_value = -150.0f;
float g_vp_get_threshold_tolerate = 20.0f;

int16_t g_vp_mould_voice_num  = 3;//得到VP的模板需要多少段语音
int16_t g_vp_mould_max_num = VP_TEMPLATE_NUM;//声纹模板最多有多少个

int16_t g_vp_match_dnn_cmpt_min_time = 1;//最少需要这个次数的完整的VP DNN运算
int16_t g_vp_match_dnn_cmpt_max_time = 4;//5//最多计算3次DNN运算，就给出距离结果
//取阈值需要的语音段数
int16_t g_vp_get_threshold_time = 2;


/**
 * @brief 将ASR识别到的结果发送到声纹
 * 
 * @param handle 识别到的命令词句柄
 */
extern void send_asr_result_to_vp(cmd_handle_t handle,int16_t start_frm,int16_t valid_frm);
void send_asr_result_to_vp_port(cmd_handle_t handle,int16_t start_frm,int16_t valid_frm)
{
    #if USE_VP
    send_asr_result_to_vp(handle,start_frm,valid_frm);
    #endif
}


/**
 * @brief vp获取ASR识别到的结果
 * 
 * @return cmd_handle_t 
 */
extern cmd_handle_t get_asr_result_to_vp(void);
cmd_handle_t get_asr_result_to_vp_port(void)
{
    #if USE_VP
    cmd_handle_t handle = get_asr_result_to_vp();
    return handle;
    #else
    return (cmd_handle_t)NULL;
    #endif
}


extern void send_vp_registe_msg(void);
/**
 * @brief 发送声纹开始注册的消息
 * 
 */
void send_vp_registe_msg_port(void)
{
    #if USE_VP
    send_vp_registe_msg();
    #endif
}


extern void send_vp_delete_msg(void);
void send_vp_delete_msg_port(void)
{
    #if USE_VP
    send_vp_delete_msg();
    #endif
}


extern vp_dnn_cmpt_flag_t vp_dnn_cmpt_one_time(uint16_t remain_fe,vp_dnn_cmpt_flag_t is_dnn_vp_vad_end,uint32_t addr);
vp_dnn_cmpt_flag_t vp_dnn_cmpt_one_time_port(uint16_t remain_fe,vp_dnn_cmpt_flag_t is_dnn_vp_vad_end,uint32_t addr)
{
    #if USE_VP
    vp_dnn_cmpt_flag_t rslt = VP_DNN_CMPT_FLAG_NULL;
    rslt = vp_dnn_cmpt_one_time(remain_fe,is_dnn_vp_vad_end,addr);
    return rslt;
    #else
    return VP_DNN_CMPT_FLAG_NULL;
    #endif
}


extern void vp_manage_init(void);
void vp_manage_init_port(void)
{
    #if USE_VP
    vp_manage_init();
    #else
    return;
    #endif
}


extern void voiceprint_manage_task(void* p );
void vp_task_creat_port(void)
{
    #if USE_VP
    xTaskCreate(voiceprint_manage_task,"voiceprint_manage_task",256,NULL,4,NULL);
    #else
    return;
    #endif
}


extern void vp_deal_with_end(void);
void vp_deal_with_end_port(void)
{
    #if USE_VP
    vp_deal_with_end();
    #else
    return;
    #endif
}


vp_dnn_cmpt_flag_t send_vp_dnn_done_to_vp_manage(uint32_t addr,uint32_t slice,uint16_t cmpt_time,
                                                vp_dnn_cmpt_flag_t is_end,uint16_t remain_fe);
vp_dnn_cmpt_flag_t send_vp_dnn_done_to_vp_manage_port(uint32_t addr,uint32_t slice,uint16_t cmpt_time,
                                                      vp_dnn_cmpt_flag_t is_end,uint16_t remain_fe)
{
    vp_dnn_cmpt_flag_t rslt = VP_DNN_CMPT_FLAG_NULL;
    #if USE_VP
    rslt = send_vp_dnn_done_to_vp_manage(addr,slice,cmpt_time,is_end,remain_fe);
    return rslt;
    #else
    return rslt;
    #endif
}


extern BaseType_t send_vp_dnn_mtx_done_msg_isr(uint32_t ptr,uint32_t slice_num);
BaseType_t send_vp_dnn_mtx_done_msg_isr_port(uint32_t ptr,uint32_t slice_num)
{
    BaseType_t rslt = pdPASS;
    #if USE_VP
    rslt = send_vp_dnn_mtx_done_msg_isr(ptr,slice_num);
    #else
    ci_logerr(CI_LOG_ERROR,"DNN mode err !\r\n");
    #endif
    return rslt;
}


//extern int32_t vp_req_dnn_compute(uint32_t fe_buf_start_addr,uint32_t fe_buf_size,int32_t fe_out_frms,int line);
int32_t vp_req_dnn_compute_port(uint32_t fe_buf_start_addr,uint32_t fe_buf_size,int32_t fe_out_frms,int line)
{
    #if 1
    #if USE_VP
    vp_req_dnn_compute(fe_buf_start_addr,fe_buf_size,fe_out_frms,line);
    return 0;
    #else
    return 0;
    #endif
    #endif
}


extern void vp_compute_clear(void);
void vp_compute_clear_port(void)
{
    #if USE_VP
    vp_compute_clear();
    #else
    return;
    #endif
}


extern void dnn_switch_vp_init(void);
void dnn_switch_vp_init_port(void)
{
    #if USE_VP
    dnn_switch_vp_init();
    #else
    return;
    #endif
}
