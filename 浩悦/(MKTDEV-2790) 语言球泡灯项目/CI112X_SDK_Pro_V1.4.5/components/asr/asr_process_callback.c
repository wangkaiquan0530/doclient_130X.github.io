/**
  ******************************************************************************
  * @文件    asr_process_callbak.c
  * @版本    V1.0.1
  * @日期    2019-3-15
  * @概要  asr 回调函数，VAD相关
  ******************************************************************************
  * @注意
  *
  * 版权归chipintelli公司所有，未经允许不得使用或修改
  *
  ******************************************************************************
  */
#include <stdlib.h>
#include <stdio.h>

#include "sdk_default_config.h"
#include "platform_config.h"
#include "ci_log_config.h"
#include "asr_process_callback.h"
#include "system_msg_deal.h"
#include "voiceprint_port.h"

#include "ci_log.h"

#define DEBUG_ASR_NOT_PLAY (0)
#define VAD_PCM_MARK_EN (1)


//#include "asr_pcm_buf.h"

#if VAD_PCM_MARK_EN
static char sg_vad_pcm_mark_start_flag = 0;
static char sg_vad_pcm_mark_end_flag = 0;

int clear_vad_start_pcm_mark_flag(void)
{
    sg_vad_pcm_mark_start_flag = 0;
    return 0;
}

int clear_vad_end_pcm_mark_flag(void)
{
    sg_vad_pcm_mark_end_flag = 0;
    return 0;
}

int get_vad_start_pcm_mark_flag(void)
{
    return sg_vad_pcm_mark_start_flag;
}

int get_vad_end_pcm_mark_flag(void)
{
    return sg_vad_pcm_mark_end_flag;
}

#endif

int set_pcm_vad_mark_flag(short *pcm_data,int frame_len)
{
    #if VAD_PCM_MARK_EN
    if(sg_vad_pcm_mark_start_flag)
    {
    	for(int i = 0;i < frame_len;i++)
        {
            pcm_data[i] = 32760;        
        }
        sg_vad_pcm_mark_start_flag = 0;
    }
    
    if(sg_vad_pcm_mark_end_flag)
    {
    	for(int i = 0;i < frame_len;i++)
        {
            pcm_data[i] = -32760;        
        }
        sg_vad_pcm_mark_end_flag = 0;
    }
    #endif

    return 0;
}

#define DEBUG_WRITE_VAD_FILE (0)

#if DEBUG_WRITE_VAD_FILE /*debug used*/
#include "ff.h"

static uint32_t mid_log_flag = 0;
static uint32_t voice_file_index = 1;

static FIL fp_result = NULL;

char voice_file_name[20];

void vad_result_open_new_file(void)
{
    FRESULT res;

    if (NULL != fp_result.fs)
    {
        ci_logerr(LOG_AUDIO_IN, "already %d\n", res);
        vPortEnterCritical();
        while (1)
            ;
    }

    sprintf(voice_file_name, "vad_result_%d.pcm", voice_file_index);
    voice_file_index++;

    ci_logdebug(LOG_AUDIO_IN, "file new %s\n", voice_file_name);

    res = f_open(&fp_result, voice_file_name, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        ci_logerr(LOG_AUDIO_IN, "fw open err %d\n", res);
        vPortEnterCritical();
        while (1)
            ;
    }

    ci_logdebug(LOG_AUDIO_IN, "file new done\n");
}

void vad_result_write_to_file(void *buf, uint32_t size)
{
    uint32_t lr;
    FRESULT res;

    if (NULL == fp_result.fs)
    {
        ci_logerr(LOG_AUDIO_IN, "file error %d\n", res);
        vPortEnterCritical();
        while (1)
            ;
    }

    res = f_write(&fp_result, (void *)buf, size, &lr);
    res = FR_OK;
    lr = size;
    if ((res != FR_OK) || (lr != size))
    {
        ci_logerr(LOG_AUDIO_IN, "f_write err %d\n", res);
    }
}

void vad_result_close_file(void)
{
    if (NULL == fp_result.fs)
    {
        ci_logerr(LOG_AUDIO_IN, "already\n");
        vPortEnterCritical();
        while (1)
            ;
    }

    f_close(&fp_result);
}

#endif

extern void vad_light_on(void);
/*******************************************************************************
    *功能：vad start 函数调用
    *参数：pdata 数据指针  pdata[0]:vadstart-addr,pdata[1]:帧数
    *返回：0
    *注意：none
*******************************************************************************/
int vadstart_callback(unsigned int *pdata, int line)
{
#if VAD_PCM_MARK_EN
    sg_vad_pcm_mark_start_flag = 1;
#endif
    
    vad_light_on();
    return 0;
}

/*******************************************************************************
    *功能：vad process 函数调用
    *参数：pdata 数据指针 pdata[0]:vadcur-addr,pdata[1]:frms
    *返回：0
    *注意：none
*******************************************************************************/
int vadprocess_callback(unsigned int *pdata, int line)
{
    return 0;
}

extern void vad_light_off(void);
/*******************************************************************************
    *功能：vad end 函数调用
    *参数：pdata 数据指针  pdata[0]:vadend-addr
            pdata[1]:vad end结束类型 0 硬件 vad end, 1 超时  vad end, 2 asr
             3,系统(释放asr,暂停asr等)
    *返回：0
    *注意：none
*******************************************************************************/
int vadend_callback(unsigned int *pdata, int line)
{
#if VAD_PCM_MARK_EN
    sg_vad_pcm_mark_end_flag = 1;
#endif
    vad_light_off();
    return 0;
}


int computevad_callback(int asrpcmbuf_addr, int pcm_byte_size, short asrfrmshift, unsigned int asrpcmbuf_start_addr, unsigned int asrpcmbuf_end_addr)
{
    return 0;
}


int send_asr_prediction_msg(uint32_t cmd_handle)
{
    return 0;
}

int send_asr_pre_cancel_msg(void)
{
    return 0;
}

int send_asr_pre_confirm_msg(uint32_t cmd_handle)
{
    return 0;
}

#if !USE_CWSL
void get_cwsl_threshold(unsigned char *wakeup_threshold,unsigned char *cmdword_threshold )
{}
#endif 

int asr_result_callback(callback_asr_result_type_t *asr)
{
    #if USE_CWSL
    extern int deal_cwsl_cmd(char * cmd_word,cmd_handle_t *cmd_handle,short *confidence,int unwakeup_flag,int *cwsl_flag);
    int unwakeup_flag = 0;
    if(SYS_STATE_UNWAKEUP == get_wakeup_state())
    {
        unwakeup_flag = 1;
    }
    int cwsl_flag ;
    int exit_flag = deal_cwsl_cmd(asr->cmd_word,&asr->cmd_handle,&asr->confidence,unwakeup_flag,&cwsl_flag);
    asr->cwsl_flag = cwsl_flag;
    if(exit_flag)
    {
        return 0;
    }
    #endif
    
    mprintf("send result:%s %d\n", asr->cmd_word, asr->confidence);

#if (!DEBUG_ASR_NOT_PLAY)
    if (INVALID_HANDLE != asr->cmd_handle)
    {
        
        sys_msg_t send_msg;
        send_msg.msg_type = SYS_MSG_TYPE_ASR;
        send_msg.msg_data.asr_data.asr_status = MSG_ASR_STATUS_GOOD_RESULT;
        send_msg.msg_data.asr_data.asr_cmd_handle = asr->cmd_handle;
        send_msg.msg_data.asr_data.asr_score = asr->confidence;
        send_msg.msg_data.asr_data.asr_pcm_base_addr = asr->asrvoice_ptr;
        send_msg.msg_data.asr_data.asr_frames = asr->vocie_valid_frame_len;
        send_msg_to_sys_task(&send_msg, NULL);
    }
    else
    {
        mprintf("asr->cmd_handle is INVALID_HANDLE\n");
    }
#endif
    return 0;
}


int asr_lite_result_callback(char * words,short cfd)
{
    mprintf("out2 :%s %d\n",words,cfd);
    /*add code for app*/
    
    return 0;
}

#if !USE_CWSL
void cwsl_fe_compute_port(unsigned int fe_buf_start_addr, unsigned int data_num, unsigned int slice_nums, unsigned int final_flag){}
int cwsl_get_vad_alc_config(void){return 0;}
#endif

/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
