#ifndef __CI112X_AUDIO_PRE_RSLT_OUT_H
#define __CI112X_AUDIO_PRE_RSLT_OUT_H

#include "ci112x_audio_play_card.h"
#include "ci112x_gpio.h"

#ifdef __cplusplus
 extern "C" {
#endif



typedef struct
{
    uint32_t gpio_base;
    gpio_pin_t gpio_pin;
    PinPad_Name pin_name;
    IOResue_FUNCTION fun;
}audio_pre_rslt_out_pin_set_t;



typedef enum
{
    PRE_RSLT_IIS_NULL = 0,
    // AUDIO_PLAY_IIS0,
    PRE_RSLT_IIS1,
    PRE_RSLT_IIS2,
    PRE_RSLT_IIS3,
    PRE_RSLT_MAX,
}audio_pre_rslt_out_num_t;


/**
 * @addtogroup audio_pre_rslt_iis_out
 * @{
 */
typedef enum
{
    //!CODEC做SLAVE
    PRE_RSLT_SLAVE = 0,
    //!CODEC做MASTER
    PRE_RSLT_MASTER,
}audio_pre_rslt_out_codec_mode_t;


typedef enum
{
    //!左通道是处理之后的结果，右通道是原始数据
    PRE_RSLT_DATA_LEFT_RSLT_RIGHT_ORIGIN = 0,
    //!左通道是原始数据，右通道是处理之后的结果
    PRE_RSLT_DATA_LEFT_ORIGIN_RIGHT_RSLT,
    //!两个通道都是处理之后的数据，原则上左右通道数据完全一致，一个通道的数据是另一个通道的复制
    PRE_RSLT_DATA_RSLT_COPY,
    //!两个通道的数据都是原始单通道的数据
    PRE_RSLT_DATA_ONE_CHA_ORIGIN_COPY,
    //!两个通道的数据分别是原始双通道的数据
    PRE_RSLT_DATA_TWO_CHA_ORIGIN,
}audio_pre_rslt_out_data_compound_t;



typedef struct
{
    //!codec的主从模式
    audio_pre_rslt_out_codec_mode_t mode;
    //!数据格式
    audio_play_card_data_format_t data_format;
    //!block size（发送多少字节的数据来一次中断，在中断中需填写下一帧发送的数据的起始地址）
    int32_t block_size;
    //!数据拼接模式
    audio_pre_rslt_out_data_compound_t data_comp;
    //!SCK和LRCK时钟频率的比值
    audio_play_card_sck_lrck_rate_t sck_lrck_rate;
}audio_pre_rslt_out_init_t;


void audio_pre_rslt_out_play_card_init(const audio_pre_rslt_out_init_t* const ptr);
void audio_pre_rslt_write_data(int16_t* rslt,int16_t* origin);
void audio_pre_rslt_stop(void);
void audio_pre_rslt_start(void);
/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/





