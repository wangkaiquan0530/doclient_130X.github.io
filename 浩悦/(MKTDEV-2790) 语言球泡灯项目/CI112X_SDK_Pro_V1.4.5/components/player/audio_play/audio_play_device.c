/**
 * @file audio_play_device.c
 * @brief  播放器底层设备
 * @version 1.0
 * @date 2019-04-02
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#include "audio_play_device.h"
#include "audio_play_api.h"
#include "audio_play_process.h"
#include "es8388.h"
#include "ci112x_audio_play_card.h"
#include <string.h>
#include "audio_play_os_port.h"
#include "ci112x_codec.h"
#include "sdk_default_config.h"
#include "freertos.h"


#if USE_INNER_CODEC
static audio_play_card_codec_num_t sg_play_device_num = AUDIO_PLAY_INNER_CODEC;
#else
static audio_play_card_codec_num_t sg_play_device_num = AUDIO_PLAY_IIS;
#endif

static int32_t g_audio_play_gain = 0;

/**
 * @brief codec初始化
 * 
 * @param sr 采样率
 * @param bw 位宽
 * @param mode 模式
 * @param df 格式
 */
void audio_play_codec_init(audio_play_card_sample_rate_t sr, audio_play_card_bit_wide_t bw, audio_play_card_codec_mode_t mode, audio_play_card_data_format_t df)
{

}


/**
 * @brief buf释放回调函数
 * 
 * @param xHigherPriorityTaskWoken 中断退出OS调度任务标志
 */
void send_one_buf_done_event(void * xHigherPriorityTaskWoken)
{
    /* 设置buf done 事件标志 */
    audio_play_event_group_set_bits_isr(audio_play_cmd_event_group,AUDIO_PLAY_EVENT_ONE_BUF_DONE,xHigherPriorityTaskWoken ); 
}


/**
 * @brief 硬件超时回调函数
 * 
 * @param xHigherPriorityTaskWoken 中断退出OS调度任务标志
 */
void audio_play_hw_timeout(void * xHigherPriorityTaskWoken)
{
    /* 设置buf done 事件标志 */
    audio_play_event_group_set_bits_isr(audio_play_cmd_event_group,AUDIO_PLAY_EVENT_PLAY_TIMEOUT,xHigherPriorityTaskWoken ); 
}


void outside_codec_play_init(audio_play_card_sample_rate_t sample_rate, audio_play_card_bit_wide_t bit_wide,
                            audio_play_card_codec_mode_t codec_mode,audio_play_card_data_format_t data_format)
{
    //es8311_init();
}      

void outside_codec_play_start(audio_play_card_channel_sel_t codec_cha,FunctionalState cmd)
{
    if(ENABLE == cmd)
    {
        //es8311_dac_on();
    }
    else
    {
        //es8311_dac_off();
    }
}
/**
 * @brief 播放硬件声卡预初始化
 * 
 */
void audio_play_hw_per_init(void)
{
    /* 声卡硬件配置注册 */
    audio_play_card_init_t audio_play_card_init_str;
    memset((void *)(&audio_play_card_init_str), 0, sizeof(audio_play_card_init_t));

    audio_play_card_init_str.codecx = sg_play_device_num;
    audio_play_card_init_str.cha_num = AUDIO_PLAY_CHANNEL_STEREO; //AUDIO_PLAY_CHANNEL_LEFT;//AUDIO_PLAY_CHANNEL_STEREO;
    audio_play_card_init_str.sample_rate = AUDIO_PLAY_SAMPLE_RATE_16K;
    audio_play_card_init_str.over_sample = AUDIO_PLAY_OVER_SAMPLE_256;
    audio_play_card_init_str.clk_source = AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE;
    audio_play_card_init_str.bit_wide = AUDIO_PLAY_BIT_WIDE_16;
    audio_play_card_init_str.codec_mode = AUDIO_PLAY_CARD_CODEC_SLAVE;               //主从模式，codec做slave
    audio_play_card_init_str.data_format = AUDIO_PLAY_CARD_DATA_FORMAT_STANDARD_I2S; //主从模式，codec做slave
    #if USE_INNER_CODEC
    audio_play_card_init_str.audio_play_codec_init = inner_codec_play_init;
    audio_play_card_init_str.audio_play_codec_start_callback = codec_play_inner_codec;
    #else
    audio_play_card_init_str.audio_play_codec_init = outside_codec_play_init;
    audio_play_card_init_str.audio_play_codec_start_callback = outside_codec_play_start;
    #endif
    audio_play_card_init_str.audio_play_buf_end_callback = send_one_buf_done_event;
    
    audio_play_card_init_str.block_size = 1024;//1152;

    audio_play_card_registe(&audio_play_card_init_str);

    //注册该函数后超时将产生停止播放信号，否则不允许超时
    //audio_play_card_play_done_timeout_callback_set(audio_play_hw_timeout ,500);
    
    audio_play_card_init(&audio_play_card_init_str);
    
    audio_play_card_set_vol_gain(sg_play_device_num,AUDIO_PLAYER_CONFIG_DEFAULT_VOLUME,AUDIO_PLAYER_CONFIG_DEFAULT_VOLUME);
    #if USE_INNER_CODEC
    codec_play_inner_codec(AUDIO_PLAY_CHANNEL_STEREO,ENABLE);
    #endif
}


/**
 * @brief 调节播放音量
 * 
 * @param gain 音量(0--100)
 */
void audio_play_set_vol_gain(int32_t gain)
{
    g_audio_play_gain = gain;

    audio_play_card_set_vol_gain(sg_play_device_num,gain,gain);
}


/**
 * @brief 获取当前播放音量
 * 
 * @return int32_t 音量(0--100)
 */
int32_t audio_play_get_vol_gain(void)
{
    return g_audio_play_gain;
}


/**
 * @brief 设置静音
 * 
 * @param is_mute 是否静音
 */
void audio_play_set_mute(bool is_mute)
{
    #if AUDIO_PLAYER_ENABLE
    if(is_mute)
    {
        audio_play_card_force_mute(sg_play_device_num,ENABLE);
    }
    else
    {
        audio_play_card_force_mute(sg_play_device_num,DISABLE);
    }
    #endif
}


/**
 * @brief pa、da控制
 * 
 * @param cmd pa使能或失能
 */
void audio_play_hw_pa_da_ctl(FunctionalState cmd)
{
    #if AUDIO_PLAYER_ENABLE
    if(ENABLE == cmd)
    {
        audio_play_card_pa_da_en(sg_play_device_num);
        inner_codec_hpout_mute_disable();
    }
    else
    {
        audio_play_card_pa_da_diable(sg_play_device_num);
        inner_codec_hpout_mute();
    }
    #endif
}
extern int get_alc_state();
/**
 * @brief 声卡驱动开始播放
 * 
 */
void audio_play_hw_start(FunctionalState pa_cmd)
{
    #if AUDIO_PLAYER_ENABLE
	//播放之前，关ALC
	//mprintf("alc off---%d",get_alc_state());
    
	if(get_alc_state()!=0)
	{
		inner_codec_left_alc_enable(INNER_CODEC_GATE_DISABLE,INNER_CODEC_USE_ALC_CONTROL_PGA_GAIN);
	}
	inner_codec_pga_gain_config_via_reg43_53_db(INNER_CODEC_LEFT_CHA,28.5f);//如果使用AEC，这个值需要更改为15.5
    

    static uint32_t last_hw_samplerate = 0; 
    static uint32_t last_hw_block_size = 0; 
    audio_format_info_t audio_format_info;

    /* 获取当前audio格式 */
    get_audio_format_info(&audio_format_info);

    #if (USE_OUTSIDE_IIS_TO_ASR && USE_OUTSIDE_CODEC_PLAY)//播报和收音都走外部IIS1
    if(audio_format_info.samprate != 16000)//不允许播放16K采样率之外的音频，会导致收音异常
    {
        CI_ASSERT(0,"sample_rate err!\n");
    }
    #endif
        
    /* 设置声卡硬件采样率 */
    if((audio_format_info.out_min_size != last_hw_block_size)||(audio_format_info.samprate  != last_hw_samplerate))
    {
        #if AUDIO_PLAY_USE_INNER_CODEC_HPOUT_MUTE
        inner_codec_hpout_mute();
        #endif
        last_hw_samplerate = audio_format_info.samprate;
        last_hw_block_size = audio_format_info.out_min_size;
        
        if(audio_format_info.samprate == 8000)
        {
            audio_play_iis_init(sg_play_device_num, AUDIO_PLAY_CHANNEL_STEREO, (audio_play_card_sample_rate_t)audio_format_info.samprate,audio_format_info.out_min_size,
                            AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE);
        }
        else if((audio_format_info.samprate == 44100) || (audio_format_info.samprate == 22050))
        {
            audio_play_iis_init(sg_play_device_num, AUDIO_PLAY_CHANNEL_STEREO, (audio_play_card_sample_rate_t)audio_format_info.samprate,audio_format_info.out_min_size,
            		AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE);
        }
        else
        {
            #if USE_OUTSIDE_CODEC_PLAY
            audio_play_iis_init(sg_play_device_num, AUDIO_PLAY_CHANNEL_STEREO, (audio_play_card_sample_rate_t)audio_format_info.samprate,audio_format_info.out_min_size,
            		AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE);
            #else
            audio_play_iis_init(sg_play_device_num, AUDIO_PLAY_CHANNEL_STEREO, (audio_play_card_sample_rate_t)audio_format_info.samprate,audio_format_info.out_min_size,
            		AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE);
            #endif
        }
        #if AUDIO_PLAY_USE_INNER_CODEC_HPOUT_MUTE
        inner_codec_hpout_mute_disable();
        #endif
    }
    
    /* 设置声卡硬件声道 */
    if(1 == audio_format_info.nChans)
    {
        audio_play_card_mono_mage_to_stereo(sg_play_device_num,ENABLE);
    }
    else
    {
        audio_play_card_mono_mage_to_stereo(sg_play_device_num,DISABLE);
    }
    /* 启动声卡硬件 */
    audio_play_card_iis_enable(sg_play_device_num);
    //audio_play_card_wake_mute(sg_play_device_num,ENABLE);
    //audio_play_task_delay(pdMS_TO_TICKS(16));
    //audio_play_card_wake_mute(sg_play_device_num,DISABLE);
    /* 启动PA */
    if(ENABLE == pa_cmd)
    {
        audio_play_card_pa_da_en(sg_play_device_num);
    }
    else
    {
        inner_codec_hpout_mute_disable();//如果不开关PA，就开关codec的HP，以去掉电流声
    }
    #endif
}

/**
 * @brief 声卡驱动停止播放
 * 
 */
void audio_play_hw_stop(FunctionalState pa_cmd)
{
    #if AUDIO_PLAYER_ENABLE
	//播放结束，开ALC
	//mprintf("alc on---%d",get_alc_state());
    #if !USE_CWSL
	if(get_alc_state()!=0)
	{
		inner_codec_left_alc_enable(INNER_CODEC_GATE_ENABLE,INNER_CODEC_USE_ALC_CONTROL_PGA_GAIN);
	}
    #endif
	audio_play_card_wake_mute(sg_play_device_num,ENABLE);
    audio_play_task_delay(pdMS_TO_TICKS(16));
    if(ENABLE == pa_cmd)
    {
        audio_play_card_pa_da_diable(sg_play_device_num);
    }
    else
    {
        inner_codec_hpout_mute();//如果不开关PA，就开关codec的HP，以去掉电流声
    }
     /* 停止声卡硬件 */
    audio_play_card_stop(sg_play_device_num); 
    #endif
}


/**
 * @brief 向硬件写入音频数据
 * 
 */
int32_t audio_play_hw_write_data(void* pcm_buf,uint32_t buf_size)
{
    audio_play_card_writedata(sg_play_device_num,pcm_buf, buf_size, AUDIO_PLAY_BLOCK_TYPE_UNBLOCK, 0);
    return AUDIO_PLAY_OS_SUCCESS;
}


/**
 * @brief 清理写入硬件的音频数据
 * 
 */
int32_t audio_play_hw_clean_data(void)
{
    reset_audio_play_card_queue(sg_play_device_num);
    return AUDIO_PLAY_OS_SUCCESS;
}


/**
 * @brief 查询音频数据消息是否发满
 * 
 */
int32_t audio_play_hw_queue_is_full(void)
{
    return audio_play_card_queue_is_full(sg_play_device_num);
}


/**
 * @brief 查询硬件待处理的消息个数
 * 
 */
int32_t audio_play_hw_get_queue_remain_nums(void)
{
    return (int32_t)get_audio_play_card_queue_remain_nums(sg_play_device_num);
}


#if AUDIO_PLAYER_FIX_DEVICE_POP_ISSUE_HARD
void audio_play_hw_fix_pop_issue_hard(void)
{
    extern int32_t get_cur_dac_l_gain(void);
    extern int32_t get_cur_dac_r_gain(void);
    int32_t dac_l_gain = get_cur_dac_l_gain();
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*95/100,dac_l_gain*95/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*90/100,dac_l_gain*90/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*85/100,dac_l_gain*85/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*80/100,dac_l_gain*80/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*60/100,dac_l_gain*60/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*40/100,dac_l_gain*40/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,dac_l_gain*20/100,dac_l_gain*20/100);
    audio_play_task_delay(pdMS_TO_TICKS(2));
    audio_play_card_set_vol_gain(sg_play_device_num,0,0);
}


void audio_play_hw_fix_pop_issue_hard_done(void)
{
    audio_play_set_vol_gain(audio_play_get_vol_gain());
}
#endif


#if AUDIO_PLAYER_FIX_DEVICE_POP_ISSUE_SOFT
void audio_play_hw_fix_pop_issue_soft(void)
{
    int32_t timeout = pdMS_TO_TICKS(100);
    audio_play_card_set_pause_flag(AUDIO_PLAY_CARD_PAUSE_DATA_DECREACE_1);
    ci_logdebug(LOG_AUDIO_PLAY,"dp%d\n",AUDIO_PLAY_CARD_PAUSE_DATA_DECREACE_1);
    while(AUDIO_PLAY_CARD_PAUSE_END != audio_play_card_get_pause_flag())
    {
        audio_play_task_delay(2);
        timeout -= 2;
        if(timeout <= 0)
        {
            /* 超时跳出 */
            break;
        }
    }
}


void audio_play_hw_fix_pop_issue_soft_done(void)
{
    audio_play_card_set_pause_flag(AUDIO_PLAY_CARD_PAUSE_NULL);
}   
#endif
