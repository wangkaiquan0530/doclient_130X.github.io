/**
 * @file ci112x_audio_play_card.c
 * @brief CI112X的放音设备驱动设计
 *      支持使用不同的codec（INNO codec、8388、8374等）进行放音操作；
 *      支持放音格式设置，主要包括：左右/双通道、采样位宽、采样率；
 *      动态调整播放频率。
 * @version 0.1
 * @date 2019-03-28
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */

#include "ci112x_audio_play_card.h"
#include "ci112x_system.h"
#include "ci112x_iis.h"
#include "ci112x_iisdma.h"
#include "ci112x_codec.h"
#include "ci112x_scu.h"
#include "ci112x_core_eclic.h"
#include "ci112x_core_misc.h"
#include "ci112x_gpio.h"
#include "es8388.h"
//#include "es8311.h"
#include "ci112x_iis1_out_debug.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include "ci112x_gpio.h"
#include "sdk_default_config.h"
#include "platform_config.h"

/*******************************************************************************
                    type define
*******************************************************************************/

#define IIS_IPCORE_CLK get_ipcore_clk()
#define IIS_OSC_CLK    get_osc_clk()
#define IIS_PLL_CLK    get_iispll_clk()

#define INNER_CODEC_DAC_USE_PROBE 0

/**
 * @brief 放音设备状态管理结构体类型
 *
 */
typedef struct
{
    //codec编号
    audio_play_card_codec_num_t codec_num;
    //iis编号
    audio_play_card_iis_num_t iis_num;
    //过采样率
    audio_play_card_over_sample_t over_sample;
    //时钟源
    audio_play_card_clk_source_t clk_source;
    //通道选择
    audio_play_card_channel_sel_t cha_num;
    //codec的主从模式
    audio_play_card_codec_mode_t codec_mode;
    //数据格式
    audio_play_card_data_format_t data_format;
    //采样深度
    audio_play_card_bit_wide_t bit_wide;
    //!SCK和LRCK时钟频率的比值
    audio_play_card_sck_lrck_rate_t sck_lrck_rate;
    //block size（发送多少字节的数据来一次中断，在中断中需填写下一帧发送的数据的起始地址）
    int32_t block_size;
    //codec是否被初始化了
    uint8_t is_codec_init;
    //目前是否是强制mute声卡
    bool force_mute;
}audio_play_card_state_t;


//一个设备的codec num确定了之后可以再确定的信息
typedef struct
{
    IIS_TypeDef* IISx;//IIS设备
    IIS_TX_CHANNAL tx_cha;//IIS通道选择
	audio_play_card_channel_sel_t codec_cha;//codec通道选择
}info_from_codec_num_t;


//IIS设置确定之后，可以确定的一些信息
typedef struct
{
    IISChax iisdmacha;//iisdma 通道选择
    IISDMA_TypeDef* IISDMA;//iisdma设备
    audio_play_card_iis_num_t iis_num;
}get_iisdma_info_from_iisx_t;

/*******************************************************************************
                    extern
*******************************************************************************/


/*******************************************************************************
                    global
*******************************************************************************/

static uint32_t sg_play_done_timeout_times[AUDIO_PLAY_CODEC_MAX] = {0};//记录没有队列却来了多少次中断
static uint32_t sg_play_done_timeout_time_shreshold[AUDIO_PLAY_CODEC_MAX] = {0};
audio_play_callback_func_ptr_t sg_play_done_time_callback[AUDIO_PLAY_CODEC_MAX] = {0};//没有队列了却依然在播放的超时回调函数


//每个buf播放完成的回调函数
audio_play_callback_func_ptr_t sg_audio_play_callback[AUDIO_PLAY_CODEC_MAX] = {NULL};

//codec开始播放的回调函数
codec_play_start_func_ptr_t sg_codec_play_start_callback[AUDIO_PLAY_CODEC_MAX] = {NULL};

//一个消息是否被peek过的记录
static uint8_t sg_is_msg_peeked[AUDIO_PLAY_CODEC_MAX] = {0};
//一个消息的buf是否真的播放完了
static int is_buf_real_end[AUDIO_PLAY_CODEC_MAX] = {0};

//播放队列
QueueHandle_t Audio_Play_Card_xQueue[AUDIO_PLAY_CODEC_MAX] = {NULL};//队列

//用于同步audio_play_card的信息的全局变量结构体数组
audio_play_card_state_t audio_card_state_str[AUDIO_PLAY_CODEC_MAX];

static audio_writedate_t tmp_state_str[AUDIO_PLAY_CODEC_MAX];



/**
 * @brief 设置声卡播放的PA的管脚
 * 
 * @param pin_name 管脚名
 * @param reuse_fun 第几号的复用功能
 * @param gpio_base GPIO设备的基地址
 * @param pin pin脚定义
 */
void audio_play_card_pa_io_init(PinPad_Name pin_name,IOResue_FUNCTION reuse_fun,gpio_base_t gpio_base,gpio_pin_t pin)
{
    Scu_SetDeviceGate((uint32_t)gpio_base,ENABLE);
    Scu_SetIOReuse(pin_name,reuse_fun);    
}


/**
 * @brief 播放器没有队列获取到队列却依然没有停止播放，超过设定的次数之后会调回调函数的设置
 * 
 * @param fun 回调函数指针
 * @param shreshold_times 阈值设置（中断次数）
 */
void audio_play_card_play_done_timeout_callback_set(audio_play_card_codec_num_t codec_num,audio_play_callback_func_ptr_t fun,uint32_t shreshold_times)
{
    sg_play_done_time_callback[codec_num] = fun;
    sg_play_done_timeout_time_shreshold[codec_num] = shreshold_times;
}


/**
 * @brief 从codec num得到一些信息
 *
 * @param codec_num
 * @return info_from_codec_num_t
 */
static info_from_codec_num_t get_info_from_codec_num(const audio_play_card_codec_num_t codec_num)
{
    IIS_TypeDef* IISx[4] = {IIS0,IIS1,IIS2};
    audio_play_card_iis_num_t iis_num = AUDIO_PLAY_IIS_NULL;
    audio_play_card_channel_sel_t cha_num = AUDIO_PLAY_CHANNEL_LEFT;

    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if(codec_num == audio_card_state_str[i].codec_num)
        {
            iis_num = audio_card_state_str[i].iis_num;
            cha_num = audio_card_state_str[i].cha_num;
            break;
        }
    }
    audio_play_card_assert_notequal_0( AUDIO_PLAY_IIS_NULL == iis_num );

    IIS_TX_CHANNAL tx_cha_sel = IIS_TXCHANNAL_Left;

    if(AUDIO_PLAY_CHANNEL_LEFT == cha_num)
    {
        tx_cha_sel = IIS_TXCHANNAL_Left;
    }
    else if(AUDIO_PLAY_CHANNEL_RIGHT == cha_num)
    {
        tx_cha_sel = IIS_TXCHANNAL_Right;
    }
    else if(AUDIO_PLAY_CHANNEL_STEREO == cha_num)
    {
        tx_cha_sel = IIS_TXCHANNAL_All;
    }

    info_from_codec_num_t info_str;
    info_str.IISx = IISx[iis_num];
    info_str.tx_cha = tx_cha_sel;
	info_str.codec_cha = cha_num;
    return info_str;
}


/**
 * @brief 从iisx得到一些信息
 *
 * @param IISx
 * @return get_iisdma_info_from_iisx_t
 */
static get_iisdma_info_from_iisx_t get_iisdma_info(IIS_TypeDef* IISx)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;
    audio_play_card_iis_num_t iis_num = AUDIO_PLAY_IIS2;

    if(IIS0==IISx)
	{
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        // iis_num = AUDIO_PLAY_IIS1;
        CI_ASSERT(0,"iisx err!\n");
    }
	else if(IIS1==IISx)
	{
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
        iis_num = AUDIO_PLAY_IIS1;
    }
	else if(IIS2==IISx)
	{
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
        iis_num = AUDIO_PLAY_IIS2;
	}
    else if(IIS3==IISx)
	{
        // iisdmacha = IISCha1;
        // IISDMA = IISDMA1;
        CI_ASSERT(0,"iisx err!\n");
	}

    get_iisdma_info_from_iisx_t info_str;
    info_str.iisdmacha = iisdmacha;
    info_str.IISDMA = IISDMA;
    info_str.iis_num = iis_num;
    return info_str;
}


/**
 * @brief 播放静音（依然会送出数据，会来中断，但是送出的数据全为0）
 *
 * @param codec_num codec编号
 * @param cmd 开关
 */
static void audio_play_card_mute(const audio_play_card_codec_num_t codec_num,FunctionalState cmd)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    IIS_TypeDef* IISx = info_str.IISx;
    IIS_TX_CHANNAL tx_cha_sel = info_str.tx_cha;

    IIS_TXMute(IISx,tx_cha_sel,cmd);
}

/**
 * @brief 声卡wake mute，相较于force mute，wake mute的所有操作，在force mute为true时是无效的
 * 
 * @param codec_num codec编号
 * @param wake_mute_cmd wake mute开关
 */
void audio_play_card_wake_mute(const audio_play_card_codec_num_t codec_num,FunctionalState wake_mute_cmd)
{
    if(false == audio_card_state_str[codec_num].force_mute)
    {
        audio_play_card_mute(codec_num,wake_mute_cmd);
    }
}


/**
 * @brief 声卡force mute，强行开关声卡的mute，即调用了force mute开，必须调用force mute的关才能解除mute
 * 
 * @param codec_num codec编号
 * @param force_mute_cmd force mute开关
 */
void audio_play_card_force_mute(const audio_play_card_codec_num_t codec_num,FunctionalState force_mute_cmd)
{
    if(ENABLE == force_mute_cmd)
    {
        audio_card_state_str[codec_num].force_mute = true;
    }
    else
    {
        audio_card_state_str[codec_num].force_mute = false;
    }
    audio_play_card_mute(codec_num,force_mute_cmd);
}


/**
 * @brief enable某一IIS的某一通道的TX
 *
 * @param IISx IIS_TypeDef指针类型数据
 * @param tx_cha 通道号
 */
static void IISx_TX_enable(IIS_TypeDef* IISx,IIS_TX_CHANNAL tx_cha)
{
    BaseType_t xResult;
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;
    audio_play_card_iis_num_t iis_num = AUDIO_PLAY_IIS2;
    int32_t block_size = 0;
    audio_writedate_t buf_msg;
    audio_play_card_codec_num_t codec_num = AUDIO_PLAY_INNER_CODEC;

    get_iisdma_info_from_iisx_t info_str = get_iisdma_info(IISx);
    iisdmacha = info_str.iisdmacha;
    IISDMA = info_str.IISDMA;
    iis_num = info_str.iis_num;

    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if(iis_num == audio_card_state_str[i].iis_num)
        {
            codec_num = audio_card_state_str[i].codec_num;
            block_size = audio_card_state_str[i].block_size;
            break;
        }
    }

    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_TX_EN,ENABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_TX_EN,ENABLE);
    //clear IIS tx中断状态，本次IIS传输从TADDR0开始
    IISxDMA_TXADDRRollbackInterruptClear(IISDMA,iisdmacha,IISDMA_TXAAD_Sel_ADDR0);
    if(0 != check_curr_trap())
    {   
        xResult = xQueuePeekFromISR(Audio_Play_Card_xQueue[codec_num],&buf_msg );
    }
    else
    {
        xResult = xQueuePeek(Audio_Play_Card_xQueue[codec_num],&buf_msg,0);
    }
    if(pdPASS == xResult)
    {//首先填TADDR0
        sg_is_msg_peeked[codec_num] = 1;
        tmp_state_str[codec_num] = buf_msg;

        //mprintf("peek TADDR0   ");
        IISxDMA_TADDR0(IISDMA,iisdmacha,tmp_state_str[codec_num].data_current_addr);
        tmp_state_str[codec_num].data_current_addr += block_size;
        tmp_state_str[codec_num].data_remain_size -= block_size;      
    }
    else
    {
        mprintf("need two more buf msg\n");
        tmp_state_str[codec_num].data_remain_size = 0;
    }

    if(tmp_state_str[codec_num].data_remain_size >= block_size)
    {//如果一个buf里面的数据填了一个block，没有填完，下一个block填在TADDR1
        IISxDMA_TADDR1(IISDMA,iisdmacha,tmp_state_str[codec_num].data_current_addr);
        //if(AUDIO_PLAY_INNER_CODEC == codec_num)
        {
            //mprintf("current_addr is %08x    \n",tmp_state_str[codec_num].data_current_addr);
        }
        tmp_state_str[codec_num].data_current_addr += block_size;
        tmp_state_str[codec_num].data_remain_size -= block_size;
    }
    else
    {//这个buf填完了，需要将另一个buf填到TADDR中，需要另一个消息
        //if(0 != __get_IPSR())
        //{
            //xResult = xQueueReceiveFromISR(Audio_Play_Card_xQueue[codec_num],&buf_msg,NULL );//将消息从队列中移除
        //}
        //else
        {
            xResult = xQueueReceive(Audio_Play_Card_xQueue[codec_num],&buf_msg,pdMS_TO_TICKS(0));
        }
        sg_is_msg_peeked[codec_num] = 0;
        if(pdPASS == xResult)
        {
            if(0 != check_curr_trap())
            {   
                xResult = xQueuePeekFromISR(Audio_Play_Card_xQueue[codec_num],&buf_msg );
            }
            else
            {
                xResult = xQueuePeek(Audio_Play_Card_xQueue[codec_num],&buf_msg,0);
            }
            if(pdPASS == xResult)
            {
                sg_is_msg_peeked[codec_num] = 1;
                tmp_state_str[codec_num] = buf_msg;
                IISxDMA_TADDR1(IISDMA,iisdmacha,tmp_state_str[codec_num].data_current_addr);
                //if(AUDIO_PLAY_INNER_CODEC == codec_num)
                {
                    //mprintf("current_addr is %08x    \n",tmp_state_str[codec_num].data_current_addr);
                }
                tmp_state_str[codec_num].data_current_addr += block_size;
                tmp_state_str[codec_num].data_remain_size -= block_size;      
            }
            else
            {
                //mprintf("need one more buf msg\n");
            }
        }
        else
        {
            ci_logerr(CI_LOG_ERROR,"receive err!\n");
        }
    }

    /*enable iis*/
    IIS_TXFIFOClear(IISx,ENABLE);
    IISDMA_EN(IISDMA,ENABLE);
    IIS_TXEN(IISx,ENABLE);
    IIS_EN(IISx,ENABLE);
    
}


/**
 * @brief disable某一IIS的某一通道的TX
 *
 * @param IISx IIS_TypeDef指针类型数据
 * @param tx_cha 通道号
 */
void IISx_TX_disable(IIS_TypeDef* IISx,IIS_TX_CHANNAL tx_cha)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;

    get_iisdma_info_from_iisx_t info_str = get_iisdma_info(IISx);
    iisdmacha = info_str.iisdmacha;
    IISDMA = info_str.IISDMA;

    // IIS_TXMute(IISx,tx_cha,ENABLE);
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);

    /*enable iis*/
    IIS_TXEN(IISx,DISABLE);
    IIS_EN(IISx,DISABLE);
    IISDMA_EN(IISDMA,DISABLE);
}


/**
 * @brief 停止某一IIS的某一通道的TX传输
 *
 * @param IISx IISx：IIS_TypeDef指针类型数据
 * @param tx_cha 通道号
 */
static void IISx_TX_stop(IIS_TypeDef* IISx,IIS_TX_CHANNAL tx_cha)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;

    get_iisdma_info_from_iisx_t info_str = get_iisdma_info(IISx);
    iisdmacha = info_str.iisdmacha;
    IISDMA = info_str.IISDMA;

    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);
    for(int i=0;i<AUDIO_PLAY_CODEC_MAX;i++)
    {
        tmp_state_str[i].data_remain_size = 0;
    }
    IIS_TXEN(IISx,DISABLE);
}


/**
 * @brief 开始某一IIS的某一通道的TX传输
 *
 * @param IISx IIS_TypeDef指针类型数据
 * @param tx_cha 通道号
 */
static void IISx_TX_start(IIS_TypeDef* IISx,IIS_TX_CHANNAL tx_cha)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;

    get_iisdma_info_from_iisx_t info_str = get_iisdma_info(IISx);
    iisdmacha = info_str.iisdmacha;
    IISDMA = info_str.IISDMA;

    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_TX_EN,ENABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_TX_EN,ENABLE);
    // IIS_TXMute(IISx,tx_cha,DISABLE);
    IIS_TXEN(IISx,ENABLE);
}



/**
 * @brief 初始化codec播放的函数，调用结构体内部函数指针，及结构体内参数初始化codec
 *
 * @param regstr audio_play_card_init_t指针类型
 */
static void audio_play_codec_init(const audio_play_card_init_t* regstr)
{
    if(NULL != regstr->audio_play_codec_init)
    {
        regstr->audio_play_codec_init(regstr->sample_rate,regstr->bit_wide,regstr->codec_mode,regstr->data_format);
    }
}
/*******************************************************************************
                    function
*******************************************************************************/

/**
 * @brief audio_play IIS初始化，根据传入的参数重新配置放音设备的IIS。
 *
 * @param codec_num codec号
 * @param cha_num 通道选择
 * @param sample_rate 采样率
 * @param block_size 每次发送多少byte的数据之后来中断
 */
void audio_play_iis_init(audio_play_card_codec_num_t codec_num,audio_play_card_channel_sel_t cha_num,
                        audio_play_card_sample_rate_t sample_rate,int32_t block_size,
                        audio_play_card_clk_source_t clk_sor)
{
    IIS_TypeDef* IISx = IIS0;
    IISNUMx iis_clk_num = IISNum1;
    IOResue_FUNCTION IISx_IO_reuse = SECOND_FUNCTION;
    IRQn_Type IISDMAx = IIS_DMA0_IRQn;
    audio_play_card_iis_num_t iis_num = AUDIO_PLAY_IIS_NULL;
    audio_play_card_codec_mode_t codec_mode = AUDIO_PLAY_CARD_CODEC_MASTER;
    audio_play_card_data_format_t data_format = AUDIO_PLAY_CARD_DATA_FORMAT_LEFT_JUSTIFY;
    audio_play_card_clk_source_t clk_source = clk_sor;
    audio_play_card_over_sample_t over_sample = AUDIO_PLAY_OVER_SAMPLE_256;
    iis_bus_sck_lrckx_t iis_sck_lrck = IIS_BUSSCK_LRCK64;
    uint32_t mclk_base_addr = HAL_MCLK1_BASE;
	audio_play_card_bit_wide_t bit_wide = AUDIO_PLAY_BIT_WIDE_16;
    audio_play_card_sck_lrck_rate_t sck_lrck_rate = AUDIO_PLAY_SCK_LRCK_64;

    //根据codec num取出一些信息
    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if(codec_num == audio_card_state_str[i].codec_num)
        {
            iis_num = audio_card_state_str[i].iis_num;
            codec_mode = audio_card_state_str[i].codec_mode;
            data_format = audio_card_state_str[i].data_format;
            bit_wide = audio_card_state_str[i].bit_wide;
            sck_lrck_rate = audio_card_state_str[i].sck_lrck_rate;
            clk_source = audio_card_state_str[i].clk_source;
            over_sample = audio_card_state_str[i].over_sample;
            audio_card_state_str[i].cha_num = cha_num;//更新cha_num
            audio_card_state_str[i].block_size = block_size;//更新block_size
            break;
        }
    }
    audio_play_card_assert_notequal_0( AUDIO_PLAY_IIS_NULL == iis_num );

    //根据iis num获取一些信息
    if(AUDIO_PLAY_IIS1 == iis_num)
    {
        iis_clk_num = IISNum1;
        IISx = IIS1;
        IISx_IO_reuse = SECOND_FUNCTION;
		Scu_SetIOReuse(I2S1_MCLK_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDI_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDO_PAD,IISx_IO_reuse);
        mclk_base_addr = HAL_MCLK1_BASE;
    }
    else if(AUDIO_PLAY_IIS2 == iis_num)
    {
        iis_clk_num = IISNum2;
        IISx = IIS2;
        #if INNER_CODEC_DAC_USE_PROBE//探针模式
        IISx_IO_reuse = FORTH_FUNCTION;
		Scu_SetIOReuse(I2S1_MCLK_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDI_PAD,IISx_IO_reuse);
        Scu_SetIOReuse(I2S1_SDO_PAD,IISx_IO_reuse);
        //iis2_test_mode_set(IIS_SLAVE,IIS2_TEST_MODE_DATA_FROM_PAD);//IIS2_TEST_MODE_DATA_FROM_INNER_IIS);//IIS2_TEST_MODE_DATA_FROM_PAD);
        //mprintf("CODAC DAC PROBE mode\n");
        #endif
        mclk_base_addr = HAL_MCLK2_BASE;
    }
    else
    {
        audio_play_card_assert_equal_0( 0 );
    }
    
    //时钟源选择
    iis_clk_source_sel_t mclk_sor_sel = IIS_CLK_SOURCE_FROM_IPCORE;
    
    //Scu_SetDeviceGate((uint32_t)IISx,DISABLE);
    uint32_t clk = IIS_IPCORE_CLK;
    if(AUDIO_PLAY_CLK_SOURCE_OSC == clk_source)
    {
        clk = IIS_OSC_CLK;
        mclk_sor_sel = IIS_CLK_SOURCE_FROM_OSC;
    }
    else if(AUDIO_PLAY_CLK_SOURCE_DIVIDER_IPCORE == clk_source)
    {
        clk = IIS_IPCORE_CLK;
        mclk_sor_sel = IIS_CLK_SOURCE_FROM_IPCORE;
    }
    else
    {
//        CI_ASSERT(0);
    }

    //IIS过采样率设置
    iis_oversample_t iis_oversample = IIS_OVERSAMPLE_128Fs;
    if(AUDIO_PLAY_OVER_SAMPLE_128 == over_sample)
    {
        iis_oversample = IIS_OVERSAMPLE_128Fs;
    }
    else if(AUDIO_PLAY_OVER_SAMPLE_192 == over_sample)
    {
        iis_oversample = IIS_OVERSAMPLE_192Fs;
    }
    else if(AUDIO_PLAY_OVER_SAMPLE_256 == over_sample)
    {
        iis_oversample = IIS_OVERSAMPLE_256Fs;
    }
    else if(AUDIO_PLAY_OVER_SAMPLE_384 == over_sample)
    {
        iis_oversample = IIS_OVERSAMPLE_384Fs;
    }
    else
    {
        //audio_play_card_assert_equal_0( 0 );
    }

    //SCK与LRCK的比值
    iis_bus_sck_lrckx_t sck_lrck_rate_iis = IIS_BUSSCK_LRCK64;
    if(AUDIO_PLAY_SCK_LRCK_64 == sck_lrck_rate)
    {
        sck_lrck_rate_iis = IIS_BUSSCK_LRCK64;
        iis_sck_lrck = IIS_BUSSCK_LRCK64;
    }
    else
    {
        sck_lrck_rate_iis = IIS_BUSSCK_LRCK32;
        iis_sck_lrck = IIS_BUSSCK_LRCK32;
    }    
    eclic_irq_enable(IISDMAx);

    //IIS主从选择
    iis_mode_sel_t iis_mode = IIS_MASTER;
    if(AUDIO_PLAY_CARD_CODEC_MASTER == codec_mode)
    {
        iis_mode = IIS_SLAVE;
    }

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

    Scu_SetDeviceGate((uint32_t)IISx,DISABLE);
    Scu_SetDeviceGate(mclk_base_addr,DISABLE);
    Scu_SetIISCLK_Config(&cap_iis_clk_str);
    Scu_SetDeviceGate((uint32_t)IISx,ENABLE);
    Scu_SetDeviceGate(mclk_base_addr,ENABLE);

    IIS_TXRXDATAFMTx iis_data_format = IIS_TXDATAFMT_IIS;
    if(AUDIO_PLAY_CARD_DATA_FORMAT_LEFT_JUSTIFY == data_format)
    {
        iis_data_format = IIS_TXDATAFMT_LEFT_JUSTIFIED;
    }
    else if(AUDIO_PLAY_CARD_DATA_FORMAT_RIGHT_JUSTIFY == data_format)
    {
        iis_data_format = IIS_TXDATAFMT_RIGHT_JUSTIFIED;
    }

    eclic_irq_enable(IISDMAx);

    //采样深度设置
    IIS_TXRXDATAWIDEx data_wide = IIS_TXRXDATAWIDE16BIT;
    if(AUDIO_PLAY_BIT_WIDE_16 == bit_wide)
    {
        data_wide = IIS_TXRXDATAWIDE16BIT;
    }
    else if(AUDIO_PLAY_BIT_WIDE_24 == bit_wide)
    {
        data_wide = IIS_TXRXDATAWIDE24BIT;
    }
    else
    {
        audio_play_card_assert_equal_0(0);
    }

    //配置iis tx dma
    IIS_DMA_TXInit_Typedef IIS_DMA_Init_Struct = {0};
    IIS_DMA_Init_Struct.rollbackaddr0size = (IISDMA_RXTXxRollbackADDR)(block_size/16-1);
    IIS_DMA_Init_Struct.rollbackaddr1size = (IISDMA_RXTXxRollbackADDR)(block_size/16-1);
	IIS_DMA_Init_Struct.tx0singlesize	= IISDMA_TXRXSINGLESIZE16bytes;
	IIS_DMA_Init_Struct.tx1singlesize	= IISDMA_TXRXSINGLESIZE16bytes;
    IIS_DMA_Init_Struct.txdatabitwide	= data_wide;
    IIS_DMA_Init_Struct.txdatafmt	= iis_data_format;//IIS_TXDATAFMT_IIS;
    IIS_DMA_Init_Struct.sck_lrck		= sck_lrck_rate_iis;

    IISx_TXInit_noenable(IISx,&IIS_DMA_Init_Struct);
}


/**
 * @brief 注册放音设备，更新放音设备的IIS号、codec号、数据格式等，更新一些回调函数等。
 *
 * @param regstr audio_play_card_init_t结构体指针
 * @return int8_t
 */
int8_t audio_play_card_registe(const audio_play_card_init_t* regstr)
{
    audio_play_card_codec_num_t codec_num = regstr->codecx;
    audio_play_card_iis_num_t iis_num = AUDIO_PLAY_IIS2;
    audio_play_card_channel_sel_t cha_num = regstr->cha_num;
    uint8_t equipment_num = 0;

    if(AUDIO_PLAY_INNER_CODEC == codec_num)
    {
        iis_num = AUDIO_PLAY_IIS2;
    }
    else
    {
        iis_num = AUDIO_PLAY_IIS1;
    }

    //参数检查，判断设备是否已经被注册了
    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        audio_play_card_assert_notequal_0( (codec_num == audio_card_state_str[i].codec_num) || (iis_num == audio_card_state_str[i].iis_num) );//判断设备是否已经注册
    }

    //设备注册
    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if( (AUDIO_PLAY_CODEC_NULL == audio_card_state_str[i].codec_num) && (AUDIO_PLAY_IIS_NULL == audio_card_state_str[i].iis_num) )
        {
            audio_card_state_str[i].codec_num = codec_num;
            audio_card_state_str[i].iis_num = iis_num;
            audio_card_state_str[i].cha_num = cha_num;
            audio_card_state_str[i].codec_mode = regstr->codec_mode;
            audio_card_state_str[i].data_format = regstr->data_format;
            audio_card_state_str[i].block_size = regstr->block_size;
            audio_card_state_str[i].over_sample = regstr->over_sample;
            audio_card_state_str[i].clk_source = regstr->clk_source;
            break;
        }
        else
        {
            equipment_num++;
        }
    }
    //注册满了
    audio_play_card_assert_notequal_0( equipment_num ==  MAX_PLAY_CARD_NUM );

    //申请队列，只运行一次
    if(NULL == Audio_Play_Card_xQueue[codec_num])
    {
        if(AUDIO_PLAY_IIS == codec_num)
        {//如果使用调试模式
            Audio_Play_Card_xQueue[codec_num] = xQueueCreate(IIS_PRE_OUT_BLOCK_NUM,sizeof(audio_writedate_t));
            audio_play_card_assert_equal_0( Audio_Play_Card_xQueue[codec_num] );
        }
        else
        {
            Audio_Play_Card_xQueue[codec_num] = xQueueCreate(AUDIO_PLAY_CARD_BLOCK_NUM,sizeof(audio_writedate_t));
            audio_play_card_assert_equal_0( Audio_Play_Card_xQueue[codec_num] );
        }
    }

    sg_audio_play_callback[codec_num] = regstr->audio_play_buf_end_callback;
    sg_codec_play_start_callback[codec_num] = regstr->audio_play_codec_start_callback;

    return RETURN_OK;
}


/**
 * @brief 播放设备的初始化，通过传入的audio_play_card_init_t结构体指针，初始化放
 * 音设备的codec及IIS。
 *
 * @param regstr audio_play_card_init_t结构体指针
 * @return int8_t
 */
int8_t audio_play_card_init(const audio_play_card_init_t* regstr)
{
    audio_play_card_codec_num_t codec_num = regstr->codecx;
//    audio_play_card_iis_num_t iis_num = regstr->iisx;
    audio_play_card_channel_sel_t cha_num = regstr->cha_num;
    audio_play_card_sample_rate_t sample_rate = regstr->sample_rate;
    int32_t block_size = regstr->block_size;
    uint8_t is_codec_init = 0;
    int i=0;
    uint8_t equipment_num = 0;

    //codec编号与iis号配对，并获取codec初始化状态
    for(i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if(codec_num == audio_card_state_str[i].codec_num)
        {
            is_codec_init = audio_card_state_str[i].is_codec_init;
            audio_card_state_str[i].cha_num = cha_num;
            audio_card_state_str[i].codec_mode = regstr->codec_mode;
            audio_card_state_str[i].data_format = regstr->data_format;
            audio_card_state_str[i].block_size = regstr->block_size;
            audio_card_state_str[i].over_sample = regstr->over_sample;
            audio_card_state_str[i].clk_source = regstr->clk_source;
            audio_card_state_str[i].bit_wide = regstr->bit_wide;
            audio_card_state_str[i].sck_lrck_rate = regstr->sck_lrck_rate;
            break;
        }
        else{
            equipment_num++;
        }
    }

    //没有在已经注册的设备中找到需要初始化的设备
    audio_play_card_assert_notequal_0( equipment_num ==  MAX_PLAY_CARD_NUM );

    if(0 == is_codec_init)
    {//初始化codec dac
        audio_play_codec_init(regstr);
        audio_card_state_str[i].is_codec_init++;
    }
    //初始化IIS
    audio_play_iis_init(codec_num,cha_num,sample_rate,block_size,regstr->clk_source);

    return RETURN_OK;
}


/**
 * @brief 为INNER codec编写的初始化函数（使用codec左声道还是右声道的数据只能通过IIS的
 * 配置设定，不能通过CODEC设置。）
 *
 * @param sample_rate sample_rate：采样率
 * @param bit_wide 采样深度
 * @param codec_mode codec主从模式
 * @param data_format 数据格式选择
 */
void inner_codec_play_init(audio_play_card_sample_rate_t sample_rate, audio_play_card_bit_wide_t bit_wide,
                            audio_play_card_codec_mode_t codec_mode,audio_play_card_data_format_t data_format)
{
    //有效数据位设置
    inner_codec_valid_word_len_t valid_word_len = INNER_CODEC_VALID_LEN_16BIT;
    switch(bit_wide)
    {
        case AUDIO_PLAY_BIT_WIDE_16:
        {
            valid_word_len = INNER_CODEC_VALID_LEN_16BIT;
            break;
        }
        case AUDIO_PLAY_BIT_WIDE_24:
        {
            valid_word_len = INNER_CODEC_VALID_LEN_24BIT;
            break;
        }
        default:
            audio_play_card_assert_equal_0(0);
            break;
    }
    /*配置DAC的模式和数据格式*/
    inner_codec_mode_t mode = INNER_CODEC_MODE_SLAVE;
    if(AUDIO_PLAY_CARD_CODEC_MASTER == codec_mode)
    {
        mode = INNER_CODEC_MODE_MASTER;
    }

    inner_codec_i2s_data_famat_t format = INNER_CODEC_I2S_DATA_FORMAT_I2S_MODE;
    if(AUDIO_PLAY_CARD_DATA_FORMAT_LEFT_JUSTIFY == data_format)
    {
        format = INNER_CODEC_I2S_DATA_FORMAT_LJ_MODE;
    }
    else if(AUDIO_PLAY_CARD_DATA_FORMAT_RIGHT_JUSTIFY == data_format)
    {
        format = INNER_CODEC_I2S_DATA_FORMAT_RJ_MODE;
    }
    inner_codec_dac_mode_set(mode,INNER_CODEC_FRAME_LEN_32BIT,valid_word_len,format);
}


/**
 * @brief 采音标准接口使用的中断处理函数
 *
 * @param iis_num IIS编号
 * @param IISDMA IISDMA0或者IISDMA1
 * @param iischa IISChax枚举类型
 */
void audio_play_card_handler(const audio_play_card_iis_num_t iis_num,IISDMA_TypeDef* IISDMA,IISChax iischa,BaseType_t * xHigherPriorityTaskWoken)
{
    BaseType_t xResult;
    audio_writedate_t buf_msg;

    audio_play_card_codec_num_t codec_num = AUDIO_PLAY_INNER_CODEC;
    int32_t block_size = 0;

    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if(iis_num == audio_card_state_str[i].iis_num)
        {
            codec_num = audio_card_state_str[i].codec_num;
            block_size = audio_card_state_str[i].block_size;
            break;
        }
    }

	//一个消息还没有播完
    if(tmp_state_str[codec_num].data_remain_size >= block_size)//这个队列里面的buf没有播完，继续播
    {
        if(0 == IISDMA_TX_ADDR_Get(IISDMA,iischa))
        {
            //if(AUDIO_PLAY_INNER_CODEC == codec_num)
            //{
                //mprintf("TADDR1   ");
            //}
            IISxDMA_TADDR1(IISDMA,iischa,tmp_state_str[codec_num].data_current_addr);
        }
        else
        {
            //if(AUDIO_PLAY_INNER_CODEC == codec_num)
            //{
                //mprintf("TADDR0   ");
            //}
            IISxDMA_TADDR0(IISDMA,iischa,tmp_state_str[codec_num].data_current_addr);
        }
        //if(AUDIO_PLAY_INNER_CODEC == codec_num)
        {
            //mprintf("current_addr is %08x    \n",tmp_state_str[codec_num].data_current_addr);
        }

        tmp_state_str[codec_num].data_current_addr += block_size;
        tmp_state_str[codec_num].data_remain_size -= block_size;
    }
    else//一个消息里面的播完了
    {
        if( 0 != sg_is_msg_peeked[codec_num])
        {
            xQueueReceiveFromISR(Audio_Play_Card_xQueue[codec_num],&buf_msg,xHigherPriorityTaskWoken );//将消息从队列中移除
            if(sg_audio_play_callback[codec_num] != NULL)
            {
                sg_audio_play_callback[codec_num](xHigherPriorityTaskWoken);
            }
            //mprintf("receive\n");
        }
        xResult = xQueuePeekFromISR(Audio_Play_Card_xQueue[codec_num],&buf_msg );
        if(pdPASS == xResult)//如果队列里面还有消息，且peek成功了
        {         
            tmp_state_str[codec_num] = buf_msg;
            
            if(0 == IISDMA_TX_ADDR_Get(IISDMA,iischa))
            {
                //if(AUDIO_PLAY_INNER_CODEC == codec_num)
                //{
                    //mprintf("TADDR1   ");
                //}
                IISxDMA_TADDR1(IISDMA,iischa,tmp_state_str[codec_num].data_current_addr);
            }
            else{
                //if(AUDIO_PLAY_INNER_CODEC == codec_num)
                //{
                    //mprintf("TADDR0   ");
                //}
                IISxDMA_TADDR0(IISDMA,iischa,tmp_state_str[codec_num].data_current_addr);
            }
            //if(AUDIO_PLAY_INNER_CODEC == codec_num)
            {
                //mprintf("current_addr is %08x    \n",tmp_state_str[codec_num].data_current_addr);
            }

            if(0 == sg_is_msg_peeked[codec_num])
            {
                sg_is_msg_peeked[codec_num] = 1;
                if(false == audio_card_state_str[codec_num].force_mute)
                {
                    audio_play_card_mute(codec_num,DISABLE);
                }
            }

            tmp_state_str[codec_num].data_current_addr += block_size;
            tmp_state_str[codec_num].data_remain_size -= block_size;
        }
        else
        {
            is_buf_real_end[codec_num]++;
            if(is_buf_real_end[codec_num] > 1)
            {
                is_buf_real_end[codec_num] = 0;
                sg_is_msg_peeked[codec_num] = 0;
                audio_play_card_mute(codec_num,ENABLE);
                sg_play_done_timeout_times[codec_num]++;//没有队列了却依然没有停止播放，超时的处理
                if( (sg_play_done_timeout_times[codec_num] > sg_play_done_timeout_time_shreshold[codec_num]) && (sg_play_done_timeout_time_shreshold[codec_num] > 0) )
                {
                    if(sg_play_done_time_callback[codec_num] != NULL)
                    {
                        sg_play_done_time_callback[codec_num](xHigherPriorityTaskWoken);
                    }
                }
                // FIXME: 目前的方式这个打印会很多，将其打印等级从warn提高到debug
                //ci_logwarn(LOG_PLAY_CARD,"no audio play queue\n");
                ci_logdebug(LOG_PLAY_CARD,"no audio play queue\n");
            }

        }
    }
}


/**
 * @brief 向声卡写数据，将需要播放的pcmbuf的起始地址和大小，发送到播放队列。
 *
 * @param pcm_buf pcm buf的指针
 * @param buf_size buf的大小
 * @param type 写数据的类型，有阻塞模式，非阻塞模式，超时模式
 * @param time 超时时间
 * @return audio_play_queue_state_t 表示播放队列的状态
 */
audio_play_queue_state_t audio_play_card_writedata(audio_play_card_codec_num_t codec_num,void* pcm_buf,uint32_t buf_size,
                                                    audio_play_block_t type,uint32_t time)
{
    BaseType_t xResult = pdFAIL;

    audio_writedate_t audio_writedata_msg;
    audio_writedata_msg.data_current_addr = (uint32_t)pcm_buf;
    audio_writedata_msg.data_remain_size = buf_size;
    
    if(AUDIO_PLAY_BLOCK_TYPE_UNBLOCK_FROM_ISR == type)
    {
        xResult = xQueueSendFromISR(Audio_Play_Card_xQueue[codec_num],&audio_writedata_msg,0);
    }
    else if(AUDIO_PLAY_BLOCK_TYPE_UNBLOCK == type)
    {
        xResult = xQueueSend(Audio_Play_Card_xQueue[codec_num],&audio_writedata_msg,0);
    }
    else if(AUDIO_PLAY_BLOCK_TYPE_TIMEOUT == type)
    {
        xResult = xQueueSend(Audio_Play_Card_xQueue[codec_num],&audio_writedata_msg,pdMS_TO_TICKS(time));
    }
    else if(AUDIO_PLAY_BLOCK_TYPE_BLOCK == type)
    {
        xResult = xQueueSend(Audio_Play_Card_xQueue[codec_num],&audio_writedata_msg,portMAX_DELAY);
    }

    if(pdPASS != xResult)
    {
        return AUDIO_PLAY_QUEUE_FULL;
    }
    else
    {
        return AUDIO_PLAY_QUEUE_EMPTY;
    }
}


/**
 * @brief inner codec开始播放的函数
 *
 * @param codec_cha 声道选择
 * @param cmd 开关
 */
void codec_play_inner_codec(audio_play_card_channel_sel_t codec_cha,FunctionalState cmd)
{
    //打开codec dac
	inner_codec_dac_enable();

    inner_cedoc_gate_t codec_dac_left_gate = INNER_CODEC_GATE_ENABLE;
    inner_cedoc_gate_t codec_dac_right_gate = INNER_CODEC_GATE_ENABLE;
	if(ENABLE == cmd)
	{
		if(AUDIO_PLAY_CHANNEL_STEREO == codec_cha)
        {
            codec_dac_left_gate = INNER_CODEC_GATE_DISABLE;
            codec_dac_right_gate = INNER_CODEC_GATE_DISABLE;
        }
        else if(AUDIO_PLAY_CHANNEL_LEFT == codec_cha)
        {
            codec_dac_left_gate = INNER_CODEC_GATE_DISABLE;
            codec_dac_right_gate = INNER_CODEC_GATE_ENABLE;
        }
        else if(AUDIO_PLAY_CHANNEL_RIGHT == codec_cha)
        {
            codec_dac_left_gate = INNER_CODEC_GATE_ENABLE;
            codec_dac_right_gate = INNER_CODEC_GATE_DISABLE;
        }
	}
    //根据选择的通道关闭相应的codec dac的通道
    inner_codec_dac_disable(INNER_CODEC_LEFT_CHA,codec_dac_left_gate);
}


/**
 * @brief 8388开始播放的函数
 *
 * @param codec_cha 声道选择
 * @param cmd 开关
 */
void codec_play_8388(audio_play_card_channel_sel_t codec_cha,FunctionalState cmd)
{
	es8388_cha_sel_t codec8388_cha = ES8388_CHA_STEREO;
	if(AUDIO_PLAY_CHANNEL_RIGHT == codec_cha)
	{
		codec8388_cha = ES8388_CHA_RIGHT;
	}
	else if(AUDIO_PLAY_CHANNEL_LEFT == codec_cha)
	{
		codec8388_cha = ES8388_CHA_LEFT;
	}
	// TODO: 这里先初始化8388会导致pop音，但是后初始化8388产生较大延迟导致dma音频队列播放完毕未能及时补充
	ES8388_DAC_power(ES8388_NUM2,codec8388_cha,ENABLE);
    ES8388_DAC_mute(ES8388_NUM2,DISABLE);
    ES8388_dac_vol_set(ES8388_NUM2,-10,-10,-10,-10);
}


/**
 * @brief 开始放音，开始IIS、codec和PA
 *
 * @param codec_num codec编号
 */
void audio_play_card_start(const audio_play_card_codec_num_t codec_num,FunctionalState pa_cmd)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    IIS_TypeDef* IISx = info_str.IISx;
    IIS_TX_CHANNAL tx_cha_sel = info_str.tx_cha;
    audio_play_card_channel_sel_t codec_cha = info_str.codec_cha;
    
    IISx_TX_enable(IISx,tx_cha_sel);
    audio_play_card_mute(codec_num,ENABLE);
    
    /*audio PA on*/
    if(ENABLE == pa_cmd)
    {
        if(NULL != sg_codec_play_start_callback[codec_num])
        {
            //调用codec开始的回调函数
            sg_codec_play_start_callback[codec_num](codec_cha,ENABLE);
        }
        //在DAC打开的情况下开关PA
        #if (PLAYER_CONTROL_PA)//如果需要声卡控制PA，则打开PA
            if(true == get_pa_control_level_flag())
            {
                gpio_set_output_high_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
            }
            else
            {
                gpio_set_output_low_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
            }
        #endif
    }
    audio_play_card_mute(codec_num,DISABLE);
}


/**
 * @brief 打开PA和DAC
 * 
 * @param codec_num 
 */
void audio_play_card_pa_da_en(const audio_play_card_codec_num_t codec_num)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    audio_play_card_channel_sel_t codec_cha = info_str.codec_cha;

    if(NULL != sg_codec_play_start_callback[codec_num])
    {
        sg_codec_play_start_callback[codec_num](codec_cha,ENABLE);
    }
    #if (PLAYER_CONTROL_PA)
        if(true == get_pa_control_level_flag())
        {
            gpio_set_output_high_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
        else
        {
            gpio_set_output_low_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
    #endif
}


/**
 * @brief 结束放音，停止IIS、codec和PA
 *
 * @param codec_num codec编号
 */
void audio_play_card_stop(const audio_play_card_codec_num_t codec_num)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    IIS_TypeDef* IISx = info_str.IISx;
    IIS_TX_CHANNAL tx_cha_sel = info_str.tx_cha;

    IISx_TX_stop(IISx,tx_cha_sel);
}



/**
 * @brief 关闭PA和DAC
 * 
 * @param codec_num 
 */
void audio_play_card_pa_da_diable(const audio_play_card_codec_num_t codec_num)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    audio_play_card_channel_sel_t codec_cha = info_str.codec_cha;
    /*audio PA on*/
    #if (PLAYER_CONTROL_PA)
        if(true == get_pa_control_level_flag())
        {
            //gpio_set_output_high_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
            gpio_set_output_low_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);         //代小韩修改
        }
        else
        {
            //gpio_set_output_low_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
            gpio_set_output_high_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
    if(NULL != sg_codec_play_start_callback[codec_num])
    {
        sg_codec_play_start_callback[codec_num](codec_cha,DISABLE);
    }
    #endif

    #if (USE_OUTSIDE_CODEC_PLAY && MIC_FROM_ES8311)
    if(NULL != sg_codec_play_start_callback[codec_num])
    {
        sg_codec_play_start_callback[codec_num](codec_cha,DISABLE);
    }
    #endif
}


/**
 * @brief 暂停播放，这个函数只停止IIS，不对codec进行操作
 *
 * @param codec_num codec编号
 */
void audio_play_card_iis_stop(const audio_play_card_codec_num_t codec_num)
{
    info_from_codec_num_t info_str = get_info_from_codec_num(codec_num);
    IIS_TypeDef* IISx = info_str.IISx;
    IIS_TX_CHANNAL tx_cha_sel = info_str.tx_cha;

    IISx_TX_stop(IISx,tx_cha_sel);
}



/**
 * @brief 开启放音设备开始录音
 *
 * @param codec_num codec编号
 */
void audio_play_card_iis_enable(const audio_play_card_codec_num_t codec_num)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    IIS_TypeDef* IISx = info_str.IISx;
    IIS_TX_CHANNAL tx_cha_sel = info_str.tx_cha;
    IISx_TX_enable(IISx,tx_cha_sel);
    audio_play_card_wake_mute(codec_num,DISABLE);
}



/**
 * @brief 开启放音设备开始录音，用于stop之后再开始
 *
 * @param codec_num codec编号
 */
void audio_play_card_iis_start(const audio_play_card_codec_num_t codec_num)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    IIS_TypeDef* IISx = info_str.IISx;
    IIS_TX_CHANNAL tx_cha_sel = info_str.tx_cha;
    IISx_TX_start(IISx,tx_cha_sel);;
}



/**
 * @brief 单声道变成双声道输出
 *
 * @param codec_num codec编号
 * @param cmd 使能或禁止
 */
void audio_play_card_mono_mage_to_stereo(const audio_play_card_codec_num_t codec_num,FunctionalState cmd)
{
    info_from_codec_num_t info_str;
    info_str = get_info_from_codec_num(codec_num);

    IIS_TypeDef* IISx = info_str.IISx;
    //IIS硬件copy单声道数据到双声道
    IIS_TXCopy_Mono_To_Stereo_En(IISx,cmd);
}


/**
 * @brief 放音设备删除
 *
 * @param codec_num codec编号
 */
void audio_play_card_delete(const audio_play_card_codec_num_t codec_num)
{
    uint8_t equipment_num = 0;
    for(int i=0;i<MAX_PLAY_CARD_NUM;i++)
    {
        if(codec_num == audio_card_state_str[i].codec_num)
        {
            memset((void*)(&audio_card_state_str[i]),0,sizeof(audio_play_card_state_t));
            break;
        }
        else
        {
            equipment_num++;
        }
    }
    audio_play_card_assert_notequal_0( equipment_num ==  MAX_PLAY_CARD_NUM );
}


/**
 * @brief 清空所有的播放队列
 *
 */
void reset_audio_play_card_queue(audio_play_card_codec_num_t codec_num)
{
    xQueueReset(Audio_Play_Card_xQueue[codec_num]);
}


/**
 * @brief receive掉一个msg，在队列太多的时候使用
 *
 */
void audio_play_card_receive_one_msg(audio_play_card_codec_num_t codec_num)
{
    audio_writedate_t buf_msg;
    if(0 != check_curr_trap())
    {
        if(pdPASS != xQueueReceiveFromISR(Audio_Play_Card_xQueue[codec_num],&buf_msg,0))
        {
            // CI_ASSERT(0,"reev error\n");
        }
    }
    else
    {
        if(pdPASS != xQueueReceive(Audio_Play_Card_xQueue[codec_num],&buf_msg,pdMS_TO_TICKS(0)))
        {
            // CI_ASSERT(0,"reev error\n");
        }
    }
}



/**
 * @brief 得到播放队列里面还有多少个消息
 *
 * @return int16_t 没有播放完成的消息的数量
 */
int16_t get_audio_play_card_queue_remain_nums(audio_play_card_codec_num_t codec_num)
{
    UBaseType_t count;
    if(0 != check_curr_trap())
    {
        count = uxQueueMessagesWaitingFromISR(Audio_Play_Card_xQueue[codec_num]);
    }
    else
    {
        count = uxQueueMessagesWaiting(Audio_Play_Card_xQueue[codec_num]);
    }
    return count;
}


/**
 * @brief 查看播放队列是否满
 *
 * @return int32_t  RETURN_ERR：播放队列未满
 *                   RETURN_OK：播放队列满
 */
int32_t audio_play_card_queue_is_full(audio_play_card_codec_num_t codec_num)
{
    #if 0
    if(pdFALSE == xQueueGenericIsFull(Audio_Play_Card_xQueue))
    {
        return RETURN_ERR;
    }
    else
    {
        return RETURN_OK;
    }
    #else
    UBaseType_t count = uxQueueMessagesWaiting(Audio_Play_Card_xQueue[codec_num]);
    //为什么是count + 1，因为硬件IIS TX使用的乒乓buf，固定会使用两个
    if( (count + 1) < AUDIO_PLAY_CARD_BLOCK_NUM)
    {
        return RETURN_ERR;
    }
    else
    {
        return RETURN_OK;
    }
    #endif
}


/**
 * @brief 调节声卡的音量（0--100）
 * 
 * @param codec_num codec编号
 * @param l_gain 左声道增益
 * @param r_gain 右声道增益
 */
void audio_play_card_set_vol_gain(audio_play_card_codec_num_t codec_num,int32_t l_gain,int32_t r_gain)
{
    if(AUDIO_PLAY_INNER_CODEC == codec_num)
    {
        inner_codec_dac_gain_set(l_gain,r_gain);
    }
    else
    {
        #if (USE_OUTSIDE_CODEC_PLAY)
        // es8311_dac_vol_set(l_gain);
        #endif
    }
}

/********** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
