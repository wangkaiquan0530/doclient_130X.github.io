/**
 * @file ci112x_audio_capture.c
 * @brief CI112X的录音设备驱动设计
 *      支持使用不同的codec（INNO codec、8388、8374等）进行录音操作；
 *      支持录音格式设置，主要包括：每帧数据大小、通道数、采样位宽、采样率；
 *      支持获取录音数据；
 *      支持采音动态切换到hw vad模式。
 * @version 0.1
 * @date 2019-03-28
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */

#include "sdk_default_config.h"

#include "ci112x_audio_capture.h"
#include "ci112x_iis1_out_debug.h"
#include "ci112x_system.h"
#include "ci112x_iis.h"
#include "ci112x_iisdma.h"
#include "ci112x_codec.h"
#include "ci112x_scu.h"
#include "ci112x_core_eclic.h"
#include "ci112x_core_misc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "platform_config.h"


/*******************************************************************************
                        define
*******************************************************************************/

typedef struct
{
    float now_div_f;
    int16_t now_sample_rate;
    int16_t now_over_sample;

}iis_change_div_t;

#define audio_capture_assert_equal_0(x) CI_ASSERT(x, "audio_capture_assert_equal_0\n")
#define audio_capture_assert_notequal_0(x) CI_ASSERT(!(x), "audio_capture_assert_notequal_0\n")

//block的数量
#define CAPTURE_BLOCK_NUM (2)

//如果不可以使用malloc，block size的大小固定，需要通过修改宏定义修改
#if !CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
#warning "audio capture used static malloc"

#ifndef AUDUI_CAPTURE_NO_USE_MALLOC_BLOCK_SIZE
#error "please give me a block size!!!"
#endif
//不能使用malloc的时候，blocksize只能固定，用户不能设置（大小为byte）
#define CAPTURE_BLOCK_SIZE (AUDUI_CAPTURE_NO_USE_MALLOC_BLOCK_SIZE)
#endif

#define IIS_CAP_IPCORE_CLK get_ipcore_clk()
#define IIS_CAP_OSC_CLK get_osc_clk()
#define IIS_CAP_PLL_CLK get_iispll_clk()
/*******************************************************************************
                    extern
*******************************************************************************/

/*******************************************************************************
                    global
*******************************************************************************/
//可以使用malloc
#if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
//buffer的指针，每一组IIS使用一个buf，没用的不会molloc。
int16_t *audio_cap_buf[MAX_MULTI_CODEC_NUM] = {NULL};
#else //不可以使用malloc
//在不能使用malloc的情况下，数据缓存在全局数组中
#if AUDIO_CAPTURE_USE_MULTI_CODEC
//使用多codec录音
#pragma location = ".no_cache_sram"
__no_init uint8_t audio_cap_buf_array[MAX_MULTI_CODEC_NUM][CAPTURE_BLOCK_SIZE * CAPTURE_BLOCK_NUM];
#else
//使用单codec
#pragma location = ".no_cache_sram"
uint8_t audio_cap_buf_array[1][CAPTURE_BLOCK_SIZE * CAPTURE_BLOCK_NUM];
#endif
#endif

//注册设备时，设备信息的存储
capture_equipment_state_t capture_equipment_state_str[MAX_MULTI_CODEC_NUM];

//目前是否有设备处于直通VAD的模式（只允许一种设备直通）
static bool sg_is_codec_straight_to_vad = false;
//当前处于直通模式的codec的编号
// static audio_cap_codec_num_t sg_straght_mode_codec_num = AUDIO_CAP_CODEC_NULL;

/*管理multi codec的全局变量*/
//是否处于多个codec同时使用的情况
static bool sg_is_multi_codec = false;

#if AUDIO_CAPTURE_USE_MULTI_CODEC                                                           //使用多个CODEC同时录音
static audio_cap_codec_num_t multi_codec_num[MAX_MULTI_CODEC_NUM] = {AUDIO_CAP_CODEC_NULL}; //多个codec正在同时工作的编号
static bool multi_codec_irq_state[MAX_MULTI_CODEC_NUM] = {false, false};                    //多通道时，判断各个通道的数据是否已经准备好
#endif

//发送的语音数据队列
QueueHandle_t Audio_Cap_xQueue = NULL;

//记录ALC的目前的模式
static audio_codec_alc_mode current_alc_mode;

static uint32_t sg_iis_clk_src_fre = 0;//IIS时钟源的频率，有IPcore、IISPLL或者直接来源于晶振的可能

/*******************************************************************************
                    static function
*******************************************************************************/



/**
 * @brief 通过codec编号找到对应的iis编号
 * 
 * @param codec_num codec编号
 * @return audio_cap_iis_num_t iis编号
 */
#if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC 
static audio_cap_iis_num_t get_iisnum_from_codecnum(uint16_t codec_num)
{
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;
    audio_capture_assert_notequal_0(codec_num == AUDIO_CAP_CODEC_NULL);
    for (int i = 0; i < MAX_MULTI_CODEC_NUM; i++)
    {
        if (codec_num == capture_equipment_state_str[i].codec_num)
        {
            iis_num = (audio_cap_iis_num_t)(i + 1);
            break;
        }
    }
    return iis_num;
}
#endif

/**
 * @brief 录音设备codec初始化
 * 
 * @param regstr audio_capture_init_t类型结构体指针
 */
static void audio_cap_codec_init(const audio_capture_init_t *regstr)
{
    regstr->audio_capture_codec_init(regstr->cha_num, regstr->sample_rate,
                                     regstr->bit_wide, regstr->codec_mode, regstr->data_format);
}

/**
 * @brief IIS初始化为中断模式
 * 
 * @param iis_num IIS号
 * @param block_size 
 * @param cha_num 
 * @param codec_mode 
 * @param data_format 
 */
static void audio_cap_iis_init(audio_cap_iis_num_t iis_num, audio_cap_block_size_t block_size,
                               audio_cap_channel_num_t cha_num, audio_cap_codec_mode_t codec_mode,
                               audio_cap_data_format_t data_format, audio_cap_sample_rate_t sample_rate,
                               audio_cap_over_sample_t over_sample, audio_cap_clk_source_t clk_source,
                               audio_cap_bit_wide_t bit_wide,audio_cap_sck_lrck_rate_t sck_lrck_rate)
{
    IISNUMx iis_clk_num = IISNum1;
    IIS_TypeDef* IISx = IIS1;
    IOResue_FUNCTION IISx_IO_reuse = SECOND_FUNCTION;
    IRQn_Type IISDMAx = IIS_DMA0_IRQn;
    iis_bus_sck_lrckx_t iis_sck_lrck = IIS_BUSSCK_LRCK64;
    uint32_t mclk_base_addr = HAL_MCLK1_BASE;

    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    iis_clk_num = IISNum3;
    IISx = IIS3;
    mclk_base_addr = HAL_MCLK3_BASE;
        #if INNER_CODEC_ADC_USE_PROBE//探针模式
        IISx_IO_reuse = THIRD_FUNCTION;
        Scu_SetIOReuse(I2S1_MCLK_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDI_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDO_PAD,IISx_IO_reuse);
        #endif
    #else
    if(AUDIO_CAP_IIS1 == iis_num)
    {
        iis_clk_num = IISNum1;
        IISx = IIS1;
        IISx_IO_reuse = SECOND_FUNCTION;
        mclk_base_addr = HAL_MCLK1_BASE;
		Scu_SetIOReuse(I2S1_MCLK_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDI_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDO_PAD,IISx_IO_reuse);
    }
    else if(AUDIO_CAP_IIS3 == iis_num)
    {
        iis_clk_num = IISNum3;
        IISx = IIS3;
        #if INNER_CODEC_ADC_USE_PROBE//探针模式
        IISx_IO_reuse = THIRD_FUNCTION;
		Scu_SetIOReuse(I2S1_MCLK_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDI_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDO_PAD,IISx_IO_reuse);
        #endif
        mclk_base_addr = HAL_MCLK3_BASE;
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
    #endif
 

    /*打开MCLK的时钟并配置分频*/
    iis_clk_source_sel_t mclk_sor_sel = IIS_CLK_SOURCE_FROM_IPCORE;
    Scu_SetDeviceGate((uint32_t)IISx,DISABLE);
    Scu_SetDeviceGate(mclk_base_addr,DISABLE);

    uint32_t clk = IIS_CAP_IPCORE_CLK;
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    if(AUDIO_CAP_CLK_SOURCE_OSC == clk_source)
    {
        clk = IIS_CAP_OSC_CLK;
        mclk_sor_sel = IIS_CLK_SOURCE_FROM_OSC;
    }
    else if(AUDIO_CAP_CLK_SOURCE_DIVIDER_IPCORE == clk_source)
    {
        clk = IIS_CAP_IPCORE_CLK;
        mclk_sor_sel = IIS_CLK_SOURCE_FROM_IPCORE;
    }
    else
    {
        CI_ASSERT(0,"para_err\n");
    }
    #endif

    
    //SCK和LRCK的比值
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    iis_bus_sck_lrckx_t sck_lrck_rate_iis = IIS_BUSSCK_LRCK64;
    if( AUDIO_CAP_SCK_LRCK_64 == sck_lrck_rate)
    {
        sck_lrck_rate_iis = IIS_BUSSCK_LRCK64;
        iis_sck_lrck = IIS_BUSSCK_LRCK64;
    }
    else
    {
        sck_lrck_rate_iis = IIS_BUSSCK_LRCK32;
        iis_sck_lrck = IIS_BUSSCK_LRCK32;
    }
    #else
    iis_bus_sck_lrckx_t sck_lrck_rate_iis = IIS_BUSSCK_LRCK64;
    #endif

    //过采样率设置
    #if !AUDIO_CAPTURE_USE_CONST_PARA
	iis_oversample_t iis_oversample = IIS_OVERSAMPLE_128Fs;
	if (AUDIO_CAP_OVER_SAMPLE_128 == over_sample)
	{
		iis_oversample = IIS_OVERSAMPLE_128Fs;
	}
	else if(AUDIO_CAP_OVER_SAMPLE_192 == over_sample)
	{
		iis_oversample = IIS_OVERSAMPLE_192Fs;
	}
	else if(AUDIO_CAP_OVER_SAMPLE_256 == over_sample)
	{
		iis_oversample = IIS_OVERSAMPLE_256Fs;
	}
	else if(AUDIO_CAP_OVER_SAMPLE_384 == over_sample)
	{
		iis_oversample = IIS_OVERSAMPLE_384Fs;
	}
	else
	{
		audio_capture_assert_equal_0(0);
	}
    #else
    iis_oversample_t iis_oversample = IIS_OVERSAMPLE_256Fs;
    #endif

    //主从模式设置
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    iis_mode_sel_t iis_mode = IIS_MASTER;
    if(AUDIO_CAP_CODEC_MASTER == codec_mode)
    {
        iis_mode = IIS_SLAVE;
    }
    #else
    iis_mode_sel_t iis_mode = IIS_MASTER;
    #endif

    //配置IIS时钟
    IIS_CLK_CONFIGTypeDef cap_iis_clk_str;
    memset(&cap_iis_clk_str,0,sizeof(IIS_CLK_CONFIGTypeDef));

    cap_iis_clk_str.device_select = iis_clk_num;
    float div_f = (float)clk/(float)sample_rate/(float)over_sample;
    div_f = div_f + 0.5f;
    cap_iis_clk_str.div_param = (int8_t)(div_f);
    cap_iis_clk_str.model_sel = iis_mode;
    cap_iis_clk_str.sck_ext_inv_sel = IIS_SCK_EXT_NO_INV;
    cap_iis_clk_str.sck_wid = iis_sck_lrck;
    cap_iis_clk_str.over_sample = iis_oversample;
    cap_iis_clk_str.clk_source = mclk_sor_sel;
    cap_iis_clk_str.mclk_out_en = IIS_MCLK_OUT_ENABLE;

    Scu_SetIISCLK_Config(&cap_iis_clk_str);
    Scu_SetDeviceGate((uint32_t)IISx,ENABLE);
    Scu_SetDeviceGate(mclk_base_addr,ENABLE);
    IIS_DMA_RXInit_Typedef IISDMARX_Init_Struct = {0};

    eclic_irq_enable(IISDMAx);
    //IISDMA数据格式设置
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    IIS_TXRXDATAFMTx iis_data_format = IIS_RXDATAFMT_IIS;
    if(AUDIO_CAP_DATA_FORMAT_LEFT_JUSTIFY == data_format)
    {
        iis_data_format = IIS_RXDATAFMT_LEFT_JUSTIFIED;
    }
    else if(AUDIO_CAP_DATA_FORMAT_RIGHT_JUSTIFY == data_format)
    {
        iis_data_format = IIS_RXDATAFMT_RIGHT_JUSTIFIED;
    }
    
    //采样深度
    IIS_TXRXDATAWIDEx data_wide = IIS_TXRXDATAWIDE16BIT;
    if(AUDIO_CAP_BIT_WIDE_16 == bit_wide)
    {
        data_wide = IIS_TXRXDATAWIDE16BIT;
    }
    else if(AUDIO_CAP_BIT_WIDE_24 == bit_wide)
    {
        data_wide = IIS_TXRXDATAWIDE24BIT;
    }
    else
    {
        audio_capture_assert_equal_0(0);
    }
    #else
    IIS_TXRXDATAFMTx iis_data_format = IIS_RXDATAFMT_IIS;
    IIS_TXRXDATAWIDEx data_wide = IIS_TXRXDATAWIDE16BIT;
    #endif
    

    uint32_t buf_addr = capture_equipment_state_str[iis_num - 1].buf_addr;
//#if !CONFIG_DIRVER_BUF_USED_FREEHEAP_EN                                    //不可以使用malloc
    // uint32_t buf_addr = capture_equipment_state_str[iis_num - 1].buf_addr; //buf首地址
    IISDMARX_Init_Struct.rxaddr = buf_addr;
//#else
    //IISDMARX_Init_Struct.rxaddr = (uint32_t)audio_cap_buf[iis_num - 1];
//#endif
    IISDMARX_Init_Struct.rollbackaddrsize = (IISDMA_RXTXxRollbackADDR)(block_size / 128 * CAPTURE_BLOCK_NUM - 1);
    IISDMARX_Init_Struct.rxinterruptsize = (IISDMA_RXxInterrupt)(block_size / 128 - 1);//IISDMA_RX32Interrupt;
    IISDMARX_Init_Struct.rxsinglesize = IISDMA_TXRXSINGLESIZE128bytes;//IISDMA_TXRXSINGLESIZE16bytes;
    IISDMARX_Init_Struct.rxdatabitwide = data_wide;
    IISDMARX_Init_Struct.sck_lrck = sck_lrck_rate_iis;
    IISDMARX_Init_Struct.rxdatafmt = iis_data_format;


    //iisdma搬运的数据选择
    IIS_RXMODEx iis_rxmode = IIS_RXMODE_STEREO;
    IIS_LR_SELx iis_lr_sel = IIS_LR_RIGHT_HIGH_LEFT_LOW;
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    if( AUDIO_CAP_CHANNEL_STEREO != cha_num)
    {
        iis_rxmode = IIS_RXMODE_MONO;
        if(AUDIO_CAP_CHANNEL_RIGHT == cha_num)
        {
            iis_lr_sel = IIS_LR_LEFT_HIGH_RIGHT_LOW;
        }
    }
    #endif
    IIS_RXMODEConfig(IISx,IIS_MERGE_16BITTO32BIT,iis_lr_sel,iis_rxmode);//CODEC一般情况
    
    IISx_RXInit_noenable(IISx,&IISDMARX_Init_Struct);
}






/*******************************************************************************
                    function
*******************************************************************************/
/**
 * @brief 注册采音设备。
 * 建立某个codec与IIS号，block size、buf地址以及IIS看门狗回调函数的对应关系。
 * 
 * @param regstr audio_capture_init_t结构体指针
 * @return int8_t 
 */
int8_t audio_capture_registe(const audio_capture_init_t *regstr)
{
    audio_cap_codec_num_t codec_num = regstr->codecx;
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;

    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    //强制建立codec与IIS的对应关系
    if (AUDIO_CAP_INNER_CODEC == codec_num)
    {
        iis_num = AUDIO_CAP_IIS3;
    }
    else
    {
        iis_num = AUDIO_CAP_IIS1;
    }
    //设备注册
    audio_capture_assert_notequal_0(AUDIO_CAP_CODEC_NULL != capture_equipment_state_str[iis_num - 1].codec_num); //判断是否有设备已经注册
    #endif

    capture_equipment_state_str[iis_num - 1].codec_num = codec_num;
    capture_equipment_state_str[iis_num - 1].block_size = regstr->block_size;
    capture_equipment_state_str[iis_num - 1].iis_watchdog_callback = regstr->iis_watchdog_callback;
    capture_equipment_state_str[iis_num - 1].over_sample = regstr->over_sample;
    capture_equipment_state_str[iis_num - 1].clk_source = regstr->clk_source;

#if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
    //为接收语音的buf分配空间,一个codec也只能一次
    audio_cap_buf[iis_num - 1] = (int16_t *)malloc(regstr->block_size * CAPTURE_BLOCK_NUM);
    audio_capture_assert_equal_0(audio_cap_buf[iis_num - 1]);
    capture_equipment_state_str[iis_num - 1].buf_addr = (uint32_t)audio_cap_buf[iis_num - 1];
#else
    capture_equipment_state_str[iis_num - 1].block_size = (audio_cap_block_size_t)CAPTURE_BLOCK_SIZE;  //如果使用数组当buf，blocksize不变

#if AUDIO_CAPTURE_USE_MULTI_CODEC //如果使用两个codec
    capture_equipment_state_str[iis_num - 1].buf_addr = (uint32_t)&audio_cap_buf_array[iis_num - 1]; //buf首地址
#else                             //如果只使用单个codec
    capture_equipment_state_str[iis_num - 1].buf_addr = (uint32_t)&audio_cap_buf_array[0]; //buf首地址
#endif

#endif

    return RETURN_OK;
}


/**
 * @brief 采音设备的初始化，codec初始化和IIS初始化
 * 
 * @param regstr audio_capture_init_t结构体指针
 * @return int8_t 
 */
int8_t audio_capture_init(const audio_capture_init_t *regstr)
{
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;
    audio_cap_block_size_t block_size = regstr->block_size;
    audio_cap_channel_num_t cha_num = regstr->cha_num;
    audio_cap_codec_num_t codec_num = regstr->codecx;
    audio_cap_codec_mode_t codec_mode = regstr->codec_mode;
    audio_cap_data_format_t data_format = regstr->data_format;
    audio_cap_sample_rate_t sample_rate = regstr->sample_rate;
    audio_cap_over_sample_t over_sample = regstr->over_sample;
    audio_cap_clk_source_t clk_source = regstr->clk_source;
    audio_cap_bit_wide_t bit_wide = regstr->bit_wide;
    audio_cap_sck_lrck_rate_t sck_lrck_rate = regstr->sck_lrck_rate;
    uint8_t is_codec_init = 0;

    //通过codec_num匹配到iis_num
    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    iis_num = get_iisnum_from_codecnum(codec_num);
    audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
    #endif

    is_codec_init = capture_equipment_state_str[iis_num - 1].is_codec_init;

    //初始化codec
    if (0 == is_codec_init)
    {
        //如果不使用malloc，不为buffer动态分配内存
        audio_cap_codec_init(regstr);
        capture_equipment_state_str[iis_num - 1].is_codec_init++;
    }

    if (AUDIO_TO_VAD_STRAIGHT == regstr->is_straight_to_vad)
    {
        //判断是否有设备已经处于直通模式，且直通此次初始化为直通模式的codec不是之前的直通模式CODEC
        /*audio_capture_assert_notequal_0((true == sg_is_codec_straight_to_vad) && (codec_num != sg_straght_mode_codec_num));
        sg_is_codec_straight_to_vad = true;
        sg_straght_mode_codec_num = codec_num;
        audio_cap_iis_straight_to_vad_init(iis_num, block_size, cha_num, codec_mode, data_format, sample_rate,
                                           over_sample, clk_source,
                                           regstr->addr_pcm_buf_for_straight, regstr->size_pcm_buf_for_straight);*/
    	audio_capture_assert_equal_0(0);
    }
    else
    {
#if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN //可以使用malloc
        audio_cap_iis_init(iis_num, block_size, cha_num, codec_mode, data_format, sample_rate, over_sample, clk_source,bit_wide,sck_lrck_rate);
#else //如不可使用malloc，则blocksize固定
        audio_cap_iis_init(iis_num, (audio_cap_block_size_t)CAPTURE_BLOCK_SIZE, cha_num, codec_mode, 
                        data_format, sample_rate, over_sample, clk_source,bit_wide,sck_lrck_rate);
#endif
        // if ((true == sg_is_codec_straight_to_vad) && (sg_straght_mode_codec_num == codec_num))
        // {
        //     sg_is_codec_straight_to_vad = false;
        //     sg_straght_mode_codec_num = AUDIO_CAP_CODEC_NULL;
        // }
    }

    //申请队列，只运行一次
    if(NULL == Audio_Cap_xQueue)
    {
        Audio_Cap_xQueue = xQueueCreate(CAPTURE_BLOCK_NUM, sizeof(audio_getdate_t));
        audio_capture_assert_equal_0(Audio_Cap_xQueue);
    }

    return RETURN_OK;
}

/**
 * @brief 为inner CODEC编写的初始化函数
 * 
 * @param cha_num 通道选择
 * @param sample_rate 过采样率设置
 * @param bit_wide 数据宽度
 * @param codec_mode 主从模式
 * @param data_format 数据格式
 */
void inner_codec_init(audio_cap_channel_num_t cha_num, audio_cap_sample_rate_t sample_rate,
                      audio_cap_bit_wide_t bit_wide, audio_cap_codec_mode_t codec_mode,
                      audio_cap_data_format_t data_format)
{
    audio_cap_sample_rate_t audio_cap_sr[8] = {
        AUDIO_CAP_SAMPLE_RATE_8K,
        AUDIO_CAP_SAMPLE_RATE_12K,
        AUDIO_CAP_SAMPLE_RATE_16K, //采样率
        AUDIO_CAP_SAMPLE_RATE_24K,
        AUDIO_CAP_SAMPLE_RATE_32K,
        AUDIO_CAP_SAMPLE_RATE_44_1K,
        AUDIO_CAP_SAMPLE_RATE_48K,
        AUDIO_CAP_SAMPLE_RATE_96K,
    };
    inner_codec_samplerate_t codec_sr_arry[8] = {INNER_CODEC_SAMPLERATE_8K,
                                                 INNER_CODEC_SAMPLERATE_12K,
                                                 INNER_CODEC_SAMPLERATE_16K,
                                                 INNER_CODEC_SAMPLERATE_24K,
                                                 INNER_CODEC_SAMPLERATE_32K,
                                                 INNER_CODEC_SAMPLERATE_44_1K,
                                                 INNER_CODEC_SAMPLERATE_48K,
                                                 INNER_CODEC_SAMPLERATE_96K};

    audio_cap_channel_num_t codec_cha_sel = cha_num;

    inner_codec_samplerate_t codec_sr = INNER_CODEC_SAMPLERATE_16K; //为codec设置的采样率
    inner_codec_valid_word_len_t valid_word_len = INNER_CODEC_VALID_LEN_16BIT;
    for (int i = 0; i < 8; i++) //传递capture的采样率到CODEC
    {
        if (sample_rate == audio_cap_sr[i])
        {
            codec_sr = codec_sr_arry[i];
            break;
        }
    }
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    switch (bit_wide) //匹配数据位宽
    {
    case AUDIO_CAP_BIT_WIDE_16:
    {
        valid_word_len = INNER_CODEC_VALID_LEN_16BIT;
        break;
    }
    case AUDIO_CAP_BIT_WIDE_24:
    {
        valid_word_len = INNER_CODEC_VALID_LEN_24BIT;
        break;
    }
    default:
        audio_capture_assert_equal_0(0);
        break;
    }
    #endif

    //主从模式设置
    inner_codec_mode_t mode = INNER_CODEC_MODE_SLAVE;
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    if (AUDIO_CAP_CODEC_MASTER == codec_mode)
    {
        mode = INNER_CODEC_MODE_MASTER;
    }
    #endif

    //匹配数据格式
    inner_codec_i2s_data_famat_t format = INNER_CODEC_I2S_DATA_FORMAT_I2S_MODE;
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    if (AUDIO_CAP_DATA_FORMAT_LEFT_JUSTIFY == data_format)
    {
        format = INNER_CODEC_I2S_DATA_FORMAT_LJ_MODE;
    }
    else if (AUDIO_CAP_DATA_FORMAT_RIGHT_JUSTIFY == data_format)
    {
        format = INNER_CODEC_I2S_DATA_FORMAT_RJ_MODE;
    }
    else if (AUDIO_CAP_DATA_FORMAT_STANDARD_I2S == data_format)
    {
        format = INNER_CODEC_I2S_DATA_FORMAT_I2S_MODE;
    }
    else
    {
        CI_ASSERT(0, "data_format err!\n");
    }
    #endif
    inner_codec_adc_mode_set(mode, INNER_CODEC_FRAME_LEN_32BIT, valid_word_len, format);

    /*打开模块内部的高通滤波器*/
    inner_codec_hightpass_config(INNER_CODEC_GATE_ENABLE, INNER_CODEC_HIGHPASS_CUT_OFF_20HZ);

    inner_codec_adc_config_t CODEC_ADC_Config_Struct;

    CODEC_ADC_Config_Struct.codec_adcl_mic_amp = INNER_CODEC_MIC_AMP_12dB;

    //如果不使能ALC，PGA的增益设置
#if 0//USE_CWSL
    CODEC_ADC_Config_Struct.pga_gain_l = 15.0f;//28.5f;
#else
    CODEC_ADC_Config_Struct.pga_gain_l = 28.5f;
#endif
    //ADC初始化
    inner_codec_adc_enable(&CODEC_ADC_Config_Struct);

    //根据传入的参数，判断使能哪个通道的ADC
    
    inner_cedoc_gate_t codec_left_adc_gate = INNER_CODEC_GATE_DISABLE;
    inner_cedoc_gate_t codec_right_adc_gate = INNER_CODEC_GATE_ENABLE;
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    if (AUDIO_CAP_CHANNEL_STEREO == codec_cha_sel)
    {
        codec_left_adc_gate = INNER_CODEC_GATE_DISABLE;
        codec_right_adc_gate = INNER_CODEC_GATE_DISABLE;
    }
    else if (AUDIO_CAP_CHANNEL_LEFT == codec_cha_sel)
    {
        codec_left_adc_gate = INNER_CODEC_GATE_DISABLE;
        codec_right_adc_gate = INNER_CODEC_GATE_ENABLE;
    }
    else if (AUDIO_CAP_CHANNEL_RIGHT == codec_cha_sel)
    {
        codec_left_adc_gate = INNER_CODEC_GATE_ENABLE;
        codec_right_adc_gate = INNER_CODEC_GATE_DISABLE;
    }
    else
    {
        CI_ASSERT(0, "codec_cha_sel err!\n");
    }
    inner_codec_adc_disable(INNER_CODEC_LEFT_CHA, codec_left_adc_gate);   //ADC左通道开关
    #endif

    inner_codec_alc_use_config_t ALC_USE_str;
    memset(&ALC_USE_str, 0, sizeof(inner_codec_alc_use_config_t));
    ALC_USE_str.alc_maxgain = INNER_CODEC_ALC_PGA_MAX_GAIN_28_5dB; //INNER_CODEC_ALC_PGA_MAX_GAIN_28_5dB;
    ALC_USE_str.alc_mingain = INNER_CODEC_ALC_PGA_MIN_GAIN_12dB;//INNER_CODEC_ALC_PGA_MIN_GAIN__6dB;
    ALC_USE_str.max_level = INNER_CODEC_ALC_LEVEL__1_5dB; //7.5f;//6;//-7.5以上，音量很大就会削顶
    ALC_USE_str.min_level = INNER_CODEC_ALC_LEVEL__3dB; //9.0f;//7.5;//
    ALC_USE_str.sample_rate = codec_sr;
    ALC_USE_str.use_ci112x_alc = INNER_CODEC_GATE_DISABLE; //CI112X芯片自带的ALC开关

    ALC_USE_str.alc_gate = INNER_CODEC_GATE_ENABLE; //inner CODEC ALC开关
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    ALC_USE_str.attack_time = INNER_CODEC_ALC_ATTACK_TIME_64MS;
    ALC_USE_str.decay_time = INNER_CODEC_ALC_DECAY_TIME_500US;//INNER_CODEC_ALC_DECAY_TIME_2MS;
    ALC_USE_str.hold_time = INNER_CODEC_ALC_HOLD_TIME_4MS;
    ALC_USE_str.noise_gate = INNER_CODEC_GATE_ENABLE;
    ALC_USE_str.fast_decrece_87_5 = INNER_CODEC_GATE_DISABLE;
    ALC_USE_str.zero_cross = INNER_CODEC_GATE_ENABLE;
    ALC_USE_str.noise_gate_threshold = INNER_CODEC_NOISE_GATE_THRE_81dB;//INNER_CODEC_NOISE_GATE_THRE_45dB;
    #endif
    ALC_USE_str.digt_gain = 197 + 8; //数字增益

    inner_codec_alc_left_config(&ALC_USE_str);  //ALC左通道初始化
    //inner_codec_alc_right_config(&ALC_USE_str); //ALC右通道初始化
}




static void audio_capture_send_msg(audio_getdate_t* data_msg,BaseType_t *xHigherPriorityTaskWoken)
{
    if (pdPASS != xQueueSendToBackFromISR(Audio_Cap_xQueue, data_msg, xHigherPriorityTaskWoken))
    {
        //如果发送错误，表示接收队列太多了，那就先接收一下，这种操作会丢掉一帧 数据，打印一次丢掉一帧
        audio_getdate_t fake_msg;
        xQueueReceiveFromISR(Audio_Cap_xQueue, &fake_msg, 0);
        xQueueSendToBackFromISR(Audio_Cap_xQueue, data_msg, xHigherPriorityTaskWoken);
        ci_logdebug(LOG_VOICE_CAPTURE,"audio get too slow\n");
    }
}

/**
 * @brief 采音标准接口使用的中断处理函数
 * 
 * @param iis_num IIS编号
 * @param IISDMA IISDMA0或者IISDMA1
 * @param iischa IISChax枚举类型
 */
void audio_cap_handler(const audio_cap_iis_num_t iis_num, IISDMA_TypeDef *IISDMA, IISChax iischa, int8_t* is_addr_rollback, BaseType_t *xHigherPriorityTaskWoken)
{
    audio_cap_block_size_t block_size = AUDIO_CAP_BLOCK_SIZE_640;
    audio_cap_codec_num_t codec_num = AUDIO_CAP_CODEC_NULL;

    //找到iis对应的block
    block_size = (audio_cap_block_size_t)capture_equipment_state_str[iis_num - 1].block_size;
    codec_num = (audio_cap_codec_num_t)capture_equipment_state_str[iis_num - 1].codec_num;
//#if !CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
    uint32_t buf_addr = capture_equipment_state_str[iis_num - 1].buf_addr;
//#endif

    static unsigned int count[MAX_MULTI_CODEC_NUM] = {0};
    static audio_getdate_t audio_data_msg;

    // if(*is_addr_rollback != 0)//地址切换中断到来，
    // {
    //     CAPTURE_BLOCK_NUM
    // }

    uint32_t addr = 0;

    //从IISDMA寄存器中读取当前中断，IIS RX addr
    //mprintf("rx = %08x\n",IISxDMA_RXADDR(IISDMA, iischa));//*(uint32_t*)((uint32_t)IISDMA0 + 0x64));
    uint32_t rx_hardware_last_addr = IISxDMA_RXADDR(IISDMA, iischa);

//    timer0_end_count_only_print_time_us();
//	init_timer0();
//    timer0_start_count();

    uint32_t current_addr = 0;
    #if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
    current_addr = buf_addr + block_size * count[iis_num - 1];
    #else
    current_addr = buf_addr + CAPTURE_BLOCK_SIZE * count[iis_num - 1];
    #endif

    //mprintf("current_addr = %08x\n",current_addr);

    //发送的地址（CPU以为的已经接受好的buffer地址）和硬件IISDMA正在搬运的buffer地址一样了，
    //表示中断到来的次数（CPU处理中断的速度），比硬件IISDMA慢。
    //这个时候就需要将发送的消息里面的buffer退一个
    if((rx_hardware_last_addr + 4) == current_addr)
    {
        for(int i=0;i<MAX_MULTI_CODEC_NUM;i++)
        {
            count[i] = count[i] + CAPTURE_BLOCK_NUM - 1;
            count[i] = count[i] % CAPTURE_BLOCK_NUM;
        }

        #if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
        current_addr = buf_addr + block_size * count[iis_num - 1];
        #else
        current_addr = buf_addr + CAPTURE_BLOCK_SIZE * count[iis_num - 1];
        #endif
    }

    count[iis_num - 1]++;
    count[iis_num - 1] = count[iis_num - 1] % CAPTURE_BLOCK_NUM;

#if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
    //addr = (uint32_t)audio_cap_buf[iis_num - 1] + block_size * count[iis_num - 1];
#else
    //addr = buf_addr + CAPTURE_BLOCK_SIZE * count[iis_num - 1];
#endif

    //IISxDMA_RADDR(IISDMA, iischa, addr);

    if (true == sg_is_multi_codec) //双通道的处理方法
    {
#if AUDIO_CAPTURE_USE_MULTI_CODEC
        for (int i = 0; i < MAX_MULTI_CODEC_NUM; i++) //找到multi_codec里面的是否有次codec
        {
            if (codec_num == multi_codec_num[i])
            {
                //某个通道接收完成的标志置位
                multi_codec_irq_state[i] = true;
                if (0 == i)
                {                                                    //inner codec设备,从audio_cap_codec_num_t枚举类型的排序中体现先后顺序
                    audio_data_msg.codec_num = codec_num;            //codec号
                    audio_data_msg.current_data_addr = current_addr; //当前有效的数据的起始地址
                    audio_data_msg.iis_num = iis_num;                //IIS号，指示是哪个IIS收到的数据
                    audio_data_msg.block_size = block_size;
                    audio_data_msg.alc_mode = current_alc_mode;
                }
                else if (1 == i)
                {                                                      //其他设备
                    audio_data_msg.codec_num_2 = codec_num;            //codec号
                    audio_data_msg.current_data_addr_2 = current_addr; //当前有效的数据的起始地址
                    audio_data_msg.iis_num_2 = iis_num;                //IIS号，指示是哪个IIS收到的数据
                    audio_data_msg.block_size = block_size;
                    audio_data_msg.alc_mode = current_alc_mode;
                    if(audio_data_msg.alc_mode != AUDIO_CODEC_ALC_NORMAL)
                    {
                        //mprintf("alc_mode is:%d\n",audio_data_msg.alc_mode);
                    }
                }
                break;
            }
        }
        if ((true == multi_codec_irq_state[0]) && (true == multi_codec_irq_state[1])) //两个通道的数据都准备好了
        {
            //接收完成标志清空，并发送数据队列
            multi_codec_irq_state[0] = false;
            multi_codec_irq_state[1] = false;
            audio_capture_send_msg(&audio_data_msg,xHigherPriorityTaskWoken);
        }
#endif
    }
    else //单通道的处理方法
    {
        audio_data_msg.current_data_addr = current_addr; //当前有效的数据的起始地址
        audio_data_msg.iis_num = iis_num;                //IIS号，指示是哪个IIS收到的数据
        audio_data_msg.codec_num = codec_num;            //codec号
        audio_data_msg.block_size = block_size;
        audio_data_msg.alc_mode = current_alc_mode;
        if(audio_data_msg.alc_mode != AUDIO_CODEC_ALC_NORMAL)
        {
          //mprintf("alc_mode is:%d\n",audio_data_msg.alc_mode);
        }
#if AUDIO_CAPTURE_USE_MULTI_CODEC
        //如果使能了两个CODEC，但是只使用单个codec录音，另个通道的信息需要填填一下
        audio_data_msg.codec_num_2 = AUDIO_CAP_CODEC_NULL; //codec号
        audio_data_msg.current_data_addr_2 = 0;            //当前有效的数据的起始地址
        audio_data_msg.iis_num_2 = AUDIO_CAP_IIS_NULL;     //IIS号，指示是哪个IIS收到的数据
#endif
        audio_capture_send_msg(&audio_data_msg,xHigherPriorityTaskWoken);
    }
}

/**
 * @brief 获取音频数据。通过接受IISDMA RX中断发送的消息，获取采音数据，有三种类
 * 型（block、非block、超时）三种方式获取采音数据。
 * 
 * @param type 使用何种方式获取数据（block、unblock、timeout）
 * @param time 最长的超时时间设置
 * @return audio_getdate_t 
 */
audio_getdate_t audio_capture_getdata(audio_cap_data_get_t type, uint32_t time)
{
    audio_getdate_t audio_data_msg;

    //三种不同的方式获取数据
    if (AUDIO_CAP_DATA_GET_UNBLOCK == type) //非阻塞方式
    {
        if (pdPASS == xQueueReceive(Audio_Cap_xQueue, &audio_data_msg, 0))
        {
            audio_data_msg.data_state = AUDIO_CAP_DATA_GET_STATE_YES;
        }
        else //目前没有消息
        {
            audio_data_msg.data_state = AUDIO_CAP_DATA_GET_STATE_NODATA;
        }
    }
    else if (AUDIO_CAP_DATA_GET_BLOCK == type) //阻塞方式
    {
        xQueueReceive(Audio_Cap_xQueue, &audio_data_msg, pdMS_TO_TICKS(portMAX_DELAY));
        audio_data_msg.data_state = AUDIO_CAP_DATA_GET_STATE_YES;
    }
    else //timeout模式
    {
        if (pdPASS == xQueueReceive(Audio_Cap_xQueue, &audio_data_msg, pdMS_TO_TICKS(time)))
        {
            audio_data_msg.data_state = AUDIO_CAP_DATA_GET_STATE_YES;
        }
        else //超时了，依然没有消息
        {
            audio_data_msg.data_state = AUDIO_CAP_DATA_GET_STATE_TIMEOUT;
        }
    }

    return audio_data_msg;
}

/**
 * @brief codec开始采音,根据选择的CODEC设备编号，打开IIS 和IISDMA RX使能开始录音。
 * 支持多个codec同时打开，例如，如果你想同时开启inner_codec和8388，那传入的参数是
 * 这样的：AUDIO_CAP_OUTSIDE_CODEC|AUDIO_CAP_INNER_CODEC。
 * @param codec_num codec编号
 */
void audio_cap_start(uint16_t codec_num)
{
    #if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    IIS_TypeDef *IISx[2] = {IIS1, IIS3}; //不是写错了，就是这样的
    #endif

    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;

#if AUDIO_CAPTURE_USE_MULTI_CODEC
    bool is_codec_single = false;
#endif

    //通过codec_num匹配到iis_num
    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    iis_num = get_iisnum_from_codecnum(codec_num);
    #endif

#if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    IISx_RX_enable_int(IIS3);
#else
    if (AUDIO_CAP_IIS_NULL != iis_num) //传入的参数不止一个codec
    {
        //是否为直通
        if (sg_straght_mode_codec_num != codec_num)
        {
            //中断模式使能IIS
            IISx_RX_enable_int(IISx[iis_num - 1]);
        }
        else
        { //直通模式使能IIS
            audio_capture_assert_notequal_0(1); //只用一个IIS的时候，却使能了多组codec，不允许！
        }
        #if AUDIO_CAPTURE_USE_MULTI_CODEC
        is_codec_single = true; //确实只有一个codec打开了
        #endif
    }
    else //传入的参数只有一个codec
    {
        #if !AUDIO_CAPTURE_USE_MULTI_CODEC
        audio_capture_assert_notequal_0(1); //只用一个IIS的时候，却使能了多组codec，不允许！
        #else
        is_codec_single = false;
        #endif
    }
#endif

#if AUDIO_CAPTURE_USE_MULTI_CODEC
    uint8_t multi_num = 0;

    //如果检测到传入的参数不止一个codec，多个codec开始采集的流程，考虑到两个codec需要同时开启
    if (false == is_codec_single)
    {
        audio_capture_assert_notequal_0(true == sg_is_multi_codec);
        uint8_t bit_cnt = 0;
        uint8_t max_bit_num = 0;
        while (1)
        {
            if ((codec_num >> max_bit_num) == 0)
            {
                break;
            }
            else
            {
                max_bit_num++;
            }
        }

        audio_cap_block_size_t block_size_tmp = AUDIO_CAP_BLOCK_SIZE_0;

        for (bit_cnt = 0; bit_cnt < max_bit_num; bit_cnt++) //最大的多少位，找多少次
        {
            if ((codec_num & (1 << bit_cnt)) != 0) //bit位不为0
            {
                audio_cap_codec_num_t standard_codec_num = (audio_cap_codec_num_t)(1 << bit_cnt);

                iis_num = get_iisnum_from_codecnum(standard_codec_num);
                audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
                IISx_RX_enable_int(IISx[iis_num - 1]);
                if (AUDIO_CAP_BLOCK_SIZE_0 != block_size_tmp) //不允许不同的block使用多个通道一起出数据的方式
                {
                    audio_capture_assert_notequal_0(block_size_tmp != capture_equipment_state_str[iis_num - 1].block_size);
                }
                block_size_tmp = (audio_cap_block_size_t)capture_equipment_state_str[iis_num - 1].block_size;

                multi_codec_num[multi_num] = standard_codec_num;
                multi_num++;
            }
        }
        sg_is_multi_codec = true;
        for (int i = 0; i < MAX_MULTI_CODEC_NUM; i++)
        {
            multi_codec_irq_state[i] = false;
        }
    }
    //超过了规定的CODEC使用数量
    audio_capture_assert_notequal_0(multi_num > MAX_MULTI_CODEC_NUM);
#endif
}

/**
 * @brief 结束采音。根据选择的CODEC设备编号，关闭IIS和IISDMA停止录音功能。
 * 
 * @param codec_num codec编号
 */
void audio_cap_stop(const audio_cap_codec_num_t codec_num)
{
    IIS_TypeDef *IISx[2] = {IIS1, IIS3}; //不是写错了，就是这样的
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;

    //通过codec_num匹配到iis_num
    #if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = get_iisnum_from_codecnum(codec_num);
    audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
    #else
    iis_num = AUDIO_CAP_IIS3;
    #endif

#if AUDIO_CAPTURE_USE_MULTI_CODEC
    //停止多codec同时使用的设备
    for (int j = 0; j < MAX_MULTI_CODEC_NUM; j++)
    {
        if (codec_num == multi_codec_num[j]) //如果次设备处于多codec模式
        {
            sg_is_multi_codec = false;
            for (int k = 0; k < MAX_MULTI_CODEC_NUM; k++)
            {
                multi_codec_num[k] = AUDIO_CAP_CODEC_NULL;
                multi_codec_irq_state[k] = false;
            }
            break;
        }
    }
#endif

    IISx_RX_disable(IISx[iis_num - 1]);
}

/**
 * @brief 采音静音（mute IIS）,mute之后，disable mute就可以恢复
 * 
 * @param codec_num codec编号
 * @param cmd 打开或者关闭
 */
void audio_cap_mute(const audio_cap_codec_num_t codec_num, FunctionalState cmd)
{
    IIS_TypeDef *IISx[2] = {IIS1, IIS3};
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;

    //通过codec_num匹配到iis_num
    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    iis_num = get_iisnum_from_codecnum(codec_num);
    audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
    #endif

    IIS_RX_MuteEN(IISx[iis_num - 1], cmd);
}

/**
 * @brief 采音设备删除
 * 
 * @param codec_num codec编号
 */
void audio_cap_delete(const audio_cap_codec_num_t codec_num)
{
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;

    //通过codec_num匹配到iis_num
    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    iis_num = get_iisnum_from_codecnum(codec_num);
    audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
    #endif

    //先停止采音
    audio_cap_stop(codec_num);
#if CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
    //释放空间
    if (NULL != audio_cap_buf[iis_num - 1])
    {
        free(audio_cap_buf[iis_num - 1]);
    }
#endif
    // //删除直通模式
    // if (codec_num == sg_straght_mode_codec_num) //如果此codec目前处于直通模式
    // {
    //     sg_is_codec_straight_to_vad = false;
    //     sg_straght_mode_codec_num = AUDIO_CAP_CODEC_NULL;
    // }

    //删除已经注册的设备全局变量
    capture_equipment_state_str[iis_num - 1].is_codec_init = 0;
    capture_equipment_state_str[iis_num - 1].codec_num = AUDIO_CAP_CODEC_NULL;
    // capture_equipment_iis[iis_num - 1] = 0;
    capture_equipment_state_str[iis_num - 1].block_size = AUDIO_CAP_BLOCK_SIZE_0;
}

/**
 * @brief 检测IIS是否异常的使能函数，
 * 根据选择的CODEC设备编号，使能IIS watchdog。注意如果使能IIS watchdog，需在注册
 * 的时候填相应的IIS watchdog的回调函数，用于IIS出现异常时的处理。
 * 
 * @param codec_num codec编号
 * @param cmd 
 */
void audio_cap_iis_watchdog_enable(const audio_cap_codec_num_t codec_num, FunctionalState cmd)
{
    #if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    IIS_TypeDef *IISx[2] = {IIS1, IIS3};
    IRQn_Type iis_irqn[3] = {IIS1_IRQn, IIS3_IRQn};
    #endif
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;
    
    //通过codec_num匹配到iis_num
    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    iis_num = get_iisnum_from_codecnum(codec_num);
    audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
    #endif

    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    if (ENABLE == cmd)
    {
        eclic_irq_enable(IIS3_IRQn);

        IIS_WatchDog_Init(IIS3, cmd);
    }
    else
    {
    	eclic_irq_disable(IIS3_IRQn);
    }
    #else
    if (ENABLE == cmd)
    {
        eclic_irq_enable(iis_irqn[iis_num - 1]);

        IIS_WatchDog_Init(IISx[iis_num - 1], cmd);
    }
    else
    {
    	eclic_irq_disable(iis_irqn[iis_num - 1]);
    }
    #endif
    
}


/**
 * @brief audio capture的采样率切换，注意切换采样的时候会改变IIS的时钟，
 * 改变之前需要暂停IIS，时钟设置完成之后再恢复时钟
 * 
 * @param codec_num codec编号
 * @param sr 需要改变的采样率
 */
void audio_cap_samplerate_set(const audio_cap_codec_num_t codec_num,audio_cap_sample_rate_t sr)
{
    uint32_t iis_src_clk = sg_iis_clk_src_fre;
    audio_cap_sample_rate_t audio_cap_sr[8] = {
        AUDIO_CAP_SAMPLE_RATE_8K,
        AUDIO_CAP_SAMPLE_RATE_12K,
        AUDIO_CAP_SAMPLE_RATE_16K, //采样率
        AUDIO_CAP_SAMPLE_RATE_24K,
        AUDIO_CAP_SAMPLE_RATE_32K,
        AUDIO_CAP_SAMPLE_RATE_44_1K,
        AUDIO_CAP_SAMPLE_RATE_48K,
        AUDIO_CAP_SAMPLE_RATE_96K,
    };
    inner_codec_samplerate_t codec_sr_arry[8] = {INNER_CODEC_SAMPLERATE_8K,
                                                INNER_CODEC_SAMPLERATE_12K,
                                                INNER_CODEC_SAMPLERATE_16K,
                                                INNER_CODEC_SAMPLERATE_24K,
                                                INNER_CODEC_SAMPLERATE_32K,
                                                INNER_CODEC_SAMPLERATE_44_1K,
                                                INNER_CODEC_SAMPLERATE_48K,
                                                INNER_CODEC_SAMPLERATE_96K};
    inner_codec_samplerate_t codec_sr = INNER_CODEC_SAMPLERATE_16K; //为codec设置的采样率，给alc使用
    for (int i = 0; i < 8; i++) //传递capture的采样率到CODEC
    {
        if (sr == audio_cap_sr[i])
        {
            codec_sr = codec_sr_arry[i];
            break;
        }
    }

    IIS_TypeDef *IISx[2] = {IIS1, IIS3};
    audio_cap_iis_num_t iis_num = AUDIO_CAP_IIS_NULL;

    //通过codec_num匹配到iis_num
    #if AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    iis_num = AUDIO_CAP_IIS3;
    #else
    iis_num = get_iisnum_from_codecnum(codec_num);
    audio_capture_assert_notequal_0(AUDIO_CAP_IIS_NULL == iis_num); //没有找到对应的IIS
    #endif

    audio_cap_over_sample_t over_sample = capture_equipment_state_str[iis_num - 1].over_sample;
    //设置IIS的分频参数

    //先暂停，采样率调整好了之后再重新开始
    audio_cap_stop(codec_num);
    Scu_SetDeviceGate((uint32_t)IISx[iis_num - 1], DISABLE);

    float div_para_f = (float)iis_src_clk / (float)sr / (float)over_sample;
    Scu_Setdiv_Parameter((uint32_t)IISx[iis_num - 1], (uint32_t)(div_para_f + 0.5f));
    if(AUDIO_CAP_INNER_CODEC == codec_num)
    {
        //inner_codec_set_sample_rate(codec_sr);
    }
    
    Scu_SetDeviceGate((uint32_t)IISx[iis_num - 1], ENABLE);
    audio_cap_start(codec_num);
}


static void _delay(unsigned int cnt)
{
    volatile unsigned int i,j = 0;
    for(i = 0; i < cnt; i++)
    {
        for(j = 0; j < 132; j++)
        {
            __NOP();
        }
    }
}


void iis3_div_para_change_to_half_of_180M_clk(void)
{
    Scu_SetDeviceGate((uint32_t)IIS3,DISABLE);
    Scu_SetDeviceGate(HAL_MCLK3_BASE,DISABLE);

    Unlock_Ckconfig();
    Scu_Para_En_Disable((uint32_t)IIS3);
    _delay(1);
    SCU->CLKDIV_PARAM3_CFG &= ~(0xff << 16);
    SCU->CLKDIV_PARAM3_CFG |= ((11 & 0xff) << 16);/////////
    Scu_Para_En_Enable((uint32_t)IIS3);
    _delay(1);

    Scu_SetDeviceGate((uint32_t)IIS3,ENABLE);
    Scu_SetDeviceGate(HAL_MCLK3_BASE,ENABLE);
}


void iis3_div_para_change_to_180M_clk(void)
{
    Scu_SetDeviceGate((uint32_t)IIS3,DISABLE);
    Scu_SetDeviceGate(HAL_MCLK3_BASE,DISABLE);

    Unlock_Ckconfig();
    Scu_Para_En_Disable((uint32_t)IIS3);
    _delay(1);
    SCU->CLKDIV_PARAM3_CFG &= ~(0xff << 16);
    SCU->CLKDIV_PARAM3_CFG |= ((22 & 0xff) << 16);/////////
    Scu_Para_En_Enable((uint32_t)IIS3);
    _delay(1);

    Scu_SetDeviceGate((uint32_t)IIS3,ENABLE);
    Scu_SetDeviceGate(HAL_MCLK3_BASE,ENABLE);
}


void iis1_div_para_change_to_half_of_180M_clk(void)
{
    Scu_SetDeviceGate((uint32_t)IIS1,DISABLE);
    Scu_SetDeviceGate(HAL_MCLK1_BASE,DISABLE);

    Unlock_Ckconfig();
    Scu_Para_En_Disable((uint32_t)IIS1);
    _delay(1);
    // SCU->CLKDIV_PARAM3_CFG &= ~(0xff << 16);
    // SCU->CLKDIV_PARAM3_CFG |= ((22 & 0xff) << 16);/////////
    SCU->CLKDIV_PARAM3_CFG &= ~(0xff << 0);
    SCU->CLKDIV_PARAM3_CFG |= ((22 & 0xff) << 0);/////////
    Scu_Para_En_Enable((uint32_t)IIS1);
    _delay(1);

    Scu_SetDeviceGate((uint32_t)IIS1,ENABLE);
    Scu_SetDeviceGate(HAL_MCLK1_BASE,ENABLE);
}


void iis1_div_para_change_to_180M_clk(void)
{
    Scu_SetDeviceGate((uint32_t)IIS1,DISABLE);
    Scu_SetDeviceGate(HAL_MCLK1_BASE,DISABLE);

    Unlock_Ckconfig();
    Scu_Para_En_Disable((uint32_t)IIS1);
    _delay(1);
    SCU->CLKDIV_PARAM3_CFG &= ~(0xff << 0);
    SCU->CLKDIV_PARAM3_CFG |= ((44 & 0xff) << 0);/////////
    Scu_Para_En_Enable((uint32_t)IIS1);
    _delay(1);

    Scu_SetDeviceGate((uint32_t)IIS1,ENABLE);
    Scu_SetDeviceGate(HAL_MCLK1_BASE,ENABLE);
}



/********** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
