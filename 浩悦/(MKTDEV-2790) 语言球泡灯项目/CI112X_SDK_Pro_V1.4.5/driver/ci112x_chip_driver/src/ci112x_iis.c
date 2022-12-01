/**
 * @file ci112x_iis.c
 * @brief 二代芯片IIS底层驱动接口
 * @version 0.1
 * @date 2019-05-10
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */

#include "ci112x_iis.h"
#include "ci_assert.h"



/**
 * @brief IIS TX FIFO清除
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param Cmd 使能或不使能
 */
void IIS_TXFIFOClear(IIS_TypeDef* IISx,FunctionalState Cmd)
{
    uint32_t iis_tx_ctrl = IISx->IISTXCTRL;
    if(ENABLE == Cmd)
    {
        iis_tx_ctrl |= 1<<10;
    }
    else
    {
        iis_tx_ctrl &= ~(1<<10);
    }
    IISx->IISTXCTRL = iis_tx_ctrl;
}


/**
 * @brief IIS TX copy，将单声道数据复制为双声道输出
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param Cmd 使能或禁止TX copy功能
 */
void IIS_TXCopy_Mono_To_Stereo_En(IIS_TypeDef* IISx,FunctionalState Cmd)
{
    uint32_t iis_tx_ctrl_reg = IISx->IISTXCTRL;
    if(ENABLE == Cmd)
    {
        iis_tx_ctrl_reg |= (0x1<<9);
    }
    else
    {
        iis_tx_ctrl_reg &= ~(0x1<<9);
    }
    IISx->IISTXCTRL = iis_tx_ctrl_reg;
}



/**
 * @brief IIS TX静音设置
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param Cha 声道选择
 * @param cmd 静音或解除静音
 */
void IIS_TXMute(IIS_TypeDef* IISx,IIS_TX_CHANNAL Cha,FunctionalState cmd)
{
    if(IIS_TXCHANNAL_All == Cha)
    {
        IISx->IISTXCTRL &= ~(3<<7);
        IISx->IISTXCTRL |= (cmd<<7);
        IISx->IISTXCTRL |= (cmd<<8);
    }
    else if(IIS_TXCHANNAL_Left == Cha)
    {
        IISx->IISTXCTRL &= ~(1<<7);
        IISx->IISTXCTRL |= (cmd<<7);
    }
    
    else if(IIS_TXCHANNAL_Right == Cha)
    {
        IISx->IISTXCTRL &= ~(1<<8);
        IISx->IISTXCTRL |= (cmd<<8);
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
}


/**
 * @brief IIS发送控制寄存器初始化配置
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param datawidebits 数据宽度
 * @param fifolevel 发送FIFO触发深度
 * @param chnum 发送通道选择
 */
void IIS_TXCTRLConfig(IIS_TypeDef* IISx,IIS_TXRXDATAWIDEx datawidebits ,
    IIS_TXFIFOLEVELx fifolevel,IIS_TXCHNUMx chnum)
{
    IISx->IISTXCTRL &=~(0x3F<<1);
    IISx->IISTXCTRL |=((datawidebits & 0x7) <<4)|((fifolevel & 0x1)<<3)
                    |((chnum & 0x3)<<1);
}


/**
 * @brief IIS发送通道使能配置
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param cmd 使能或禁止
 */
void IIS_TXEN(IIS_TypeDef* IISx,FunctionalState cmd)
{
    uint32_t iis_tx_ctrl_reg = IISx->IISTXCTRL;
    if(cmd!=ENABLE)
    {
        iis_tx_ctrl_reg &=~(1<<0);
    }
    else
    {
        iis_tx_ctrl_reg |=(1<<0);	
    }
    IISx->IISTXCTRL = iis_tx_ctrl_reg;
}


/**
 * @brief IIS接收静音使能配置
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param cmd 静音或解除静音
 */
void IIS_RX_MuteEN(IIS_TypeDef* IISx,FunctionalState cmd)
{
    if(cmd != ENABLE)
    {
        IISx->IISRXCTRL &=~(1<<26);	
    }
    else
    {
        IISx->IISRXCTRL |=(1<<26);
    }
}


/**
 * @brief IIS RX FIFO清除
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param cmd 使能或不使能
 */
void IIS_RXFIFOClear(IIS_TypeDef* IISx,FunctionalState cmd)
{
    if(cmd != ENABLE)
    {
        IISx->IISRXCTRL &=~(1<<25);	
    }
    else
    {
        IISx->IISRXCTRL |=(1<<25);
    }
}


/**
 * @brief IIS接收控制寄存器初始化配置
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param datawidebits 数据宽度选择
 * @param fifolevel 接受FIFO触发深度选择
 */
void IIS_RXCTRLConfig(IIS_TypeDef* IISx,IIS_TXRXDATAWIDEx datawidebits,IIS_RXFIFOLEVELx  fifolevel)
{
    IISx->IISRXCTRL &=~(0X1F<<4);
    IISx->IISRXCTRL |=((datawidebits & 0x7)<<6)|
    ((fifolevel & 0x3)<<4);
}


/**
 * @brief IIS接收通道使能
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param cmd 使能或者禁止
 */
void IIS_RXEN(IIS_TypeDef* IISx,FunctionalState cmd)
{
    if(cmd !=ENABLE)
    {
        IISx->IISRXCTRL &=~(1<<0);	
    }
    else
    {
        IISx->IISRXCTRL |=(1<<0);
    }
}


/**
 * @brief IIS总线SCK与LRCK比例关系配置
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param iisscklrckrate SCK与LRCK比例选择，32或者64
 */
void IIS_BUSSCK_LRCKConfig(IIS_TypeDef* IISx,iis_bus_sck_lrckx_t iisscklrckrate)
{
    IISx->IISGBCTRL &=~(0x1<<5);	
    IISx->IISGBCTRL	|=((iisscklrckrate & 0x1)<<5);
}


/**
 * @brief 配置IIS发送/接收数据格式
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param iisdatafmt 数据格式选择
 */
void IIS_RXTXDATAFMTConfig(IIS_TypeDef* IISx,IIS_TXRXDATAFMTx iisdatafmt)
{
    if(iisdatafmt>=IIS_TXDATAFMT_IIS)
    {
        IISx->IISGBCTRL &=~(0x3<<1);	
        IISx->IISGBCTRL |=(((iisdatafmt - IIS_TXDATAFMT_IIS) & 0x3)<<1);	
    }
    else
    {
        IISx->IISGBCTRL &=~(0x3<<3);
        IISx->IISGBCTRL |=((iisdatafmt & 0x3)<<3);
    }
}


/**
 * @brief IIS全局控制器使能
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param cmd 使能或者禁止
 */
void IIS_EN(IIS_TypeDef* IISx,FunctionalState cmd)
{
    uint32_t iis_gb_ctrl_reg = IISx->IISGBCTRL;
    if(cmd!=ENABLE)
    {
        iis_gb_ctrl_reg	&=~(1<<0);
    }
    else
    {
        iis_gb_ctrl_reg |= (1<<0);
    }
    IISx->IISGBCTRL = iis_gb_ctrl_reg;
}


/**
 * @brief IIS输入输出选择，并使能或者禁止
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param iischselinout 输入输出配置
 * @param cmd 使能或禁止
 */
void IIS_CHSELConfig(IIS_TypeDef* IISx,IIS_CHSELAUDIOINOUTx iischselinout,FunctionalState cmd)
{
    if(cmd!=ENABLE)
    {
        IISx->IISCHSEL &=~(1<<((iischselinout ==IIS_AUDIO_INPUT)?3:1));
    }
    else
    {
        IISx->IISCHSEL |=(1<<((iischselinout ==IIS_AUDIO_INPUT)?3:1));
    }
}


/**
 * @brief IIS接收模式及merge选择
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param merge RX merge功能选择
 * @param lr_sel merge之后，左声道在高位还是右声道在高位
 * @param iisrxmode 数据单双声道选择
 */
void IIS_RXMODEConfig(IIS_TypeDef* IISx,IIS_MERGEx merge,IIS_LR_SELx lr_sel,IIS_RXMODEx iisrxmode)
{
    IISx->IISRXCTRL &= ~((0x3<<16)|(1<<24));
    IISx->IISRXCTRL |=((merge & 0x1)<< 24)|
            ((lr_sel & 0x1)<<17)|((iisrxmode & 0x1)<<16);
}


/**
 * @brief IIS发送初始化
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param IIS_DMA_Init_Struct IIS_DMA_TXInit_Typedef类型结构体指针
 * @param mode 0：IIS为master模式；
 *             2：IIS为slave模式
 */
void IISx_TXInit(IIS_TypeDef* IISx, IIS_DMA_TXInit_Typedef* IIS_DMA_Init_Struct)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;
    if(IIS0==IISx)
    {
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        // Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
        //CI_ASSERT(0,"para_err\n");
    }
    else if(IIS1==IISx)
    {
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);
    }
    else if(IIS2==IISx)
    {
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);
    }
    else if(IIS3==IISx)
    {
        iisdmacha = IISCha2;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
        
    IISDMA_DMICModeConfig(IISDMA, DISABLE);
    IISDMA_ChannelENConfig(IISDMA, iisdmacha,IISxDMA_TX_EN,DISABLE);
    IISDMA_ADDRRollBackINT(IISDMA, iisdmacha,IISxDMA_TX_EN,DISABLE);
    IISDMA_ChannelIntENConfig(IISDMA, iisdmacha,IISxDMA_TX_EN,DISABLE);   
    IISxDMA_TADDR0(IISDMA, iisdmacha,IIS_DMA_Init_Struct->txaddr0);
    IISxDMA_TADDR1(IISDMA, iisdmacha,IIS_DMA_Init_Struct->txaddr1);
    //IISxDMA_TXADDRRollbackInterruptClear(iisdma,IISDMA_TXAAD_Sel_ADDR0);
    IISxDMA_TNUM0(IISDMA, iisdmacha,IIS_DMA_Init_Struct->rollbackaddr0size,IIS_DMA_Init_Struct->tx0singlesize);
    IISxDMA_TNUM1(IISDMA, iisdmacha,IIS_DMA_Init_Struct->rollbackaddr1size,IIS_DMA_Init_Struct->tx1singlesize);
    
    IIS_BUSSCK_LRCKConfig(IISx,IIS_DMA_Init_Struct->sck_lrck);
    IIS_RXTXDATAFMTConfig(IISx,IIS_DMA_Init_Struct->txdatafmt);
    
    IIS_TXEN(IISx,DISABLE);
    IIS_TXCTRLConfig(IISx,IIS_DMA_Init_Struct->txdatabitwide,IIS_TXFIFOLEVEL1_2,IIS_TXCHNUM_STEREO);

    IIS_CHSELConfig(IISx,IIS_AUDIO_OUTPUT,ENABLE);
    IIS_TXEN(IISx,ENABLE);
    IIS_EN(IISx,ENABLE);
    IISDMA_EN(IISDMA, ENABLE);
    IISDMA_ChannelENConfig(IISDMA, iisdmacha,IISxDMA_TX_EN,ENABLE);
    IISDMA_ADDRRollBackINT(IISDMA, iisdmacha,IISxDMA_TX_EN,ENABLE);
} 


/**
 * @brief IIS接收模式初始化
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param IIS_DMA_Init_Struct IIS_DMA_RXInit_Typedef类型结构体指针
 * @param mode 0：IIS为master模式；2：IIS为slave模式
 */
void IISx_RXInit(IIS_TypeDef* IISx,IIS_DMA_RXInit_Typedef* IIS_DMA_Init_Struct)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;    
    if(IIS0==IISx)
    {
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        // Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
        //CI_ASSERT(0,"para_err\n");
    }
    else if(IIS1==IISx)
    {
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else if(IIS2==IISx)
    {
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else if(IIS3==IISx)
    {
        iisdmacha = IISCha2;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
    
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    IISDMA_ChannelIntENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    /*接受地址位rxaddr*/
    IISxDMA_RADDR(IISDMA,iisdmacha,IIS_DMA_Init_Struct->rxaddr);
    IISxDMA_RNUM(IISDMA,iisdmacha,IIS_DMA_Init_Struct->rxinterruptsize,
    IIS_DMA_Init_Struct->rollbackaddrsize,IIS_DMA_Init_Struct->rxsinglesize);
    /*关闭RX*/
    IIS_RXEN(IISx,DISABLE);
    /*传输数据的宽度为16位，FIFO深度*/
    IIS_RXCTRLConfig(IISx,IIS_DMA_Init_Struct->rxdatabitwide,IIS_RXFIFOLEVEL1_4);
    /*sck/lrck=*/
    IIS_BUSSCK_LRCKConfig(IISx,IIS_DMA_Init_Struct->sck_lrck);
    IIS_RXTXDATAFMTConfig(IISx,IIS_DMA_Init_Struct->rxdatafmt);
    IIS_CHSELConfig(IISx,IIS_AUDIO_INPUT,ENABLE);
    IIS_EN(IISx,ENABLE);
    /*打开RX*/
    IIS_RXEN(IISx,ENABLE);
    /*打开IIS的DMA通道*/
    IISDMA_EN(IISDMA,ENABLE);
    /*打开IIS的DMA接收使能*/
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,ENABLE);
    /*打开IISDMA接收 BusMatrix完成一次地址切换的中断使能*/
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_RX_EN,ENABLE);
}


/**
 * @brief IIS接收初始化，但是不使能IIS和IISDMA
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param IIS_DMA_Init_Struct IIS_DMA_RXInit_Typedef结构体指针
 * @param mode 0：IIS为master模式；
 *             2：IIS为slave模式
 */
void IISx_RXInit_noenable(IIS_TypeDef* IISx,IIS_DMA_RXInit_Typedef* IIS_DMA_Init_Struct)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;    
    #if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    if(IIS0==IISx)
    {
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        // Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
        //CI_ASSERT(0,"para_err\n");
    }
    else if(IIS1==IISx)
    {
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else if(IIS2==IISx)
    {
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else if(IIS3==IISx)
    {
        iisdmacha = IISCha2;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
    #else
    iisdmacha = IISCha2;
    IISDMA = IISDMA0;
    Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    #endif
    
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    IISDMA_ChannelIntENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    /*接受地址位rxaddr*/
    IISxDMA_RADDR(IISDMA,iisdmacha,IIS_DMA_Init_Struct->rxaddr);
    IISxDMA_RNUM(IISDMA,iisdmacha,IIS_DMA_Init_Struct->rxinterruptsize,
    IIS_DMA_Init_Struct->rollbackaddrsize,IIS_DMA_Init_Struct->rxsinglesize);
    /*关闭RX*/
    IIS_RXEN(IISx,DISABLE);
    /*传输数据的宽度为16位，FIFO深度*/
    IIS_RXCTRLConfig(IISx,IIS_DMA_Init_Struct->rxdatabitwide,IIS_RXFIFOLEVEL1_4);
    /*sck/lrck=*/
    IIS_BUSSCK_LRCKConfig(IISx,IIS_DMA_Init_Struct->sck_lrck);
    IIS_RXTXDATAFMTConfig(IISx,IIS_DMA_Init_Struct->rxdatafmt);
    IIS_CHSELConfig(IISx,IIS_AUDIO_INPUT,ENABLE);

    /*打开IIS的DMA通道*/
    IISDMA_EN(IISDMA,ENABLE);
}


/**
 * @brief IIS接收使能（中断模式）
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 */
void IISx_RX_enable_int(IIS_TypeDef* IISx)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;
    #if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    if(IIS0==IISx)
    {
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        //CI_ASSERT(0,"para_err\n");
    }
    else if(IIS1==IISx)
    {
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
    }
    else if(IIS2==IISx)
    {
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
    }
    else if(IIS3==IISx)
    {
        iisdmacha = IISCha2;
        IISDMA = IISDMA0;
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
    #else
    iisdmacha = IISCha2;
    IISDMA = IISDMA0;
    #endif

    /*打开IIS的DMA接收使能*/
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,ENABLE);
    /*打开IISDMA接收 BusMatrix完成一次地址切换的中断使能*/
//    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_RX_EN,ENABLE);

    IISDMA_ChannelIntENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,ENABLE);

    IIS_EN(IISx,ENABLE);
    /*打开RX*/
    IIS_RXEN(IISx,ENABLE);
}


/**
 * @brief IIS接收使能（直通模式）
 * 
 * @param IISx CI112X：IISx IIS1、IIS2、IIS3可选
 */
void IISx_RX_enable_straight_to_vad(IIS_TypeDef* IISx)
{
    // IISChax iisdmacha = IISCha0;
    // IISDMA_TypeDef* IISDMA = IISDMA0;
    
    // if(IIS0==IISx)
    // {
    //     // iisdmacha = IISCha0;
    //     // IISDMA = IISDMA0;
    //     CI_ASSERT(0,"para_err\n");
    // }
    // else if(IIS1==IISx)
    // {
    //     iisdmacha = IISCha0;
    //     IISDMA = IISDMA0;
    // }
    // else if(IIS2==IISx)
    // {
    //     iisdmacha = IISCha1;
    //     IISDMA = IISDMA0;
    // }
    // else if(IIS3==IISx)
    // {
    //     iisdmacha = IISCha2;
    //     IISDMA = IISDMA0;
    // }
    // else
    // {
    //     CI_ASSERT(0,"iisx err!\n");
    // }

    // IIS_EN(IISx,ENABLE);
    // /*打开RX*/
    // IIS_RXEN(IISx,ENABLE);

    // /*打开IIS的DMA接收使能*/
    // IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,ENABLE);
    // /*打开IISDMA接收 BusMatrix完成一次地址切换的中断使能*/
    // IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
}


/**
 * @brief 关闭IIS接收使能
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 */
void IISx_RX_disable(IIS_TypeDef* IISx)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;
    #if !AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
    if(IIS0==IISx)
    {
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        CI_ASSERT(0,"para_err\n");
    }
    else if(IIS1==IISx)
    {
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
    }
    else if(IIS2==IISx)
    {
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
    }
    else if(IIS3==IISx)
    {
        iisdmacha = IISCha2;
        IISDMA = IISDMA0;
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
    #else
    iisdmacha = IISCha2;
    IISDMA = IISDMA0;
    #endif

    IIS_EN(IISx,DISABLE);
    /*打开RX*/
    IIS_RXEN(IISx,DISABLE);

    /*打开IIS的DMA接收使能*/
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
    /*打开IISDMA接收 BusMatrix完成一次地址切换的中断使能*/
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_RX_EN,DISABLE);
}


/**
 * @brief IIS发送模式初始化
 * 
 * @param IISx CI112X：IIS1、IIS2、IIS3可选
 * @param IIS_DMA_Init_Struct IIS_DMA_TXInit_Typedef类型结构体指针
 * @param mode 0：IIS作为master模式；2：IIS作为slave模式
 */
void IISx_TXInit_noenable(IIS_TypeDef* IISx,IIS_DMA_TXInit_Typedef* IIS_DMA_Init_Struct)
{
    IISChax iisdmacha = IISCha0;
    IISDMA_TypeDef* IISDMA = IISDMA0;    
    if(IIS0==IISx)
    {
        // iisdmacha = IISCha0;
        // IISDMA = IISDMA0;
        // Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
        CI_ASSERT(0,"para_err\n");
    }
    else if(IIS1==IISx)
    {
        iisdmacha = IISCha0;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else if(IIS2==IISx)
    {
        iisdmacha = IISCha1;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else if(IIS3==IISx)
    {
        iisdmacha = IISCha2;
        IISDMA = IISDMA0;
        Scu_SetDeviceGate((unsigned int)IISDMA0,ENABLE);    //必须打开IISDMA的时钟
    }
    else
    {
        CI_ASSERT(0,"iisx err!\n");
    }
    
    IISDMA_DMICModeConfig(IISDMA,DISABLE);
    IISDMA_ChannelENConfig(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);
    IISDMA_ADDRRollBackINT(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);
    IISDMA_ChannelIntENConfig(IISDMA,iisdmacha,IISxDMA_TX_EN,DISABLE);   
    IISxDMA_TADDR0(IISDMA,iisdmacha,IIS_DMA_Init_Struct->txaddr0);
    IISxDMA_TADDR1(IISDMA,iisdmacha,IIS_DMA_Init_Struct->txaddr1);
    //IISxDMA_TXADDRRollbackInterruptClear(iisdma,IISDMA_TXAAD_Sel_ADDR0);
    IISxDMA_TNUM0(IISDMA,iisdmacha,IIS_DMA_Init_Struct->rollbackaddr0size,IIS_DMA_Init_Struct->tx0singlesize);
    IISxDMA_TNUM1(IISDMA,iisdmacha,IIS_DMA_Init_Struct->rollbackaddr1size,IIS_DMA_Init_Struct->tx1singlesize);
    
    IIS_BUSSCK_LRCKConfig(IISx,IIS_DMA_Init_Struct->sck_lrck);
    IIS_RXTXDATAFMTConfig(IISx,IIS_DMA_Init_Struct->txdatafmt);
    
    IIS_TXEN(IISx,DISABLE);
    IIS_TXCTRLConfig(IISx,IIS_DMA_Init_Struct->txdatabitwide,IIS_TXFIFOLEVEL1_2,IIS_TXCHNUM_STEREO);

    IIS_CHSELConfig(IISx,IIS_AUDIO_OUTPUT,ENABLE);
}


/**
 * @brief IIS看门狗使能
 * 
 * @param iis CI112X：IIS1、IIS2、IIS3可选
 * @param Cmd 使能或者禁止
 */
void IIS_WatchDog_Init(IIS_TypeDef* iis,FunctionalState Cmd)
{
    if(ENABLE == Cmd)
    {
        iis->IISINT |= 1<<5;
        iis->IISCNT &= ~(0x1ffff<<0);
        iis->IISCNT |= (1<<16)|(0xffff<<0);
    }
    else
    {
        iis->IISINT &= ~(1<<5);
        iis->IISCNT &= ~(0x1ffff<<0);
        iis->IISCNT &= ~(1<<16);
    }
}

/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/ 

