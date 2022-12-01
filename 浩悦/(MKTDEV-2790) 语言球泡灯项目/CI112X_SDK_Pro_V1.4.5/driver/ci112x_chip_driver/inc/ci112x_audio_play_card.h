/**
 * @file ci112x_audio_play_card.h
 * @brief 
 * @version 0.1
 * @date 2019-03-28
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */


#ifndef __CI112X_AUDIO_PLAY_CARD_H
#define __CI112X_AUDIO_PLAY_CARD_H

#include "ci112x_system.h"
#include "ci112x_iis.h"
#include "ci_log.h"
#include "ci112x_gpio.h"
#include "FreeRTOS.h"

#ifdef __cplusplus
 extern "C" {
#endif


/*******************************************************************************
                    type define
*******************************************************************************/
//最多可以注册多少个声卡
#define MAX_PLAY_CARD_NUM (2)

//队列里面有多少个消息
#define AUDIO_PLAY_CARD_BLOCK_NUM   AUDIO_PLAY_BLOCK_CONT

#define audio_play_card_assert_equal_0(x) CI_ASSERT(x,"audio_play_card_assert_equal_0\n")
#define audio_play_card_assert_notequal_0(x) CI_ASSERT(!(x),"audio_play_card_assert_notequal_0\n")

#define AUDIO_PLAY_CARD_DEFAULT_GPIO_BASE GPIO3
#define AUDIO_PLAY_CARD_DEFAULT_GPIO_PIN gpio_pin_2
#define AUDIO_PLAY_CARD_DEFAULT_PINPAD_NAME GPIO3_2_PAD
#define AUDIO_PLAY_CARD_DEFAULT_FUN_CHOICE FIRST_FUNCTION


/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_audio_play_card ci112x_audio_play_card
 * @brief CI112X芯片audio_play_card驱动
 * @{
 */


/**
 * @brief 放音设备的IIS编号
 * 
 */
typedef enum
{
    AUDIO_PLAY_IIS_NULL = 0,
    // AUDIO_PLAY_IIS0,
    AUDIO_PLAY_IIS1,
    AUDIO_PLAY_IIS2,
    // AUDIO_PLAY_IIS3,
}audio_play_card_iis_num_t;


/**
 * @brief 放音设备的codec编号
 * 
 */
typedef enum
{
    AUDIO_PLAY_CODEC_NULL = 0,
    AUDIO_PLAY_ES8388_1,
    AUDIO_PLAY_ES8388_2,
    AUDIO_PLAY_IIS,
    AUDIO_PLAY_INNER_CODEC,
    AUDIO_PLAY_CODEC_MAX,
}audio_play_card_codec_num_t;


/**
 * @brief 放音设备通道选择
 * 
 */
typedef enum
{
    //!右通道
    AUDIO_PLAY_CHANNEL_RIGHT = 0,
    //!左声道
    AUDIO_PLAY_CHANNEL_LEFT = 1,
    //!双声道
    AUDIO_PLAY_CHANNEL_STEREO = 2,
}audio_play_card_channel_sel_t;


/**
 * @brief 放音设备的采样率
 * 
 */
typedef enum
{
	//!采样率8K
    AUDIO_PLAY_SAMPLE_RATE_8K = 8000U,
	//!采样率12K
    AUDIO_PLAY_SAMPLE_RATE_12K = 12000U,
	//!采样率16K
    AUDIO_PLAY_SAMPLE_RATE_16K = 16000U,
    //!采样率22K
    AUDIO_PLAY_SAMPLE_RATE_22_05K = 22050U,
	//!采样率32K
    AUDIO_PLAY_SAMPLE_RATE_32K = 32000U,
	//!采样率44.1K
    AUDIO_PLAY_SAMPLE_RATE_44_1K = 44100U,
	//!采样率48K
    AUDIO_PLAY_SAMPLE_RATE_48K = 48000U,
}audio_play_card_sample_rate_t;


/**
 * @brief 放音设备的过采样率
 * 
 */
typedef enum
{
	//!过采样率128
    AUDIO_PLAY_OVER_SAMPLE_128 = 128,
	//!过采样率192
    AUDIO_PLAY_OVER_SAMPLE_192 = 192,
	//!过采样率256
    AUDIO_PLAY_OVER_SAMPLE_256 = 256,
	//!过采样率384
    AUDIO_PLAY_OVER_SAMPLE_384 = 384,
}audio_play_card_over_sample_t;


//放音设备的时钟源选择
typedef enum
{
    //IIS时钟源为OSC
    AUDIO_PLAY_CLK_SOURCE_OSC = 0,
    //IIS时钟源为IPCORE
    AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE = 1,
    //IIS时钟源为IISPLL
    //AUDIO_PLAY_CLK_SOURCE_DIVIDER_IISPLL = 2,
}audio_play_card_clk_source_t;


/**
 * @brief 放音设备的数据宽度
 * 
 */
typedef enum
{
    //!数据宽度为16bit
    AUDIO_PLAY_BIT_WIDE_16 = 0,
    //!数据宽度为24bit
    AUDIO_PLAY_BIT_WIDE_24,//24bit
}audio_play_card_bit_wide_t;


/**
 * @brief 放音设备中codec的主从模式
 * 
 */
typedef enum
{
    //!CODEC做SLAVE
    AUDIO_PLAY_CARD_CODEC_SLAVE = 0,
    //!CODEC做MASTER
    AUDIO_PLAY_CARD_CODEC_MASTER,
}audio_play_card_codec_mode_t;


/**
 * @brief 放音设备的数据格式选择
 * 
 */
typedef enum
{
    //!左对齐格式
    AUDIO_PLAY_CARD_DATA_FORMAT_LEFT_JUSTIFY = 0,
    //!右对齐格式
    AUDIO_PLAY_CARD_DATA_FORMAT_RIGHT_JUSTIFY,
    //!标准IIS格式
    AUDIO_PLAY_CARD_DATA_FORMAT_STANDARD_I2S,
}audio_play_card_data_format_t;


/**
 * @brief 向放音设备投递数据消息的时候使用的模式
 * 
 */
typedef enum
{
    //!非阻塞模式
    AUDIO_PLAY_BLOCK_TYPE_UNBLOCK = 0,
    //!阻塞模式
    AUDIO_PLAY_BLOCK_TYPE_BLOCK,
    //!超时模式
    AUDIO_PLAY_BLOCK_TYPE_TIMEOUT,
    //!非阻塞模式,中断模式
    AUDIO_PLAY_BLOCK_TYPE_UNBLOCK_FROM_ISR,
}audio_play_block_t;


/**
 * @brief 放音设备的放音队列状态
 * 
 */
typedef enum
{
    //!播放队列满
    AUDIO_PLAY_QUEUE_FULL = 0,
    //!播放队列有空位
    AUDIO_PLAY_QUEUE_EMPTY = 1,
}audio_play_queue_state_t;

/**
 * @brief SCK和LRCK频率的比值
 * 
 */
typedef enum
{
    //!SCK和LRCK比值为64，最多承载32bit采样深度的数据
    AUDIO_PLAY_SCK_LRCK_64 = 0,
    //!SCK和LRCK比值为32，最多承载16bit采样深度的数据
    AUDIO_PLAY_SCK_LRCK_32,
}audio_play_card_sck_lrck_rate_t;


/**
 * @brief 放音设备初始化结构体类型
 * 
 */
typedef struct
{
    //!codec编号
    audio_play_card_codec_num_t codecx;
    //!通道选择
    audio_play_card_channel_sel_t cha_num;
    //!采样率
    audio_play_card_sample_rate_t sample_rate;
    //!过采样率
    audio_play_card_over_sample_t over_sample;
    //!时钟源
    audio_play_card_clk_source_t clk_source;
    //!采样深度
    audio_play_card_bit_wide_t bit_wide;
    //!SCK和LRCK时钟频率的比值
    audio_play_card_sck_lrck_rate_t sck_lrck_rate;
    //!codec的主从模式
    audio_play_card_codec_mode_t codec_mode;
    //!数据格式
    audio_play_card_data_format_t data_format;
    //!block size（发送多少字节的数据来一次中断，在中断中需填写下一帧发送的数据的起始地址）
    int32_t block_size;
    //!指向codec初始化的函数指针
    void (*audio_play_codec_init)(audio_play_card_sample_rate_t ,audio_play_card_bit_wide_t,
                                audio_play_card_codec_mode_t, audio_play_card_data_format_t);
    //!放音设备将所有的消息中的数据发送完之后的回调函数
    void (*audio_play_buf_end_callback)(void *);
    //!放音设备的打开codec的回调函数
	void (*audio_play_codec_start_callback)(audio_play_card_channel_sel_t ,FunctionalState );
}audio_play_card_init_t;


/**
 * @brief 给放音设备投递数据的结构体类型
 * 
 */
typedef struct
{
    //!需要发送的数据的起始地址
    uint32_t data_current_addr;
    //!需要发送的数据的大小
    int32_t data_remain_size;
}audio_writedate_t;

/*******************************************************************************
                    extern 
*******************************************************************************/


/*******************************************************************************
                    global
*******************************************************************************/

/*******************************************************************************
                    function
*******************************************************************************/

typedef void (*audio_play_callback_func_ptr_t)(void *);

typedef void (*codec_play_start_func_ptr_t)(audio_play_card_channel_sel_t ,FunctionalState );

void codec_play_inner_codec(audio_play_card_channel_sel_t codec_cha,FunctionalState cmd);
void codec_play_8388(audio_play_card_channel_sel_t codec_cha,FunctionalState cmd);


int8_t audio_play_card_registe(const audio_play_card_init_t* regstr);
int8_t audio_play_card_init(const audio_play_card_init_t* regstr);
void audio_play_iis_init(audio_play_card_codec_num_t codec_num,audio_play_card_channel_sel_t cha_num,audio_play_card_sample_rate_t sample_rate,int32_t block_size,
                        audio_play_card_clk_source_t clk_sor);
audio_play_queue_state_t audio_play_card_writedata(audio_play_card_codec_num_t codec_num,void* pcm_buf,uint32_t buf_size,audio_play_block_t type,uint32_t time);
void audio_play_card_start(const audio_play_card_codec_num_t codec_num,FunctionalState pa_cmd);
void inner_codec_play_init(audio_play_card_sample_rate_t sample_rate, audio_play_card_bit_wide_t bit_wide,
                             audio_play_card_codec_mode_t codec_mode,audio_play_card_data_format_t data_format);
void audio_play_card_stop(const audio_play_card_codec_num_t codec_num);

void audio_play_card_iis_start(const audio_play_card_codec_num_t codec_num);
void audio_play_card_wake_mute(const audio_play_card_codec_num_t codec_num,FunctionalState cmd);
void audio_play_card_mono_mage_to_stereo(const audio_play_card_codec_num_t codec_num,FunctionalState cmd);
void audio_play_card_delete(const audio_play_card_codec_num_t codec_num);
int16_t get_audio_play_card_queue_remain_nums(audio_play_card_codec_num_t codec_num);
void reset_audio_play_card_queue(audio_play_card_codec_num_t codec_num);
void audio_play_card_iis_stop(const audio_play_card_codec_num_t codec_num);
int32_t audio_play_card_queue_is_full(audio_play_card_codec_num_t codec_num);
void audio_play_card_set_vol_gain(audio_play_card_codec_num_t codec_num,int32_t l_gain,int32_t r_gain);
void audio_play_card_force_mute(const audio_play_card_codec_num_t codec_num,FunctionalState force_mute_cmd);
void audio_play_card_pa_da_en(const audio_play_card_codec_num_t codec_num);
void audio_play_card_pa_da_diable(const audio_play_card_codec_num_t codec_num);
void audio_play_card_iis_enable(const audio_play_card_codec_num_t codec_num);
void audio_play_card_receive_one_msg(audio_play_card_codec_num_t codec_num);
void audio_play_card_handler(const audio_play_card_iis_num_t iis_num,IISDMA_TypeDef* IISDMA,IISChax iischa,BaseType_t * xHigherPriorityTaskWoken);




#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/





