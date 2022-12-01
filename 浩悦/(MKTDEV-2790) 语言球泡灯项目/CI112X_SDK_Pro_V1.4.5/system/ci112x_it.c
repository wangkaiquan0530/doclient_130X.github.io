/**
 * @file ci112x_it.c
 * @brief 中断服务函数
 * @version 1.0
 * @date 2018-05-21
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include "ci112x_it.h"
#include "ci112x_iwdg.h"
#include "ci112x_iisdma.h"
#include "ci112x_timer.h"
#include "ci112x_TimerWdt.h"
#include "ci112x_spiflash.h"
#include "ci112x_dma.h"
#include "ci112x_i2c.h"
#include "ci112x_uart.h"
#include "ci112x_audio_capture.h"
#include "ci112x_audio_play_card.h"
#include "ci112x_core_misc.h"
#include "FreeRTOS.h"
#include "ci_log.h"
#include "i2c_protocol_module.h"


/**
 * @brief timer看门狗中断处理函数
 * @note 在使用前需调用SCU相关接口初始化,复位系统使能
 * 
 */
void TWDG_IRQHandler(void)
{
    TWDG_service();
	mprintf("i am TWDG IRQ!\n");
}


/**
 * @brief 独立看门狗中断处理函数
 * @note 在使用前需调用SCU相关接口初始化,复位系统使能
 * 
 */
void IWDG_IRQHandler(void)
{
	/*清除中断标志*/
	iwdg_feed(IWDG);
    ci_logdebug(LOG_IWDT,"i am IWDG IRQ!\n");
}

__WEAK void ADC_IRQHandler(void){}


/**
 * @brief DMA中断服务程序
 * 
 */
void DMA_IRQHandler(void)
{
#if !DRIVER_OS_API
    dma_without_os_int();
#else
    dma_with_os_int();
#endif
}


/**
 * @brief IISDMA0中断处理函数
 * 
 */
void IIS_DMA0_IRQHandler(void)
{
    static BaseType_t xHigherPriorityTaskWoken;
    volatile unsigned int tmp;
    tmp = IISDMA0->IISDMASTATE;
    static uint32_t a = 0;
    static uint32_t b = 0;
    
    int8_t iis3_is_addr_rollback = 0;
    int8_t iis1_is_addr_rollback = 0;

    if (tmp & (1 << (1 + IISCha0*2 + IISxDMA_RX_EN)))//RX0
    {
        IISDMA_INT_Clear(IISDMA0,(1 << (1 + IISCha0*2 + IISxDMA_RX_EN)) );
        audio_cap_handler(AUDIO_CAP_IIS1,IISDMA0,IISCha0,&iis1_is_addr_rollback,&xHigherPriorityTaskWoken);
    }
    if (tmp & (1 << (7 + IISCha1 + IISxDMA_RX_EN * 3)))//RX1
    {
        //IISDMA_INT_Clear(IISDMA0,(1 << (7 + IISCha1 + IISxDMA_RX_EN * 3)) );
        //audio_cap_handler(AUDIO_CAP_IIS1,IISDMA0,IISCha1,&xHigherPriorityTaskWoken);
    }

    if (tmp & (1 << (1 + IISCha2*2 + IISxDMA_RX_EN )))//RX3
	{
		IISDMA_INT_Clear(IISDMA0,(1 + IISCha2*2 + IISxDMA_RX_EN ) );
		audio_cap_handler(AUDIO_CAP_IIS3,IISDMA0,IISCha2,&iis3_is_addr_rollback,&xHigherPriorityTaskWoken);
	}


    if (tmp & (1 << (7 + IISCha0 + IISxDMA_TX_EN * 3)))//TX1
    {
        IISDMA_INT_Clear(IISDMA0,(1 << (7 + IISCha0 + IISxDMA_TX_EN * 3)) );
        audio_play_card_handler(AUDIO_PLAY_IIS1,IISDMA0,IISCha0,&xHigherPriorityTaskWoken);
    }
    if (tmp & (1 << (7 + IISCha1 + IISxDMA_TX_EN * 3)))//TX2
    {
        IISDMA_INT_Clear(IISDMA0,(1 << (7 + IISCha1 + IISxDMA_TX_EN * 3)) );
        audio_play_card_handler(AUDIO_PLAY_IIS2,IISDMA0,IISCha1,&xHigherPriorityTaskWoken);
    }

    if( pdFALSE != xHigherPriorityTaskWoken )
    {
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
}

__WEAK void SCU_IRQHandler  (void){}
__WEAK void EXTI0_IRQHandler(void){}
__WEAK void EXTI1_IRQHandler(void){}


/**
 * @brief 定时器0中断服务函数
 * 
 */
void TIMER0_IRQHandler(void)
{
    timer_clear_irq(TIMER0);
    mprintf("TIMER0 IRQ\n");
}


/**
 * @brief 定时器1中断服务函数
 * 
 */
void TIMER1_IRQHandler(void)
{
	timer_clear_irq(TIMER1);
    mprintf("TIMER1 IRQ\n");
}


/**
 * @brief 定时器2中断服务函数
 * 
 */
void TIMER2_IRQHandler(void)
{
    timer_clear_irq(TIMER2);
    mprintf("TIMER2 IRQ\n");
}


/**
 * @brief 定时器3中断服务函数
 * 
 */
void TIMER3_IRQHandler(void)
{
    timer_clear_irq(TIMER3);
    mprintf("TIMER3 IRQ\n");
}


/**
 * @brief UART0中断服务程序
 *
 */
void UART0_IRQHandler(void)
{
    #if UART_LOG_UASED_INT_MODE
    #if ((HAL_UART0_BASE) == (CONFIG_CI_LOG_UART))
    uart_log_int_handler();
    #endif
    #endif

    /*发送数据*/
    if (UART0->UARTMIS & (1UL << UART_TXInt))
    {
        UART_IntClear(UART0,UART_TXInt);
    }
    /*接受数据*/
    if (UART0->UARTMIS & (1UL << UART_RXInt))
    {
        UART_IntClear(UART0,UART_RXInt);
    }

    UART_IntClear(UART0,UART_AllInt);
}


/**
 * @brief UART1中断服务程序
 *
 */
void UART1_IRQHandler(void)
{
    #if UART_LOG_UASED_INT_MODE
    #if ((HAL_UART1_BASE) == (CONFIG_CI_LOG_UART))
    uart_log_int_handler();
    #endif
    #endif

    if (UART_MaskIntState(UART1,UART_OverrunErrorInt))
    {
        mprintf("uart rev overrun!\n");
        UART_IntClear(UART1,UART_OverrunErrorInt);
    }

    /*发送数据*/
    if (UART_MaskIntState(UART1,UART_TXInt))
    {
        UART_IntClear(UART1,UART_TXInt);
    }

    /*接受数据*/
    if (UART_MaskIntState(UART1,UART_RXInt))
    {
        UART_IntClear(UART1,UART_RXInt);
    }

    UART_IntClear(UART1,UART_AllInt);
}


/**
 * @brief i2c0中断服务函数
 * 
 */
void I2C0_IRQHandler(void)
{
	extern uint32_t i2c0_is_master;
	extern uint32_t i2c0_send_count;
	extern struct i2c_data i2c0_master;
	static uint32_t i2c0_master_send = 0;

    struct i2c_data *i2c = &i2c0_master;
    volatile uint32_t i2c_status;
    i2c_status = IIC0->STATUS;
    IIC0->INTCLR = CR_IRQ_CLEAR;

    if ((i2c->algo_data.read_byte(i2c, IIC_STATUS) & SR_IRQ_FLAG) == 0)
    {
        i2c->algo_data.i2c_errors = i2c->algo_data.read_byte(i2c, IIC_STATUS);
    }
    if (i2c->algo_data.i2c_over == I2C_TRANS_OVER)
    {
        i2c->algo_data.i2c_errors &= ~SR_RXACK;
        i2c->algo_data.i2c_over = I2C_TRANS_ON;
    }

    if (i2c0_is_master == 1)     /*master */
    {
        if (i2c_status & STATUS_TRANSMITTER) /* 作为 master-transmitter */
        {
            i2c0_master_send = 1;
            if (i2c_status & STATUS_SEND_BYTE) /* 发送完 1 byte */
            {
                i2c0_send_count ++;
            }
        }
        else /* 作为 master-receiver */
        {
            i2c0_master_send = 0;
        }
        if (i2c0_master_send == 1) /* master-transmitter */
        {
            if (i2c_status & STATUS_NOACK) /* slave-receiver 发来 NOACK */
            {
                i2c0_send_count = 0;
                IIC0->CMD = (CR_STOP | CR_TB);
            }
            else /* slave-receiver 发来 ACK */
            {
            }
        }
        else /* master-receiver */
        {
            if (i2c_status & STATUS_NOACK) /* slave-transmitter 发来 NOACK */
            {
                i2c0_send_count = 0;
            }
            else /* slave-transmitter 发来 ACK */
            {
            }
        }
        iic_with_os_int(i2c); /* 设置中断标志 */
    }
    else /*slave */
    {
        #if MSG_USE_I2C_EN

    	volatile uint32_t recv_finish_flag = 0;
    	extern volatile uint8_t i2c_recv_status;
    	extern volatile uint8_t i2c0_reg;
    	extern uint32_t i2c0_recv_count;
    	static uint32_t i2c0_slave_send = 0;
    	extern volatile uint32_t send_count;

        if (i2c_status & STATUS_SLAVE_SELECTED)   /* 此slave被总线上其他master寻中,作为slave-transmitter*/
        {
            if(i2c_status & STATUS_WRITE)
            {
                i2c0_slave_send = 1;
            }
            else /* 作为 slave-receiver*/
            {
                i2c0_slave_send = 0;
            }
        }
        if (i2c0_slave_send == 0)  /* 作为 slave-receiver */
        {
            if(((i2c_status & STATUS_TRANSMITTER) == 0) && (i2c_status & STATUS_RXDR_FULL))   /* 接收寄存器满 */
            {
                if(i2c0_recv_count == 0)       /*第1个字节为寄存器地址*/
                {
                    i2c0_reg = IIC0->RXDR;    /*从接收寄存器读数据*/
                    i2c_recv_status = I2C_RECV_STATUS_REG;
                    send_count = 0;
                }

                #if IIC_PROTOCOL_CMD_ID
                if(i2c0_reg == I2C_REG_PLAY_CMDID)
                {
                   i2c_recv_cmdid_packet(IIC0->RXDR);  /*接收命令ID播报消息*/
                   i2c0_recv_count ++;              /*接收数据个数计数*/
                   if(i2c0_recv_count == IIC_RECV_CMDID_LEN)
                   {
                      recv_finish_flag = 1;
                   }
                }
                #endif

                #if IIC_PROTOCOL_SEC_ID
                if(i2c0_reg == I2C_REG_PLAY_SECID)
                {
                   i2c_recv_secid_packet(IIC0->RXDR);  /*接收语义ID播报消息*/
                   i2c0_recv_count ++;              /*接收数据个数计数*/
                   if(i2c0_recv_count == IIC_RECV_SECID_LEN)
                   {
                      recv_finish_flag = 1;
                   }
                }
                #endif

                IIC0->CMD = (CR_READ | CR_TB);
            }
            else    /* 接收寄存器未满 */
            {
                if (i2c0_recv_count == 0)
                {
                    IIC0->CMD = CR_TB;
                }
            }
        }
        else     /* (*i2cx_slave_send) = 1，作为 slave-transmitter */
        {
            if ((i2c_status & STATUS_NOACK) == 0) /* 其他 master-receiver 发来 ACK */
            {
                i2c_send_data(&i2c0_reg); /* IIC通信协议发送数据 */
            }
        }
        if ((i2c_status & STATUS_STOP) || (recv_finish_flag == 1)) /* 当总线有stop产生，或数据接收完成*/
        {
            send_count = 0;
            i2c_recv_status = I2C_RECV_STATUS_REG; 
            i2c0_recv_count = 0;
            i2c0_reg = 0xFF;
            IIC0->CMD = CR_TB;
        }

        #endif
    }
}


__WEAK void GPIO0_IRQHandler(void){}
__WEAK void GPIO1_IRQHandler(void){}
__WEAK void GPIO2_IRQHandler(void){}
__WEAK void GPIO3_IRQHandler(void){}
__WEAK void GPIO4_IRQHandler(void){}
__WEAK void IIS1_IRQHandler (void){}
__WEAK void IIS2_IRQHandler (void){}


/**
 * @brief IIS3中断处理函数
 * 
 */
void IIS3_IRQHandler(void)
{
    extern capture_equipment_state_t capture_equipment_state_str[MAX_MULTI_CODEC_NUM];
    volatile uint32_t tmp = 0;
    tmp = IIS3->IISINT;
    if (tmp & (1 << 2))
    {
        IIS3->IISINT |= (1 << 8);
        capture_equipment_state_str[AUDIO_CAP_IIS3 - 1].iis_watchdog_callback();
    }
}


__WEAK void ALC_IRQHandler (void){}
__WEAK void LVD_IRQHandler (void){}

/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
