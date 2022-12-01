#ifndef __CI112X_IIS1_OUT_DEBUG_H
#define __CI112X_IIS1_OUT_DEBUG_H

#include "user_config.h"

/***************************** codec iis config ******************************/
//是否调试送入FE计算的PCM数据
#define     DEBUG_FE_PCM                       (0)

//是否调试送入本地asr_FE的计算的pcm数据
#define    DEBUG_VAD_FE_PCM                    (0)
//是否调试送入cloud_asr的PCM数据
#define    DEBUG_SEND_CLOUD_PCM                (0)


//使用IIS输出算法处理之后的语音
#if DEBUG_FE_PCM 
    #define USE_IIS1_OUT_AUDIO  (0)
#else
    #if !USE_OUTSIDE_CODEC_PLAY
    #define USE_IIS1_OUT_AUDIO  (1)
    #else
    #define USE_IIS1_OUT_AUDIO  (0)
    #endif
#endif



//!内部的codec是否使用探针模式，即从I2S1的管脚引出来
#if (USE_IIS1_OUT_AUDIO||DEBUG_FE_PCM) 
#define INNER_CODEC_ADC_USE_PROBE           (0)
#else
    #if !USE_OUTSIDE_CODEC_PLAY
    #define INNER_CODEC_ADC_USE_PROBE           (1)
    #else
    #define INNER_CODEC_ADC_USE_PROBE           (0)
    #endif
#endif


#if(!IIS1_ENABLE)
    #undef USE_IIS1_OUT_AUDIO
    #define USE_IIS1_OUT_AUDIO              (0)
    #undef INNER_CODEC_ADC_USE_PROBE
    #define INNER_CODEC_ADC_USE_PROBE       (0)
#endif


/*IIS1输出前处理结果的block数目*/
#if DEBUG_FE_PCM
#define IIS_PRE_OUT_BLOCK_NUM   (80)
#else
#define IIS_PRE_OUT_BLOCK_NUM   (6)
#endif



#endif
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/





