/**
  ******************************************************************************
  * @file    voice_in_manage.c
  * @version V1.0.0
  * @date    2019.04.04
  * @brief 
  ******************************************************************************
  */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sdk_default_config.h"
#include "debug_time_consuming.h"

#include "asr_api.h"
#include "ci_task_monitor.h"
#include "ci112x_iisdma.h"
#include "ci_log.h"

#include <stdbool.h>

#include "ci112x_audio_capture.h"
#include "es8388.h"
#include "audio_in_manage.h"

#include "alg_preprocess.h"

#include "ci112x_audio_pre_rslt_out.h"
#include "ci112x_iis1_out_debug.h"

#include "system_msg_deal.h"

#include "ci112x_codec.h"
//#include "ci_task_monitor.h"
#if UPLOAD_VOICE_DATA_EN
#include "voice_upload_manage.h"
#endif

#if USE_CODE_ES7243
#include "es7243.h"
#endif

#if (AUDIO_CAPTURE_USE_SINGLE_CHANNEL)
    #if (USE_SINGLE_MIC_AEC)||(USE_TWO_MIC_AEC)||(USE_TWO_MIC_TDOA)||(USE_TWO_MIC_BEAMFORMING)
    #error "config error!"
    #endif
#endif


/**************************************************************************
                    typedef
****************************************************************************/
#define ASRPCM_PER_FRAME_SHIFT_SIZE (160)/*10ms frame shift*/
#if UPLOAD_VOICE_DATA_EN
#define ASRPCM_TOTAL_FRAME    (80 )/*160 for 16ms 10ms ok  for upload wakeup world to cloud*/
#else
#define ASRPCM_TOTAL_FRAME    (80 )/*80 for 16ms 10ms ok*/
#endif

#define ALG_FRAME_SIZE (320)/*16ms perframe*/

/*debug used for alg output save to sd card*/
#define ALG_DEBUG_SAVE_RESULT_FRAMS 0//(1000)//(500)


/**************************************************************************
                    global
****************************************************************************/


/**************************************************************************
                    function
****************************************************************************/

#if ALG_DEBUG_SAVE_RESULT_FRAMS
#define TEST_RESULT_FILENAME "0:/result_01.pcm"
static FIL fp_result = NULL;
static void debug_result_write_to_file( void *buf, uint32_t size )
{
    static uint32_t cnt = 0;
    uint32_t lr;
    FRESULT res;
    
    if(0 == cnt)
    {
        res = f_open(&fp_result, TEST_RESULT_FILENAME, FA_WRITE|FA_CREATE_ALWAYS );
        if(res != FR_OK)
        {
            ci_logerr(LOG_AUDIO_IN,"fw open err %d\n",res);
            vPortEnterCritical();
            while(1);
        }
        ci_loginfo(LOG_AUDIO_IN,"open debug result file\n");
        cnt++;
    }
    else if(cnt < ALG_DEBUG_SAVE_RESULT_FRAMS)
    {
        res = f_write( &fp_result, (void*)buf, size, &lr );
        res = FR_OK;
        lr = size;
        if ( (res != FR_OK) || (lr != size) )
        {
            ci_logerr(LOG_AUDIO_IN,"f_write err %d\n", res );
        }
        cnt++;
    }
    else if(ALG_DEBUG_SAVE_RESULT_FRAMS == cnt)
    {
        f_close( &fp_result );
        ci_loginfo(LOG_AUDIO_IN,"colse debug result file\n");
        cnt++;
    }
    else
    {

    }
}
#endif


int32_t audio_in_buffer_init( void )
{
    // uint32_t pcm_buf;
    // uint32_t frame_size,frame_count;

    // /*must be carefull, this malloc aligned 8 byte,
    //  address will used by DMA, so need check DMA config*/
    // if(RETURN_OK != asr_pcm_buf_init(ASRPCM_PER_FRAME_SHIFT_SIZE,ASRPCM_TOTAL_FRAME))
    // {
    //     mprintf("pcm_buff alloc error\n");
    //     return RETURN_ERR;
    // }

    // asr_pcm_buf_get_info(&pcm_buf,&frame_size,&frame_count);

    // /*for debug loadbin used*/
    // mprintf("pcm_buf is 0x%x\n",pcm_buf);

    // asrtop_asrpcmbuf_mem_cfg((uint32_t)pcm_buf,ASRPCM_TOTAL_FRAME,ASRPCM_PER_FRAME_SHIFT_SIZE);

    // return RETURN_OK;
}


void audio_capture_es8388_init(audio_cap_channel_num_t ch,audio_cap_sample_rate_t sr,audio_cap_bit_wide_t bw,
                                    audio_cap_codec_mode_t mode,audio_cap_data_format_t df)
{
    //ES8388_play_init(ES8388_NUM1);
    #if USE_CODE_ES7243
    //es7243_aec_hw_init();
    #endif
}


static void send_audio_data_to_asr( uint32_t buf, uint32_t size )
{
    int frm_shif = get_asrtop_asrfrmshift();
    int frms = size/(frm_shif*sizeof(short));
    extern int send_runvad_msg(unsigned int voice_ptr,int voice_frm_nums,int irq_flag);
    send_runvad_msg(buf ,frms,0);
}




static void audio_in_iis_callback(void)
{
	uint32_t iis3_reg[7] = {0};
	for(int i=0;i<7;i++)
	{
		iis3_reg[i] = *(volatile uint32_t*)((uint32_t)IIS3 + 4*i);
	}
	Scu_Setdevice_Reset((uint32_t)IIS3);
	Scu_Setdevice_ResetRelease((uint32_t)IIS3);
	for(int i=0;i<7;i++)
	{
		*(volatile uint32_t*)((uint32_t)IIS3 + 4*i) = iis3_reg[i];
	}
	mprintf("IIS watchdog callback\n");
}

int8_t audio_capture_start_flag = 0;

static void audio_in_hw_nostraight_init(uint32_t block_size,audio_cap_channel_num_t ch)
{
    audio_capture_init_t audio_cap_init_struct;
   
#if !USE_OUTSIDE_IIS_TO_ASR//正常模式，使用内部codec
    audio_cap_init_struct.codecx = AUDIO_CAP_INNER_CODEC;
    audio_cap_init_struct.block_size = (audio_cap_block_size_t)block_size;
    audio_cap_init_struct.cha_num = ch;
    #if 0//INNER_CODEC_AUDIO_IN_USE_RESAMPLE
    audio_cap_init_struct.sample_rate = AUDIO_CAP_SAMPLE_RATE_32K;
    #else
    audio_cap_init_struct.sample_rate = AUDIO_CAP_SAMPLE_RATE_32K;
    #endif

    audio_cap_init_struct.over_sample = AUDIO_CAP_OVER_SAMPLE_256;
    audio_cap_init_struct.clk_source = AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE;//AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE;//AUDIO_CAP_CLK_SOURCE_OSC;//AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE;
    audio_cap_init_struct.bit_wide = AUDIO_CAP_BIT_WIDE_16;//采样深度，一般16bit
    audio_cap_init_struct.sck_lrck_rate = AUDIO_CAP_SCK_LRCK_64;//SCK和LRCK的比值为64
    audio_cap_init_struct.codec_mode = AUDIO_CAP_CODEC_SLAVE;//主从模式，codec做slave
    audio_cap_init_struct.data_format = AUDIO_CAP_DATA_FORMAT_STANDARD_I2S;//标准iis格式
    audio_cap_init_struct.audio_capture_codec_init = inner_codec_init;//audio_capture_es8388_init;//inner_codec_init;//codec 的初始化函数
    audio_cap_init_struct.is_straight_to_vad = AUDIO_TO_VAD_NOT_STRAIGHT;//是否为直通模式
    audio_cap_init_struct.iis_watchdog_callback = audio_in_iis_callback;
    audio_capture_registe(&audio_cap_init_struct);
    audio_capture_init(&audio_cap_init_struct);

    #if ( (!AUDIO_CAPTURE_USE_MULTI_CODEC) && (USE_SINGLE_MIC_AEC) )
    /* !!! must be after init*/
    /*修改mic增益和输入模式*/
    inner_codec_set_input_mode_right(INNER_CODEC_INPUT_MODE_SINGGLE_ENDED);
    inner_codec_set_mic_gain_right(INNER_CODEC_MIC_AMP_0dB);
    /*配置参考信号的ALC*/
    inner_codec_alc_use_config_t ALC_USE_str;
    memset(&ALC_USE_str,0,sizeof(inner_codec_alc_use_config_t));
    ALC_USE_str.alc_maxgain = INNER_CODEC_ALC_PGA_MAX_GAIN_28_5dB;
    ALC_USE_str.alc_mingain = INNER_CODEC_ALC_PGA_MIN_GAIN__18dB;
    ALC_USE_str.max_level = -14.0f;
    ALC_USE_str.min_level = -16.0f;
    ALC_USE_str.sample_rate = AUDIO_PLAY_SAMPLE_RATE_16K;
    ALC_USE_str.use_ci112x_alc = INNER_CODEC_GATE_DISABLE;
    ALC_USE_str.alc_gate = INNER_CODEC_GATE_ENABLE;
    ALC_USE_str.attack_time = INNER_CODEC_ALC_ATTACK_TIME_2MS;
    ALC_USE_str.decay_time = INNER_CODEC_ALC_DECAY_TIME_2MS;
    ALC_USE_str.hold_time = INNER_CODEC_ALC_HOLD_TIME_2MS;
    ALC_USE_str.noise_gate = INNER_CODEC_GATE_DISABLE;//AEC的line-in端不建议使用noisegate
    ALC_USE_str.fast_decrece_87_5 = INNER_CODEC_GATE_DISABLE;
    ALC_USE_str.zero_cross = INNER_CODEC_GATE_ENABLE;
    ALC_USE_str.noise_gate_threshold = INNER_CODEC_NOISE_GATE_THRE_45dB;
    ALC_USE_str.digt_gain = 197;
    inner_codec_alc_right_config(&ALC_USE_str);
    #endif

    #if !(AUDIO_CAPTURE_USE_MULTI_CODEC)

    // audio_cap_start(AUDIO_CAP_INNER_CODEC);
    audio_capture_start_flag = 1;

    #else  /*two codec, start at same time*/

    audio_cap_init_struct.codecx = AUDIO_CAP_OUTSIDE_CODEC;//codec选择
    audio_cap_init_struct.block_size = (audio_cap_block_size_t)block_size;
    audio_cap_init_struct.cha_num = ch;
    audio_cap_init_struct.sample_rate = AUDIO_CAP_SAMPLE_RATE_16K;
    audio_cap_init_struct.over_sample = AUDIO_CAP_OVER_SAMPLE_256;
    audio_cap_init_struct.clk_source = AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE;
    audio_cap_init_struct.bit_wide = AUDIO_CAP_BIT_WIDE_16;//采样深度，一般16bit
    audio_cap_init_struct.sck_lrck_rate = AUDIO_CAP_SCK_LRCK_64;//SCK和LRCK的比值
    audio_cap_init_struct.codec_mode = AUDIO_CAP_CODEC_SLAVE;//codec做slave
    audio_cap_init_struct.data_format = AUDIO_CAP_DATA_FORMAT_STANDARD_I2S;//标准iis格式
    audio_cap_init_struct.audio_capture_codec_init = audio_capture_es8388_init;//codec 的初始化函数
    audio_cap_init_struct.is_straight_to_vad = AUDIO_TO_VAD_NOT_STRAIGHT;//是否为直通模式

    audio_capture_registe(&audio_cap_init_struct);
    audio_capture_init(&audio_cap_init_struct);
    
    // audio_cap_start(AUDIO_CAP_INNER_CODEC|AUDIO_CAP_OUTSIDE_CODEC);
    audio_capture_start_flag = 2;
    #endif
#else//如果使用外部IIS输入的语音进行识别
    audio_cap_init_struct.codecx = AUDIO_CAP_OUTSIDE_CODEC;
    audio_cap_init_struct.block_size = (audio_cap_block_size_t)block_size;
    audio_cap_init_struct.cha_num = ch;
    audio_cap_init_struct.sample_rate = AUDIO_CAP_SAMPLE_RATE_16K;
    audio_cap_init_struct.over_sample = AUDIO_CAP_OVER_SAMPLE_256;
    audio_cap_init_struct.clk_source = AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE;
    audio_cap_init_struct.bit_wide = AUDIO_CAP_BIT_WIDE_16;//采样深度，一般16bit
    audio_cap_init_struct.sck_lrck_rate = AUDIO_CAP_SCK_LRCK_64;//SCK和LRCK的比值为64
    #if USE_OUTSIDE_IIS_TO_ASR_THIS_BORD_SLAVE//这块板子的IIS做slave
    audio_cap_init_struct.codec_mode = AUDIO_CAP_CODEC_MASTER;//主从模式，codec做slave
    #else
    audio_cap_init_struct.codec_mode = AUDIO_CAP_CODEC_SLAVE;//主从模式，codec做slave
    #endif
    audio_cap_init_struct.data_format = AUDIO_CAP_DATA_FORMAT_STANDARD_I2S;//标准iis格式
    audio_cap_init_struct.audio_capture_codec_init = inner_codec_init;//codec 的初始化函数
    audio_cap_init_struct.is_straight_to_vad = AUDIO_TO_VAD_NOT_STRAIGHT;//是否为直通模式

    audio_capture_registe(&audio_cap_init_struct);
    audio_capture_init(&audio_cap_init_struct);
    // audio_cap_start(AUDIO_CAP_OUTSIDE_CODEC);
    audio_capture_start_flag = 3;
#endif
    audio_cap_iis_watchdog_enable(AUDIO_CAP_INNER_CODEC,ENABLE);
}

void audio_in_preprocess_mode_task(void* p)
{
    //uint32_t next_fram_buff;
    uint32_t block_size;
    
    audio_getdate_t audio_cap_data_str;
    audio_cap_channel_num_t ch;
    
    int ret = 0;

    alg_init();

//    #if AUDIO_CAPTURE_USE_SINGLE_CHANNEL
//    block_size = ALG_FRAME_SIZE*sizeof(short);/*1 ch, use inner codec*/
//    ch = AUDIO_CAP_CHANNEL_LEFT;
//    #else/*multi codec only support stereo input*/
    block_size = ALG_FRAME_SIZE * sizeof(short) * 2;/*2 ch, if multi codec support use es7241 for aec ref*/
    ch = AUDIO_CAP_CHANNEL_STEREO;
//    #endif


	
    audio_in_hw_nostraight_init(block_size,ch);


    #if UPLOAD_VOICE_DATA_EN
    ci_aec_codec_init();
    ci_aec_codec_config(1);
    #endif
    
    
    int asr_inited_done = asrtop_is_startup_done();
    while(!asr_inited_done)
    {
        vTaskDelay(10);
        asr_inited_done = asrtop_is_startup_done();
    }

    //发送消息给系统任务，表示audio_in初始化完成，可以播放welcome播报音了。
    sys_msg_t send_msg;
    send_msg.msg_type = SYS_MSG_TYPE_AUDIO_IN_STARTED;
    send_msg_to_sys_task(&send_msg, NULL);

    if(1 == audio_capture_start_flag)
    {
        audio_cap_start(AUDIO_CAP_INNER_CODEC);
    }
    else if(2 == audio_capture_start_flag)
    {
        audio_cap_start(AUDIO_CAP_INNER_CODEC|AUDIO_CAP_OUTSIDE_CODEC);
    }
    else if(3 == audio_capture_start_flag)
    {
        audio_cap_start(AUDIO_CAP_OUTSIDE_CODEC);
    }
    
    //打印中间结果
    //open_asr_detail_result();
    
    for(;;)
    {
        audio_cap_data_str = audio_capture_getdata(AUDIO_CAP_DATA_GET_BLOCK,30);

    
        if(AUDIO_CAP_DATA_GET_STATE_YES != audio_cap_data_str.data_state)/*used in timeout mode*/
        {
            /*do nothing*/
            ci_logwarn(LOG_AUDIO_IN,"timeout\n");
            continue;
        }
        #if !USE_OUTSIDE_IIS_TO_ASR//正常模式，使用外部IIS输入的语音进行识别
        
        if((AUDIO_CAP_INNER_CODEC != audio_cap_data_str.codec_num)||
        #else//使用外部IIS输入的语音进行识别
        if((AUDIO_CAP_OUTSIDE_CODEC != audio_cap_data_str.codec_num)||
        #endif
            #if (AUDIO_CAPTURE_USE_MULTI_CODEC)/*if support multi codec, par add check codec_2*/    
            (AUDIO_CAP_OUTSIDE_CODEC != audio_cap_data_str.codec_num_2)||
            #endif
            (block_size != audio_cap_data_str.block_size))
        {
            ci_logerr(LOG_AUDIO_IN,"wrong setting\n");
            continue;
        }
        
        /*other module not init ok, do nothint*/
        if(1 != asrtop_is_startup_done())
        {
            continue;
        }
		
        ret = alg_preprocess_single_ch((short*)audio_cap_data_str.current_data_addr,(short*)NULL, ALG_FRAME_SIZE );
        if(1 != ret)
        {
        	continue;
        }
		
        /*send buffer msg*/
        send_audio_data_to_asr(0,0);

        task_alive(*(uint8_t*)p);
    }
}





