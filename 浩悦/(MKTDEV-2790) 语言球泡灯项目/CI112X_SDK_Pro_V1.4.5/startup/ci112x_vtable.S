/**
 * @file ci112x_vtable.S
 * @brief ci112x中断向量表
 * @version 1.0.0
 * @date 2019-11-21
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */
	.section .vtable

    .weak  eclic_msip_handler 
    .weak  eclic_mtip_handler 

    .globl vector_base
vector_base:
    j _start                   //0
    .align 2
    .word  0                   //1
    .word  0                   //2
    .word  eclic_msip_handler  //3
    .word  0                   //4
    .word  0                   //5
    .word  0                   //6
    .word  eclic_mtip_handler  //7
    .word  0                   //8
    .word  0                   //9
    .word  0                   //10
    .word  0                   //11
    .word  0                   //12
    .word  0                   //13
    .word  0                   //14
    .word  0                   //15
    .word  0                   //16
    .word  0                   //17
    .word  0                   //18
    .word  TWDG_IRQHandler     //19
    .word  IWDG_IRQHandler     //20
    .word  0                   //21
    .word  0                   //22
    .word  dnn_irqhandle       //23
    .word  ADC_IRQHandler      //24
    .word  DMA_IRQHandler      //25
    .word  IIS_DMA0_IRQHandler //26
    .word  SCU_IRQHandler      //27
    .word  EXTI0_IRQHandler    //28
    .word  EXTI1_IRQHandler    //29
    .word  TIMER0_IRQHandler   //30
    .word  TIMER1_IRQHandler   //31
    .word  TIMER2_IRQHandler   //32
    .word  TIMER3_IRQHandler   //33
    .word  UART0_IRQHandler    //34
    .word  UART1_IRQHandler    //35
    .word  I2C0_IRQHandler     //36
    .word  GPIO0_IRQHandler    //37
    .word  GPIO1_IRQHandler    //38
    .word  GPIO2_IRQHandler    //39
    .word  GPIO3_IRQHandler    //40
    .word  GPIO4_IRQHandler    //41
    .word  IIS1_IRQHandler     //42
    .word  IIS2_IRQHandler     //43
    .word  IIS3_IRQHandler     //44
    .word  ALC_IRQHandler      //45
    .word  LVD_IRQHandler      //46
    .word  0 //47

    .word  0 //48
    .word  0 //49
    .word  0 //50
    .fill  13, 4, 0x0          //填充向量表到下一个对齐的地址以存放load_flag
