/**
 * @file ci112x_it.h
 * @brief 中断服务函数
 * @version 1.0
 * @date 2018-05-21
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef __CI112X_IT_H
#define __CI112X_IT_H


#ifdef __cplusplus
 extern "C" {
#endif 

void TWDG_IRQHandler(void);
void IWDG_IRQHandler(void);
void fft_irqhandle(void);
void vad_irqhandle(void);
void dnn_irqhandle(void);
void ADC_IRQHandler(void);
void DMA_IRQHandler(void);
void IIS_DMA0_IRQHandler(void);
void SCU_IRQHandler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void TIM0_IRQHandler(void);
void TIM1_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void UART0_IRQHandler(void);                    
void UART1_IRQHandler(void);
void I2C0_IRQHandler(void);
void GPIO0_IRQHandler(void);
void GPIO1_IRQHandler(void);
void GPIO2_IRQHandler(void);
void GPIO3_IRQHandler(void);
void GPIO4_IRQHandler(void);
void IIS1_IRQHandler(void);
void IIS2_IRQHandler(void);
void IIS3_IRQHandler(void);

#ifdef __cplusplus
}
#endif 


#endif /* __CI112X_IT_H */
