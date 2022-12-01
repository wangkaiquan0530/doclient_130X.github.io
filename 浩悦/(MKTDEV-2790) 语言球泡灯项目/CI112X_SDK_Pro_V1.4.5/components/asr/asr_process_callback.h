/**
  ******************************************************************************
  * @文件    asr_process_callbak.h
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

#ifndef _ASR_PROCESS_CALLBACK_H_
#define _ASR_PROCESS_CALLBACK_H_


#ifdef __cplusplus
extern "C" {
#endif
  
#include "command_info.h"
  
typedef struct
{
    char* cmd_word;
    cmd_handle_t cmd_handle;
    unsigned int asrvoice_ptr;
    short confidence;
    short vocie_valid_frame_len;
    short voice_start_frame;
    short start_word_confidence;
    short end_word_confidence;
    short lowest_confidence;
    char cwsl_flag;
}callback_asr_result_type_t;


int vadstart_callback(unsigned int* pdata ,int line);
int vadprocess_callback(unsigned int* pdata ,int line);
int vadend_callback(unsigned int* pdata,int line);
int computevad_callback(int asrpcmbuf_addr,int pcm_byte_size, short asrfrmshift,unsigned int asrpcmbuf_start_addr,unsigned int asrpcmbuf_end_addr);
int asr_result_callback(callback_asr_result_type_t * asr);

int set_pcm_vad_mark_flag(short *pcm_data,int frame_len);



#ifdef __cplusplus
}
#endif


#endif

/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
