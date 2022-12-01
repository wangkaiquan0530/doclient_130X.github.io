/**
  ******************************************************************************
  * @file    alg_preprocess.c
  * @version V1.0.0
  * @date    2019.04.04
  * @brief 
  ******************************************************************************
  */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "ci_audio_wrapfft.h"
#include "../alg/denoise/ci_denoise.h"
#include "sdk_default_config.h"
//#include "ff.h"
//#include "ci_denoise.h"
//#include "ci_adapt_aec.h"
//#include "alg/tdoa/ci_tdoa.h"
//#include "alg/beamforming/ci_bf.h"
#include "system_msg_deal.h"
#include "asr_process_callback.h"
#include "ci112x_audio_pre_rslt_out.h"
#include "ci112x_iis1_out_debug.h"
//#include "arm_biquad_cascade_iir_hpf.h"
#include "asr_api.h"
#if UPLOAD_VOICE_DATA_EN
#include "ci_estvad_compute.h"
#endif
//#include "ci_dpcm_agc.h"

#include "ci112x_codec.h"
#include "ci112x_audio_capture.h"
#include "user_config.h"
#include "debug_time_consuming.h"
#include "ci_fft_ifft_test.h"

#include "../alg/alc_auto_switch/alc_auto_switch.h"
#include "../alg/ci_alg_malloc.h"
/**************************************************************************
                    macro
****************************************************************************/
#define DEBUG_ALG_TIME          (0)
#define USE_IIR_HPF             (0)
#define USE_WS2812_OUTPUT_TDOA  (0)

//使能重采样的宏控制
#if 0//INNER_CODEC_AUDIO_IN_USE_RESAMPLE
#define LP_TMP_BUF_NUM (2U)//32K采样率下，2帧256点的PCM，处理之后变成1帧16K采样率下256点的PCM
#define PCM_FRAME_LENGTH (256U)
static int16_t resample_rslt_tmp_l[PCM_FRAME_LENGTH*LP_TMP_BUF_NUM] = {0};
    #if (!AUDIO_CAPTURE_USE_SINGLE_CHANNEL && !USE_OUTSIDE_IIS_TO_ASR)
    static int16_t resample_rslt_tmp_r[PCM_FRAME_LENGTH*LP_TMP_BUF_NUM] = {0};
    #endif

static bool sg_is_inner_codec_resample = false;//记录当前是否为resample状态
static bool sg_is_inner_codec_resample_pre_state = false;
#endif
#if USE_WS2812_OUTPUT_TDOA
#include "ws2812.h"
#endif

#if DEBUG_ALG_TIME
#    include "example/alg_example/ci_timer.h"
#    include "assist/debug_time_consuming.h"
#    define SHOW_TIMER_INFO() do { \
                                    static int frame_cnt = 0; \
                                    frame_cnt++; \
                                    if (frame_cnt == 1000) \
                                    { \
                                        frame_cnt = 0; \
                                        ci_timer_show_result(); \
                                    } \
                                 } while(0)           
#else
#   define ci_timer_start(a)
#   define ci_timer_stop(a)
#   define SHOW_TIMER_INFO() 
#endif

#if USE_SINGLE_MIC_AEC
#   if USE_TWO_MIC_AEC
#       error "!!!!!!!!!!!!!!!!!!!!!!!!! error config,  not supprot"
#   endif
#endif

#if (!AUDIO_CAPTURE_USE_MULTI_CODEC)
#   if USE_SINGLE_MIC_AEC
#       if (USE_TWO_MIC_TDOA+USE_TWO_MIC_BEAMFORMING)
#       error "!!!!!!!!!!!!!!!!!!!!!!!!! no no, not support, check the user_config.h please"
#       endif
#   endif

#   if USE_TWO_MIC_AEC
#   error "!!!!!!!!!!!!!!!!!!!!!!!!! no, not support, no ref in for 2mic aec, check the user_config.h please"  
#   endif
#endif

extern int get_vad_state(float *fft_data,float *);
extern int apply_asr_data_deal(int vad_state,float *psd,short *pcm_data);

extern int set_pcm_vad_mark_flag(short *pcm_data,int frame_len);

extern fft_config_t fft_1024;
extern fft_config_t fft_512_for_asr;

/**************************************************************************
                    global
****************************************************************************/

//static void *adapt_aec = NULL;
//static void *iir_hpf_l = NULL;
//static void *iir_hpf_r = NULL;
void *denoise   = NULL;
void *alc_auto  = NULL;


ci_fft_ifft_creat_t* fft_ifft_resample_l;//左通道resample结构体
ci_fft_ifft_creat_t* fft_ifft_resample_r;//左通道resample结构体

ci_wrapfft_audio *wrapfft_audio_t = NULL;
ci_wrapfft_audio * wrapfft_audio_t_asr = NULL;

/**************************************************************************
                    function
****************************************************************************/

static int alg_warpfft_deal_model(void)
{
    int mic_channel_num = 0;
    int ref_channel_num = 0;

    mic_channel_num = 1;

    wrapfft_audio_t =  ci_warpfft_create( mic_channel_num,ref_channel_num,&fft_1024);
    
    if (!wrapfft_audio_t ) 
    {
        return -1;
    }
#if SEND_POSTALG_VOICE_TO_ASR
    wrapfft_audio_t_asr =  ci_warpfft_create( mic_channel_num,ref_channel_num,&fft_512_for_asr);

    if (!wrapfft_audio_t_asr )
    {
        return -1;
    }

	extern void config_use512fft_forasr(void);
	config_use512fft_forasr();
#endif

    return 0;   
}

static int alg_denoise_init( void )
{
    int ret = 0;
    //int size1 =0,size2 = 0;
    //size1 = get_alg_malloc_size()+get_alg_calloc_size();

	denoise = (void*)ci_denoise_create(0,8000);

	//size2 = get_alg_malloc_size()+get_alg_calloc_size() - size1;
    //mprintf("version = %d,size = %d\n",ci_denoise_version(),size2);

    if (!denoise)
    {
        ci_logerr(LOG_AUDIO_IN,"%s() failed\n", __FUNCTION__ );
        return -1;
    }

    ret = ci_denoise_set_threshold(denoise, SET_DENOISE_THRESHOLD, SET_DENOISE_THRESHOLD_WINDOW_SIZE );
    if (ret != 0)
    {
        ci_logerr(LOG_AUDIO_IN,"%s() failed\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

void alg_init( void )
{
    int ret = 0;

	#if 1
    ret =  alg_warpfft_deal_model();  
    #endif
	
    #if USE_SINGLE_MIC_DENOISE
    ret += alg_denoise_init();
    #endif

    #if USE_IIR_HPF
    ret += iir_hpf_init();    
    #endif

#if USE_ALC_AUTO_SWITCH
    alc_auto = (void*)alc_auto_switch_create();
    ban_alc_function(alc_auto,1);  
#endif

    if (ret != 0)
    {
        ci_logerr(LOG_AUDIO_IN," alg_init %d failed\n",__LINE__ );
        while( 1 );
    }
    
    #if DEBUG_ALG_TIME
    init_timer0();
    timer0_start_count();
    ret = ci_timer_init( 1, "AEC"  );
    if (ret != 0)
    {
        ci_logerr(LOG_AUDIO_IN,"%s().%d: ci_timer_init() failed, exit\n", __FUNCTION__, __LINE__ );
        while( 1 );
    }

    ret = ci_timer_init( 2, "den"  );
    if (ret != 0)
    {
        ci_logerr(LOG_AUDIO_IN,"%s().%d: ci_timer_init() failed, exit\n", __FUNCTION__, __LINE__ );
        while( 1 );
    }
    #endif

    #if (USE_IIS1_OUT_AUDIO||DEBUG_FE_PCM)
    audio_pre_rslt_out_init_t init_str;
    memset(&(init_str),0,sizeof(audio_pre_rslt_out_init_t));
    init_str.mode = PRE_RSLT_MASTER;//PRE_RSLT_SLAVE;//PRE_RSLT_MASTER;
    init_str.data_format = AUDIO_PLAY_CARD_DATA_FORMAT_STANDARD_I2S;
    init_str.block_size = 320*2;
    init_str.data_comp = PRE_RSLT_DATA_LEFT_ORIGIN_RIGHT_RSLT;
    init_str.sck_lrck_rate = AUDIO_PLAY_SCK_LRCK_64;//SCK和LRCK的比值为64
    audio_pre_rslt_out_play_card_init(&init_str);
    #endif
}


/* @brief 双通道使用重采样的左通道
 * 
 * return 0:降采样处理的时候，需要两帧拼成一帧，但是如果是第一帧的处理，返回0；
 *          其余情况返回1.
 */
static int8_t fft_resample_procese_l(int16_t* src,int16_t* dst,int16_t frame_length)
{
    int8_t ret_flag = 0;
    #if 0//INNER_CODEC_AUDIO_IN_USE_RESAMPLE
    if(true == sg_is_inner_codec_resample)
    {
        static uint8_t count_flag = 0; 
        uint32_t fir_rsl_tmp_addr_l = (uint32_t)resample_rslt_tmp_l + frame_length*sizeof(int16_t)*count_flag;
        //init_timer0();
        //timer0_start_count();
        #if 1
        ci_fft_ifft_test_deal( fft_ifft_resample_l,src, NULL, (short*)fir_rsl_tmp_addr_l, NULL );//置进行fft_ifft
        #else
        memcpy((void*)fir_rsl_tmp_addr_l,src,frame_length*sizeof(int16_t));
        #endif
        //int16_t* data = (int16_t*)src;
        //mprintf("%d\n",data[3]);
        //timer0_end_count("end");
        
        if(count_flag%2 != 0)
        {
            int16_t* dst_tmp_p = (int16_t*)(fir_rsl_tmp_addr_l - frame_length*sizeof(int16_t));
            for(int i=0;i<frame_length;i++)//抽点，从512个32k采样率的点里面抽256个出来
            {
                dst[i] = dst_tmp_p[i*2];
            }

            ret_flag = 1;
        }

        count_flag++;
        count_flag %= LP_TMP_BUF_NUM;
    }
    else
    {
        ret_flag = 1;
    }
    #else
    ret_flag = 1;
    #endif

    return ret_flag;
}

void inner_codec_left_alc_enable_port()
{
    inner_codec_left_alc_enable(INNER_CODEC_GATE_ENABLE,INNER_CODEC_USE_ALC_CONTROL_PGA_GAIN);
}

void inner_codec_alc_disable_port()
{
    inner_codec_alc_disable(1,15);
}

int get_vad_idle_flag_port()
{
	extern int is_vad_idle();
	return is_vad_idle();
}

void reset_threshold_for_alc_switch()
{
    static int state = 1;
    static int pre_state = 1;
    state = get_alc_state();

    if(state != pre_state)
    {
      if(0 == state)
      {
#if USE_SINGLE_MIC_DENOISE
    	  ci_denoise_set_threshold(denoise, 0, SET_DENOISE_THRESHOLD_WINDOW_SIZE);
#endif
      }
      else if(1 == state)
      {
#if USE_SINGLE_MIC_DENOISE
    	  ci_denoise_set_threshold(denoise, SET_DENOISE_THRESHOLD, SET_DENOISE_THRESHOLD_WINDOW_SIZE);
#endif
      }
    }

    pre_state = state;
}

#if (USE_IIS1_OUT_AUDIO || SEND_POSTALG_VOICE_TO_ASR  )
short dst_temp[320] = {0};
#endif

static int sg_pcm_frames_count = 0;
#if AUDIO_CAPTURE_USE_SINGLE_CHANNEL
int alg_preprocess_single_ch( short *mic_data, short *dst_data, int frame_length )
{
    short *src  = mic_data;
    short *micl = src;
    int ret = 0;
    int ret_flag = 0;
    
    /*suppress compiler warning of unused parameter*/
    (void)src;
    (void)micl;
    (void)ret;

    for(int i=0;i<frame_length;i++)
    {
    	micl[i] = src[i*2];
		//dst_temp[i] = src[i*2 + 1];
    }
    
    ret_flag = 0;

	wrapfft_audio_t->mic[0] = micl;
	ret =  ci_fft_deal(wrapfft_audio_t);

	//前端处理流程
#if USE_ALC_AUTO_SWITCH
    if( !get_mute_voice_in_state() && !check_current_playing())
    {
        switch_alc_state_automatic(alc_auto,micl,frame_length) ;
    }
    reset_threshold_for_alc_switch();
#endif

#if USE_SINGLE_MIC_DENOISE
	if( !get_mute_voice_in_state() )
	{
		ret += ci_denoise_deal( denoise, wrapfft_audio_t->fft_mic_out[0], wrapfft_audio_t->fft_mic_out[0] );//fft_输入_ifft_输出
	}
#endif

    //输出前端处理的语音 需要开启USE_IFFT 或 当SEND_POSTALG_VOICE_TO_ASR为1时默认开启USE_IFFT
#if ((USE_IFFT&&USE_IIS1_OUT_AUDIO) || ((0 == USE_IIS1_OUT_AUDIO) && (SEND_POSTALG_VOICE_TO_ASR)))
	ci_wrapfft_ifft( wrapfft_audio_t,  wrapfft_audio_t->fft_mic_out[0],dst_temp);
	for(int i = 0;i<frame_length/2;i++)
	{
		dst_temp[i] = dst_temp[2*i];
	}
#endif

	int state = 0;
	sg_pcm_frames_count ++;
	if(3 == sg_pcm_frames_count)
	{
		state = get_vad_state(wrapfft_audio_t->fft_mic_out[0],wrapfft_audio_t->psd);
		sg_pcm_frames_count = 0;
	}

	//识别流程
#if SEND_POSTALG_VOICE_TO_ASR
	wrapfft_audio_t_asr->mic[0] = dst_temp;
	ret =  ci_fft_deal(wrapfft_audio_t_asr);
	apply_asr_data_deal(state,wrapfft_audio_t_asr->psd,micl);
#else
	apply_asr_data_deal(state,wrapfft_audio_t->psd,micl);
#endif
	
    //IIS输出
#if USE_IIS1_OUT_AUDIO
	for(int i = 0;i<frame_length/2;i++)
	{
		micl[i] = micl[2*i];
	}
	#if !USE_IFFT
	memset((void*)dst_temp,0,sizeof(int16_t)*frame_length);
	set_pcm_vad_mark_flag(dst_temp,frame_length/2);
	audio_pre_rslt_write_data(  (int16_t*)micl/*Right*/, (int16_t*)dst_temp );
	#else
	set_pcm_vad_mark_flag(micl,frame_length/2);
	audio_pre_rslt_write_data(  (int16_t*)dst_temp/*Right*/, (int16_t*)micl );
	#endif
#endif


	return 1;
}
#endif



