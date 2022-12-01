/**
 * @file ci112x_audio_capture.h
 * @brief 
 * @version 0.1
 * @date 2019-03-28
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */

#ifndef __CI112X_AUDIO_CAPTURE_H
#define __CI112X_AUDIO_CAPTURE_H

#include "ci112x_system.h"
#include "ci112x_iis.h"
#include "ci_log.h"
#include "FreeRTOS.h"
#include "ci112x_audio_play_card.h"


#ifdef __cplusplus
 extern "C" {
#endif





/*******************************************************************************
                            define
*******************************************************************************/
//CI112X只能使用最多两个CODEC同时录音，不可更改，内部计数使用
#define MAX_MULTI_CODEC_NUM 2
//最多同时使用的codec数量

//函数指针结构体类型
typedef void (*audio_capture_void_void_t)(void);



/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_audio_capture ci112x_audio_capture
 * @brief CI112X芯片audio_capture驱动
 * @{
 */

/**
 * @brief IIS编号枚举类型
 * 
 */
typedef enum
{
    AUDIO_CODEC_ALC_NORMAL = 0,
    AUDIO_CODEC_ALC_LOCAL_PLAY = 1,
	AUDIO_CODEC_ALC_EXTERNAL_PLAY = 2,
}audio_codec_alc_mode;


/**
 * @brief IIS编号枚举类型
 * 
 */
typedef enum
{
    AUDIO_CAP_IIS_NULL = 0,
    AUDIO_CAP_IIS1,
    AUDIO_CAP_IIS3,
} audio_cap_iis_num_t;


/**
 * @brief codec编号枚举类型
 * 
 */
typedef enum
{
    AUDIO_CAP_CODEC_NULL = 0<<0,
    AUDIO_CAP_INNER_CODEC = 1<<0,
    AUDIO_CAP_OUTSIDE_CODEC = 1<<1,
} audio_cap_codec_num_t;


/**
 * @brief 每帧数据大小（字节为单位）枚举类型
 * 必须为16byte的整数倍（IISDMA搬运的最小单位）
 * 例如：16bit 双声道时，16ms的数据大小为1024byte
 *      16bit 双声道时，10ms的数据大小为640byte
 * 
 */
typedef enum
{
    AUDIO_CAP_BLOCK_SIZE_0 = 0,
    //!每帧数据大小为640, 16bit 双声道时，10ms的数据
    AUDIO_CAP_BLOCK_SIZE_320 = 320,
    //!每帧数据大小为512
    AUDIO_CAP_BLOCK_SIZE_512 = 512,
    //!每帧数据大小为640, 16bit 双声道时，10ms的数据
    AUDIO_CAP_BLOCK_SIZE_640 = 640,
    //!每帧数据大小为1024, 16bit 双声道时，16ms的数据
    AUDIO_CAP_BLOCK_SIZE_1024 = 1024,
    //!每帧数据大小为2048, 16bit 双声道时，32ms的数据
    AUDIO_CAP_BLOCK_SIZE_2048 = 2048,
} audio_cap_block_size_t;


/**
 * @brief 声道选择枚举类型
 * 
 */
typedef enum
{
    //!左声道
    AUDIO_CAP_CHANNEL_LEFT = 0,
    //!右声道
    AUDIO_CAP_CHANNEL_RIGHT = 1,
    //!双声道
    AUDIO_CAP_CHANNEL_STEREO = 2,
} audio_cap_channel_num_t; 


/**
 * @brief 采样率枚举类型
 * 
 */
typedef enum
{
    AUDIO_CAP_SAMPLE_RATE_8K = 8000,
    AUDIO_CAP_SAMPLE_RATE_12K = 12000,
    AUDIO_CAP_SAMPLE_RATE_16K = 16000,
    AUDIO_CAP_SAMPLE_RATE_24K = 24000,
    AUDIO_CAP_SAMPLE_RATE_32K = 32000,
    AUDIO_CAP_SAMPLE_RATE_44_1K = 44100,
    AUDIO_CAP_SAMPLE_RATE_48K = 48000,
    AUDIO_CAP_SAMPLE_RATE_96K = 96000,
} audio_cap_sample_rate_t;


/**
 * @brief 过采样率
 * 
 */
typedef enum
{
    AUDIO_CAP_OVER_SAMPLE_128 = 128,
    AUDIO_CAP_OVER_SAMPLE_192 = 192,
    AUDIO_CAP_OVER_SAMPLE_256 = 256,
    AUDIO_CAP_OVER_SAMPLE_384 = 384,
}audio_cap_over_sample_t;


/**
 * @brief 放音设备的时钟源选择
 * 
 */
typedef enum
{
    //!IIS时钟源为OSC
    AUDIO_CAP_CLK_SOURCE_OSC = 0,
    //!IIS时钟源为IPCORE
    AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE = 1,
    //!IIS时钟源为IISPLL
    //AUDIO_CAP_CLK_SOURCE_DIVIDER_IISPLL = 2,
}audio_cap_clk_source_t;


/**
 * @brief 数据位宽枚举类型
 * 
 */
typedef enum
{
    //!采样深度16bit
    AUDIO_CAP_BIT_WIDE_16 = 0,
    //!采样深度24bit
    AUDIO_CAP_BIT_WIDE_24,
} audio_cap_bit_wide_t;


/**
 * @brief 数据采集美剧类型（中断模式或者直通模式）
 * 
 */
typedef enum
{
    //!需要中断处理的模式
    AUDIO_TO_VAD_NOT_STRAIGHT = 0,
    //!直通模式
    AUDIO_TO_VAD_STRAIGHT,
} audio_is_straight_to_vad_t;


/**
 * @brief 获取数据的模式
 * 
 */
typedef enum
{
    //!非阻塞模式
    AUDIO_CAP_DATA_GET_UNBLOCK = 0,
    //!阻塞模式
    AUDIO_CAP_DATA_GET_BLOCK,
    //!超时模式
    AUDIO_CAP_DATA_GET_TIMEOUT,
}audio_cap_data_get_t;


/**
 * @brief 当前帧数据的状态
 * 
 */
typedef enum
{
    //!数据是OK的
    AUDIO_CAP_DATA_GET_STATE_YES = 0x11,
    //!没有数据
    AUDIO_CAP_DATA_GET_STATE_NODATA = 0x22,
    //!等待超时
    AUDIO_CAP_DATA_GET_STATE_TIMEOUT = 0x33,
}audio_cap_data_get_state_t;


/**
 * @brief CODEC主从模式选择
 * 
 */
typedef enum
{
    //!CODEC做SLAVE
    AUDIO_CAP_CODEC_SLAVE = 0,
    //!CODEC做MASTER
    AUDIO_CAP_CODEC_MASTER,
}audio_cap_codec_mode_t;


/**
 * @brief 录音系统的数据格式
 * 
 */
typedef enum
{
    //!左对齐格式
    AUDIO_CAP_DATA_FORMAT_LEFT_JUSTIFY = 0,
    //!右对齐格式
    AUDIO_CAP_DATA_FORMAT_RIGHT_JUSTIFY,
    //!标准IIS格式
    AUDIO_CAP_DATA_FORMAT_STANDARD_I2S,
}audio_cap_data_format_t;


/**
 * @brief SCK和LRCK频率的比值
 * 
 */
typedef enum
{
    //!SCK和LRCK比值为64，最多承载32bit采样深度的数据
    AUDIO_CAP_SCK_LRCK_64 = 0,
    //!SCK和LRCK比值为32，最多承载16bit采样深度的数据
    AUDIO_CAP_SCK_LRCK_32,
}audio_cap_sck_lrck_rate_t;


/**
 * @brief 录音设备初始化结构体类型
 * 
 */
typedef struct
{
    //!codec编号
    audio_cap_codec_num_t codecx;
    //!每帧数据的大小
    audio_cap_block_size_t block_size;
    //!录音通道
    audio_cap_channel_num_t cha_num;
    //!采样率，一般16K
    audio_cap_sample_rate_t sample_rate;
    //!过采样率
    audio_cap_over_sample_t over_sample;
    //!时钟源
    audio_cap_clk_source_t clk_source;
    //!SCK和LRCK时钟频率的比值
    audio_cap_sck_lrck_rate_t sck_lrck_rate;
    //!采样深度，一般16bit
    audio_cap_bit_wide_t bit_wide;
    //!codec的主从模式设置（是codec的主从模式，不是IIS的主从模式）
    audio_cap_codec_mode_t codec_mode;
    //!数据格式
    audio_cap_data_format_t data_format;
    //!是否为直通模式
    audio_is_straight_to_vad_t is_straight_to_vad;
    //!直通模式下pcm_buf的首地址
    uint32_t addr_pcm_buf_for_straight;
    //!直通模式下pcm_buf的大小
    uint32_t size_pcm_buf_for_straight;
    //!指向codec初始化的函数指针
    void (*audio_capture_codec_init)(audio_cap_channel_num_t ,audio_cap_sample_rate_t ,audio_cap_bit_wide_t,
                                    audio_cap_codec_mode_t,audio_cap_data_format_t);
    //!iis挂掉之后，触发的中断函数中调用的回调函数
    audio_capture_void_void_t iis_watchdog_callback;
}audio_capture_init_t;


#if AUDIO_CAPTURE_USE_MULTI_CODEC
/**
 * @brief 获取数据结构体类型
 * 
 */
typedef struct
{
    //!此block的数据在memory中的起始地址
    uint32_t current_data_addr;
    //!此block的数据是哪个codec的
    audio_cap_codec_num_t codec_num;
    //!此block的数据来自iisx
    audio_cap_iis_num_t iis_num;
    //!（双通道模式下）第二个通道此block的数据在memory中的起始地址
    uint32_t current_data_addr_2;
    //!（双通道模式下）第二个通道此block的数据是哪个codec的
    audio_cap_codec_num_t codec_num_2;
    //!（双通道模式下）第二个通道此block的数据来自iisx
    audio_cap_iis_num_t iis_num_2;
    //!block大小
    audio_cap_block_size_t block_size;
    //!此block数据的状态
    audio_cap_data_get_state_t data_state;
	//!此block数据alc的状态
    audio_codec_alc_mode alc_mode;
}audio_getdate_t;
#else
/**
 * @brief 获取数据结构体类型
 * 
 */
typedef struct
{
    //!此block的数据在memory中的起始地址
    uint32_t current_data_addr;
    //!此block的数据来自iisx
    audio_cap_iis_num_t iis_num;
    //!此block的数据是哪个codec的
    audio_cap_codec_num_t codec_num;
    //!block大小
    audio_cap_block_size_t block_size;
    //!此block数据的状态
    audio_cap_data_get_state_t data_state;
	//!此block数据alc的状态
    audio_codec_alc_mode alc_mode;
}audio_getdate_t;
#endif

/*******************************************************************************
                    extern 
*******************************************************************************/


/*******************************************************************************
                    global
*******************************************************************************/

/*******************************************************************************
                    function
*******************************************************************************/

int8_t audio_capture_init(const audio_capture_init_t* regstr);
int8_t audio_capture_registe(const audio_capture_init_t* regstr);
void inner_codec_init(audio_cap_channel_num_t cha_num,audio_cap_sample_rate_t sample_rate,
                    audio_cap_bit_wide_t bit_wide,audio_cap_codec_mode_t codec_mode,
                    audio_cap_data_format_t data_format);
void audio_cap_handler(audio_cap_iis_num_t iis_num,IISDMA_TypeDef* IISDMA,IISChax iischa,int8_t* is_addr_rollback,BaseType_t * xHigherPriorityTaskWoken);
audio_getdate_t audio_capture_getdata(audio_cap_data_get_t type,uint32_t time);
void audio_cap_start(uint16_t codec_num);//允许同时打开多个codec
void audio_cap_stop(audio_cap_codec_num_t codec_num);
void audio_cap_mute(audio_cap_codec_num_t codec_num,FunctionalState cmd);
void audio_cap_delete(const audio_cap_codec_num_t codec_num);
void audio_cap_iis_watchdog_enable(const audio_cap_codec_num_t codec_num,FunctionalState cmd);
void audio_cap_samplerate_set(const audio_cap_codec_num_t codec_num,audio_cap_sample_rate_t sr);

void iis3_div_para_change_to_half_of_180M_clk(void);
void iis3_div_para_change_to_180M_clk(void);
void iis1_div_para_change_to_half_of_180M_clk(void);
void iis1_div_para_change_to_180M_clk(void);

/**
 * @}
 */

/**
 * @brief 录音设备状态结构体
 * 
 */
typedef struct
{
    //过采样率
    audio_cap_over_sample_t over_sample;
    //时钟源
    audio_cap_clk_source_t clk_source;
    //此组的codec是否已经初始化
    uint8_t is_codec_init;
    //已注册的codec号
    audio_cap_codec_num_t codec_num;
    //当前设备注册的block size
    audio_cap_block_size_t block_size;
    //存储当前数据的buf地址
    uint32_t buf_addr;
    //iis挂死之后的回调函数
    audio_capture_void_void_t iis_watchdog_callback;
}capture_equipment_state_t;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/





