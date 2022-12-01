#include "ci112x_audio_play_card.h"
#include "ci112x_audio_pre_rslt_out.h"
#include <string.h>
#include "ci112x_codec.h"
#include "ci112x_gpio.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ci112x_iis1_out_debug.h"
#include "sdk_default_config.h"

#if DEBUG_FE_PCM
    #if DEBUG_VAD_FE_PCM
      #define BLOCK_SIZE (640U)
    #else
       #define BLOCK_SIZE (1024U)
    #endif
#else
    #define BLOCK_SIZE (1024U)
#endif

#define BLOCK_NUM (IIS_PRE_OUT_BLOCK_NUM)
#if (CONFIG_DIRVER_BUF_USED_FREEHEAP_EN)
static int16_t* data_buf[BLOCK_NUM];
#else
#if DEBUG_FE_PCM
#pragma location = ".no_cache_psram"
#else
#pragma location = ".no_cache_sram"
#endif
static __no_init int16_t data_buf[BLOCK_NUM][BLOCK_SIZE];
#endif

typedef struct
{
    audio_pre_rslt_out_init_t init_str;
    //写了多少次数据
    uint32_t write_data_cnt;
    //已经发送了多少次数据，写数据频次是固定的，我们不能改变，只能改变发送数据的频度，
    //如果发送太快，会在mute模式再发送上一个buf，
    //如果发送太慢，会扔掉一个buf不发送
    uint32_t send_data_cnt;
    
    int32_t write_send_sub_slave;//在slave模式下，write和send的差值
    //是否使用硬件tx merge功能
    bool hardware_tx_merge;
    //codec号，给playcard用
    audio_play_card_codec_num_t codec_num;
}audio_pre_init_tmp_t;


static audio_pre_init_tmp_t sg_init_tmp_str;


static audio_pre_rslt_out_pin_set_t sg_pre_rslt_vad_pin = 
{
    .gpio_base = (uint32_t)GPIO1,
    .gpio_pin = gpio_pin_1,
    .pin_name = PWM0_PAD,
    .fun = FIRST_FUNCTION,
};


static audio_pre_rslt_out_pin_set_t sg_pre_rslt_wakeup_pin = 
{
    .gpio_base = (uint32_t)GPIO1,
    .gpio_pin = gpio_pin_2,
    .pin_name = PWM1_PAD,
    .fun = FIRST_FUNCTION,
};


#if (CONFIG_DIRVER_BUF_USED_FREEHEAP_EN)
static void *pre_rslt_malloc(size_t size)
{
    return pvPortMalloc(size);
}
#endif


static void pre_rslt_data_buf_init(size_t size)
{
    #if (CONFIG_DIRVER_BUF_USED_FREEHEAP_EN)
    for(int i=0;i<BLOCK_NUM;i++)
    {
        data_buf[i] = pre_rslt_malloc(size);
        if(NULL == data_buf[i])
        {
            CI_ASSERT(0,"alloc error\n");
        }
    }
    #endif
}


void pre_rslt_out_set_vad_pad(audio_pre_rslt_out_pin_set_t* pin)
{
    sg_pre_rslt_vad_pin.gpio_base = pin->gpio_base;
    sg_pre_rslt_vad_pin.gpio_pin = pin->gpio_pin;
    sg_pre_rslt_vad_pin.pin_name = pin->pin_name;
    sg_pre_rslt_vad_pin.fun = pin->fun;
    Scu_SetDeviceGate(sg_pre_rslt_vad_pin.gpio_base,ENABLE);
    Scu_SetIOReuse(sg_pre_rslt_vad_pin.pin_name,sg_pre_rslt_vad_pin.fun);
}


void pre_rslt_out_set_wakeup_pad(audio_pre_rslt_out_pin_set_t* pin)
{
    sg_pre_rslt_wakeup_pin.gpio_base = pin->gpio_base;
    sg_pre_rslt_wakeup_pin.gpio_pin = pin->gpio_pin;
    sg_pre_rslt_wakeup_pin.pin_name = pin->pin_name;
    sg_pre_rslt_wakeup_pin.fun = pin->fun;
    Scu_SetDeviceGate(sg_pre_rslt_wakeup_pin.gpio_base,ENABLE);
    Scu_SetIOReuse(sg_pre_rslt_wakeup_pin.pin_name,sg_pre_rslt_wakeup_pin.fun);
}


#define PI (3.1416926f)
void sine_wave_generate(int16_t* sine_wave, uint32_t sample_rate,uint32_t wave_fre,uint32_t point_num)
{
    int16_t num_of_one_period = sample_rate/wave_fre;//每个周期的采样点数
    for(int i=0;i<point_num;i++)
    {
        sine_wave[i] = (int16_t)(32767.0f * sinf( (2.0f*PI*i) / num_of_one_period ));
    }
}


void audio_pre_rslt_send_data(void*);


/**
 * @brief 语音前处理使用IIS输出功能的初始化
 * 
 * @param ptr audio_pre_rslt_out_init_t结构体指针
 */
void audio_pre_rslt_out_play_card_init(const audio_pre_rslt_out_init_t* const ptr)
{
    audio_play_card_codec_num_t codec_num = AUDIO_PLAY_IIS;
    sg_init_tmp_str.codec_num = codec_num;

    sg_init_tmp_str.init_str.mode = ptr->mode;
    sg_init_tmp_str.init_str.data_format = ptr->data_format;
    #if (CONFIG_DIRVER_BUF_USED_FREEHEAP_EN)
    sg_init_tmp_str.init_str.block_size = ptr->block_size;
    #else
    sg_init_tmp_str.init_str.block_size = BLOCK_SIZE;
    #endif
    sg_init_tmp_str.init_str.data_comp = ptr->data_comp;
    sg_init_tmp_str.write_data_cnt = 0;
    sg_init_tmp_str.send_data_cnt = 0;
    if( (ptr->data_comp ==  PRE_RSLT_DATA_RSLT_COPY)||(ptr->data_comp ==  PRE_RSLT_DATA_ONE_CHA_ORIGIN_COPY) )
    {
        sg_init_tmp_str.hardware_tx_merge = true;
    }
    else
    {
        sg_init_tmp_str.hardware_tx_merge = false;
    }

    //空间分配
    pre_rslt_data_buf_init(ptr->block_size);
    
    //根据前处理模块的主从设置，设置codec的主从模式
    audio_play_card_codec_mode_t codec_mode = AUDIO_PLAY_CARD_CODEC_MASTER;
    if(PRE_RSLT_SLAVE == ptr->mode)
    {
        codec_mode = AUDIO_PLAY_CARD_CODEC_MASTER;
    }
    else
    {
        codec_mode = AUDIO_PLAY_CARD_CODEC_SLAVE;
    }
    
    audio_play_card_init_t audio_play_card_init_str;
    memset((void *)(&audio_play_card_init_str), 0, sizeof(audio_play_card_init_t));

    audio_play_card_init_str.codecx = codec_num;
    audio_play_card_init_str.cha_num = AUDIO_PLAY_CHANNEL_STEREO; 
    audio_play_card_init_str.sample_rate = AUDIO_PLAY_SAMPLE_RATE_16K;
    audio_play_card_init_str.over_sample = AUDIO_PLAY_OVER_SAMPLE_256;
    audio_play_card_init_str.clk_source = AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE;//AUDIO_PLAY_CLK_SOURCE_OSC;//AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE;//AUDIO_PLAY_CLK_SOURCE_DIVIDER_IISPLL;
    audio_play_card_init_str.bit_wide = AUDIO_PLAY_BIT_WIDE_16;
    audio_play_card_init_str.sck_lrck_rate = ptr->sck_lrck_rate;//SCK和LRCK的比值
    audio_play_card_init_str.codec_mode = codec_mode;//AUDIO_PLAY_CARD_CODEC_SLAVE;//codec_mode;               
    audio_play_card_init_str.data_format = ptr->data_format; 
    #if (CONFIG_DIRVER_BUF_USED_FREEHEAP_EN)
    audio_play_card_init_str.block_size = ptr->block_size;
    #else
    audio_play_card_init_str.block_size = BLOCK_SIZE;
    #endif
    if(PRE_RSLT_SLAVE == ptr->mode)
    {
        audio_play_card_init_str.audio_play_buf_end_callback = audio_pre_rslt_send_data;//每一个buf播放完成的回调函数
    }
    
    audio_play_card_registe(&audio_play_card_init_str);
    audio_play_card_init(&audio_play_card_init_str);

    //如果选择的模式copy模式，则使用硬件merge
    if(true == sg_init_tmp_str.hardware_tx_merge)
    {
        audio_play_card_mono_mage_to_stereo(codec_num,ENABLE);
    }
}


/**
 * @brief 写数据到发送端
 * 
 * @param rslt 处理结果的起始指针
 * @param origin 原始数据的起始指针
 *  此函数不可用于中断
 */
#pragma location = ".text_sram"
void audio_pre_rslt_write_data(int16_t* rslt,int16_t* origin)
{
    uint32_t block_size = sg_init_tmp_str.init_str.block_size;
    audio_play_card_codec_num_t codec_num = sg_init_tmp_str.codec_num;
    audio_pre_rslt_out_data_compound_t data_comp = sg_init_tmp_str.init_str.data_comp;

    uint32_t cnt = sg_init_tmp_str.write_data_cnt % BLOCK_NUM;
    //mprintf("send iis cnt = %d\n",cnt);
    int num = block_size/sizeof(int16_t)/2;

    if(PRE_RSLT_DATA_LEFT_RSLT_RIGHT_ORIGIN == data_comp)
    {//左通道结果，右通道原始
        for(int i=0;i<num;i++)
        {
            data_buf[cnt][2*i + 1] = rslt[i];
            data_buf[cnt][2*i] = origin[i];
        }
    }
    else if(PRE_RSLT_DATA_LEFT_ORIGIN_RIGHT_RSLT == data_comp)
    {//右通道结果，左通道原始
        for(int i=0;i<num;i++)
        {
            data_buf[cnt][2*i] = rslt[i];
            data_buf[cnt][2*i + 1] = origin[i];
        }
    }
    else if(PRE_RSLT_DATA_TWO_CHA_ORIGIN == data_comp)
    {//需要输出的数据时双通道的原始数据
        for(int i=0;i<num*2;i++)
        {
            data_buf[cnt][i] = origin[i];
        }
    }
    else if(PRE_RSLT_DATA_RSLT_COPY == data_comp)
    {
        for(int i=0;i<num;i++)
        {
            data_buf[cnt][2*i + 1] = rslt[i];
        }
    }
    else if(PRE_RSLT_DATA_ONE_CHA_ORIGIN_COPY == data_comp)
    {
        for(int i=0;i<num;i++)
        {
            data_buf[cnt][2*i + 1] = origin[i];
        }
    }
    else
    {
        CI_ASSERT(0,"state err!\n");
    }
    if(PRE_RSLT_MASTER == sg_init_tmp_str.init_str.mode)
    {//如果是master模式，写一个数据就发送一个出去
        //mprintf("%d\n",cnt);
        if(get_audio_play_card_queue_remain_nums(codec_num) == (BLOCK_NUM))
        {//如果剩余的消息比总数小一个，
            audio_play_card_receive_one_msg(codec_num);
            //mprintf("send data too slow\n");
        }
        audio_play_queue_state_t rslt = audio_play_card_writedata(codec_num,(void*)data_buf[cnt],block_size,AUDIO_PLAY_BLOCK_TYPE_UNBLOCK,0);
        if(AUDIO_PLAY_QUEUE_FULL == rslt)
        {
            CI_ASSERT(0,"send err\n");
        }
        sg_init_tmp_str.send_data_cnt++;
    }
    else
    {//如果是slave模式，不会马上将数据通过IIS数据发送出去,也就是不会马上调用audio_play_card_writedata函数
        if((BLOCK_NUM - 2) > sg_init_tmp_str.write_data_cnt)
        {//前面几次，没有打开传输之前先发送几个buf到play card
            //mprintf("%d\n",cnt);
            if(get_audio_play_card_queue_remain_nums(codec_num) == (BLOCK_NUM))
            {//如果剩余的消息比总数小一个，
                audio_play_card_receive_one_msg(codec_num);
                //mprintf("send data too slow\n");
            }
            audio_play_queue_state_t rslt = audio_play_card_writedata(codec_num,(void*)data_buf[cnt],block_size,AUDIO_PLAY_BLOCK_TYPE_UNBLOCK,0);
            if(AUDIO_PLAY_QUEUE_FULL == rslt)
            {
                CI_ASSERT(0,"send err\n");
            }
            //sg_init_tmp_str.send_data_cnt++;
        }
    }

    sg_init_tmp_str.write_data_cnt++;
    //mprintf("write:%d\n",(int32_t)sg_init_tmp_str.write_data_cnt);
    if(PRE_RSLT_MASTER == sg_init_tmp_str.init_str.mode)
    {
        if( (BLOCK_NUM - 2) == sg_init_tmp_str.write_data_cnt)
        {//从第二个buf才开始发送，保留一定余量
            audio_play_card_start(codec_num,DISABLE);
            audio_play_card_wake_mute(codec_num,DISABLE); //要解除mute
        }
    }
    else
    {//SLAVE模式下，填了2个buffer就开启传输
        if( (BLOCK_NUM - 2) == sg_init_tmp_str.write_data_cnt)
        {//从第二个buf才开始发送，保留一定余量
            sg_init_tmp_str.write_send_sub_slave = sg_init_tmp_str.write_data_cnt - sg_init_tmp_str.send_data_cnt;
            audio_play_card_start(codec_num,DISABLE);
            audio_play_card_wake_mute(codec_num,DISABLE); //要解除mute
        }
    }   
    
}


//slave模式下，发送数据一定存在两种情况，即外部时钟太快，或者太慢
void audio_pre_rslt_send_data(void * ptr)
{
    audio_play_card_codec_num_t codec_num = sg_init_tmp_str.codec_num;

    uint32_t send_cnt = sg_init_tmp_str.send_data_cnt;
    uint32_t write_cnt = sg_init_tmp_str.write_data_cnt;

    uint32_t cnt = sg_init_tmp_str.send_data_cnt % BLOCK_NUM;
    uint32_t block_size = sg_init_tmp_str.init_str.block_size;

    //判断发送计数和写数据计数的关系，判断是否正常，或者发送太快，或者发送太慢
    if((BLOCK_NUM-2) < (write_cnt - send_cnt))//(send_cnt < (write_cnt - BLOCK_NUM))
    {//如果buf已经写满了，但是还是没有发送，表示写数据太快了
        mprintf("write too many\n");
        audio_play_card_receive_one_msg(codec_num);
        audio_play_card_receive_one_msg(codec_num);
        sg_init_tmp_str.send_data_cnt = write_cnt-2;// - sg_init_tmp_str.write_send_sub_slave;
        //mprintf("send:%d\n",(int32_t)sg_init_tmp_str.send_data_cnt);
    }
    else if((write_cnt - send_cnt) <= 2)
    {//send的次数等于write的次数，表示写数据太慢了
        mprintf("write too less\n");
        audio_play_card_receive_one_msg(codec_num);
        audio_play_card_receive_one_msg(codec_num);
        //写得太慢，就将buf退回去一个，也可以退回去两个，也可以退回去3个，
        //sg_init_tmp_str.send_data_cnt--;
        sg_init_tmp_str.send_data_cnt = write_cnt - sg_init_tmp_str.write_send_sub_slave;
    }

    audio_play_queue_state_t send_rslt;
    cnt = sg_init_tmp_str.send_data_cnt % BLOCK_NUM;
    send_rslt = audio_play_card_writedata(codec_num,(void*)data_buf[cnt],block_size,AUDIO_PLAY_BLOCK_TYPE_UNBLOCK_FROM_ISR,0);
    if(AUDIO_PLAY_QUEUE_FULL == send_rslt)
    {//播放队列满，发送失败，不应该发生
        mprintf("play queue full\n");
    }
    else
    {
        sg_init_tmp_str.send_data_cnt++;
        //mprintf("send:%d\n",(int32_t)sg_init_tmp_str.send_data_cnt);
    }
}


/**
 * @brief 语音前处理输出停止
 * 
 */
void audio_pre_rslt_stop(void)
{
    #if USE_IIS1_OUT_AUDIO
    audio_play_card_codec_num_t codec_num = sg_init_tmp_str.codec_num;
    audio_play_card_stop(codec_num);
    #endif
}


/**
 * @brief 语音前处理输出开始
 * 
 */
void audio_pre_rslt_start(void)
{
    #if USE_IIS1_OUT_AUDIO
    audio_play_card_codec_num_t codec_num = sg_init_tmp_str.codec_num;
    if(get_audio_play_card_queue_remain_nums(codec_num) == BLOCK_NUM)
    {
        audio_play_card_receive_one_msg(codec_num);
    }
    audio_play_card_start(codec_num,DISABLE);
    #endif
}

/********** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/