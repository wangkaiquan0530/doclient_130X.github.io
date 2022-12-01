/**
 * @file ci112x_iis.h
 * @brief 二代芯片IIS底层驱动接口头文件
 * @version 0.1
 * @date 2019-05-10
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef __CI112X_IIS_H
#define __CI112X_IIS_H


#include "ci112x_system.h"
#include "ci112x_iisdma.h" 
#include "ci112x_scu.h"


#ifdef __cplusplus
extern "C" {
#endif 


/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_iis ci112x_iis
 * @brief CI112X芯片iis驱动
 * @{
 */


/**
 * @brief IIS通道选择
 * 
 */
typedef enum
{
    //!左通道
    IIS_TXCHANNAL_Left	=0,
    //!右通道
    IIS_TXCHANNAL_Right	=1,
    //!两个通道
    IIS_TXCHANNAL_All = 2,
}IIS_TX_CHANNAL;


/**
 * @brief IIS数据宽度
 * 
 */
typedef enum
{
    //!数据宽度为16bit
    IIS_TXRXDATAWIDE16BIT	=0,
    //!数据宽度为24bit
    IIS_TXRXDATAWIDE24BIT	=1,
    //!数据宽度为32bit
    IIS_TXRXDATAWIDE32BIT	=2,
    //!数据宽度为20bit
    IIS_TXRXDATAWIDE20BIT	=3,
    //!数据宽度为8bit
    IIS_TXRXDATAWIDE8BIT	=4,
}IIS_TXRXDATAWIDEx;


/**
 * @brief IIS TX FIFO触发深度
 * 
 */
typedef enum
{
    //!IIS TX FIFO触发深度为1/2 FIFO
    IIS_TXFIFOLEVEL1_2=0,
    //!IIS TX FIFO触发深度为1/4 FIFO
    IIS_TXFIFOLEVEL1_4=1,
}IIS_TXFIFOLEVELx;



/**
 * @brief IIS TX通道选择
 * 
 */
typedef enum
{
    IIS_TXCHNUM_STEREO	=0,
    IIS_TXCHNUM_FOURE 	=1,
    IIS_TXCHNUM_SIX	=2, 
}IIS_TXCHNUMx;


/**
 * @brief IIS RX FIFO触发深度
 * 
 */
typedef enum
{
    //!IIS FIFO触发深度为1/4
    IIS_RXFIFOLEVEL1_4 =0,
    //!IIS FIFO触发深度为1/8
    IIS_RXFIFOLEVEL1_8	=1,
    //!IIS FIFO触发深度为1/16
    IIS_RXFIFOLEVEL1_16=2,
    //!IIS FIFO触发深度为1/32
    IIS_RXFIFOLEVEL1_32=3,
}IIS_RXFIFOLEVELx; 



/**
 * @brief IIS格式（标准IIS格式，左对齐格式，右对齐格式）
 * 
 */
typedef enum
{
    //!RX格式为标准IIS格式
    IIS_RXDATAFMT_IIS				=0,
    //!RX格式为左对齐格式
    IIS_RXDATAFMT_LEFT_JUSTIFIED	=1,
    //!RX格式为右对齐格式
    IIS_RXDATAFMT_RIGHT_JUSTIFIED	=2,
    //!TX格式为标准IIS格式
    IIS_TXDATAFMT_IIS				=3,
    //!TX格式为左对齐格式
    IIS_TXDATAFMT_LEFT_JUSTIFIED	=4,
    //!TX格式为右对齐格式
    IIS_TXDATAFMT_RIGHT_JUSTIFIED	=5,
}IIS_TXRXDATAFMTx;


/**
 * @brief IIS输入输出选择
 * 
 */
typedef enum
{
    IIS_AUDIO_INPUT     =0,
    IIS_AUDIO_OUTPUT 	=1,
}IIS_CHSELAUDIOINOUTx;


/**
 * @brief IIS数据merge选择
 * 
 */
typedef enum
{
    //!将双通道16bit的数据merge到一个32bit的数据
    IIS_MERGE_16BITTO32BIT	=1,
    //!不merge，双通道16bit的数据，在两个32bit的数据上
    IIS_MERGE_NONE			=0,
}IIS_MERGEx;


/**
 * @brief IIS数据merge的时候，左通道数据在高位还是右通道数据在高位
 * 
 */
typedef enum
{
    //!merge的时候左通道数据在高位
    IIS_LR_LEFT_HIGH_RIGHT_LOW=0,
    //!merge的时候右通道数据在高位
    IIS_LR_RIGHT_HIGH_LEFT_LOW=1,
}IIS_LR_SELx;


/**
 * @brief IIS RX通道选择（单通道还是双通道）
 * 
 */
typedef enum
{
    //!IIS RX为单通道
    IIS_RXMODE_MONO		=1,
    //!IIS RX为双通道
    IIS_RXMODE_STEREO	=0,
}IIS_RXMODEx;









/**
 * @brief IIS TX初始化结构体
 * 
 */
typedef struct
{
    //!TXADDR0地址设置
    unsigned int txaddr0;
    //!TXADDR1地址设置
    unsigned int txaddr1;
    //!IISDMA传输地址回卷中断产生，需要IISDMA搬运多少次
    IISDMA_RXTXxRollbackADDR rollbackaddr0size;
    //!IISDMA传输地址回卷中断产生，需要IISDMA搬运多少次
    IISDMA_RXTXxRollbackADDR rollbackaddr1size;
    //!IISDMA单次搬运的数据大小
    IISDMA_TXRXSingleSIZEx tx0singlesize;
    //!IISDMA单次搬运的数据大小
    IISDMA_TXRXSingleSIZEx tx1singlesize;
    //!数据宽度
    IIS_TXRXDATAWIDEx txdatabitwide;
    //!数据格式
    IIS_TXRXDATAFMTx txdatafmt;
    iis_bus_sck_lrckx_t sck_lrck;
}IIS_DMA_TXInit_Typedef;


/**
 * @brief IIS RX初始化结构体
 * 
 */
typedef struct
{
    //!RX的地址设置
    unsigned int rxaddr;
    //!IISDMA搬运多少次之后，产生传输完成中断
    IISDMA_RXxInterrupt rxinterruptsize;
    //!IISDMA搬运多少次之后，产生地址回卷中断
    IISDMA_RXTXxRollbackADDR rollbackaddrsize;
    //!IISDMA单次传输数据大小
    IISDMA_TXRXSingleSIZEx rxsinglesize;
    //!数据宽度
    IIS_TXRXDATAWIDEx rxdatabitwide;
    //!数据格式
    IIS_TXRXDATAFMTx rxdatafmt;
    iis_bus_sck_lrckx_t sck_lrck;
}IIS_DMA_RXInit_Typedef;

void IIS_TXCopy_Mono_To_Stereo_En(IIS_TypeDef* IISx,FunctionalState Cmd);
void IIS_RXFIFOClear(IIS_TypeDef* IISx,FunctionalState cmd);
void IIS_TXFIFOClear(IIS_TypeDef* IISx,FunctionalState Cmd);
void IIS_TXMute(IIS_TypeDef* IISx,IIS_TX_CHANNAL Cha,FunctionalState cmd);
void IIS_RX_MuteEN(IIS_TypeDef* IISx,FunctionalState cmd);
void IISx_RXInit(IIS_TypeDef* IISx,IIS_DMA_RXInit_Typedef* IIS_DMA_Init_Struct);
void IISx_TXInit(IIS_TypeDef* IISx,IIS_DMA_TXInit_Typedef* IIS_DMA_Init_Struct);
void IISx_TXInit_noenable(IIS_TypeDef* IISx,IIS_DMA_TXInit_Typedef* IIS_DMA_Init_Struct);
void IISx_RXInit_noenable(IIS_TypeDef* IISx,IIS_DMA_RXInit_Typedef* IIS_DMA_Init_Struct);
void IIS_RXMODEConfig(IIS_TypeDef* IISx,IIS_MERGEx merge,\
        IIS_LR_SELx lr_sel,IIS_RXMODEx iisrxmode);
void IIS_TXEN(IIS_TypeDef* IISx,FunctionalState cmd);
void IIS_RXEN(IIS_TypeDef* IISx,FunctionalState cmd);
void IIS_EN(IIS_TypeDef* IISx,FunctionalState cmd);
void IIS_CHSELConfig(IIS_TypeDef* IISx,IIS_CHSELAUDIOINOUTx iischselinout\
	,FunctionalState cmd);
void IISx_RX_enable_int(IIS_TypeDef* IISx);
void IISx_RX_enable_straight_to_vad(IIS_TypeDef* IISx);
void IISx_RX_disable(IIS_TypeDef* IISx);

void IIS_WatchDog_Init(IIS_TypeDef* iis,FunctionalState Cmd);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif 

#endif /*__IIS_H*/

/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/

