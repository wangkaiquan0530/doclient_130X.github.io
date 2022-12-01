/**
 * @file ci112x_system.h
 * @brief chip级定义
 * @version 1.0
 * @date 2018-05-21
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef _CI112X_SYSTEM_H_
#define _CI112X_SYSTEM_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******function return defines******/
#define INT32_T_MAX (0x7fffffff)
#define INT32_T_MIN (0x80000000)

enum _retval
{
    RETURN_OK = 0,       RET_SUCCESS = RETURN_OK,
    PARA_ERROR = -1,     RET_INVALIDARGMENT = PARA_ERROR,
    RETURN_ERR = -2,     RET_FAIL = RETURN_ERR,
                         RET_MOMEM = -3,
                         RET_READONLY = -4,
                         RET_OUTOFRANGE = -5,
                         RET_TIMEOUT = -6,
                         RET_NOTRANSFEINPROGRESS = -7,

                         RET_UNKNOW = INT32_T_MIN,
};


typedef enum IRQn
{
/******  RISC-V N307 Processor Exceptions Numbers *******************************/
    MSIP_IRQn                 = 3,      /*!<            */
    MTIP_IRQ                  = 7,      /*!<            */
/****** smt specific Interrupt Numbers ****************************************/
    TWDG_IRQn                 = 19,     /*!<            */
    IWDG_IRQn                 = 20,     /*!<            */
    FFT_IRQn                  = 21,     /*!<            */
    VAD_IRQn                  = 22,     /*!<            */
    DNN_IRQn                  = 23,     /*!<            */
    ADC_IRQn                  = 24,     /*!<            */
    GDMA_IRQn                 = 25,     /*!<            */
    IIS_DMA0_IRQn             = 26,     /*!<            */
    SCU_IRQn                  = 27,     /*!<            */
    EXTI0_IRQn                = 28,     /*!<            */
    EXTI1_IRQn                = 29,     /*!<            */
    TIMER0_IRQn               = 30,     /*!<            */
    TIMER1_IRQn               = 31,     /*!<            */
    TIMER2_IRQn               = 32,     /*!<            */
    TIMER3_IRQn               = 33,     /*!<            */
    UART0_IRQn                = 34,     /*!<            */
    UART1_IRQn                = 35,     /*!<            */
    IIC0_IRQn                 = 36,     /*!<            */
    GPIO0_IRQn                = 37,     /*!<            */
    GPIO1_IRQn                = 38,     /*!<            */
    GPIO2_IRQn                = 39,     /*!<            */
    GPIO3_IRQn                = 40,     /*!<            */
    GPIO4_IRQn                = 41,     /*!<            */
    IIS1_IRQn                 = 42,     /*!<            */
    IIS2_IRQn                 = 43,     /*!<            */
    IIS3_IRQn                 = 44,     /*!<            */
	ALC_IRQn                  = 45,     /*!<            */
	LVD_IRQn                  = 46,     /*!<            */
/********************************* END ****************************************/
} IRQn_Type;

#define IRQn_MAX_NUMBER (51)


typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))


#ifndef NULL
#define NULL 0
#endif

#define MASK_ROM_WINDOW_ADDR     (0x00001C00)                                       // sin窗函数
#define MASK_ROM_FFT_BIT_ADDR    (0x1F002000)                                       // 512fft的表1
#define MASK_ROM_FFT_COEF_R_ADDR (MASK_ROM_FFT_BIT_ADDR + sizeof(short)*440)       // 512fft的表2
#define MASK_ROM_FFT_COEF_ADDR   (MASK_ROM_FFT_COEF_R_ADDR + sizeof(float)*512)    // 512fft的表3
#define MASK_ROM_MEL_SCALE_ADDR  (MASK_ROM_FFT_COEF_ADDR + sizeof(float)*512)      // mel需要的表1
#define MASK_ROM_MEL_OFFSET_ADDR (MASK_ROM_MEL_SCALE_ADDR + sizeof(float)*512)     // mel需要的表2
#define MASK_ROM_ASR_WINDOW_ADDR (MASK_ROM_MEL_OFFSET_ADDR + sizeof(short)*(60))   // fe 需要的窗函数
#define MASK_ROM_PCM_TABLE_ADDR  (MASK_ROM_ASR_WINDOW_ADDR + sizeof(short)*(400))  // ADPCM需要用的表

/*IIC寄存器定义*/
typedef struct
{
    volatile uint32_t SCLDIV;   /*0x0  IIC SCL分频参数寄存器*/
    volatile uint32_t SRHLD;    /*0x4   IIC start条件保持时间*/
    volatile uint32_t DTHLD;    /*0x8   IIC SDA data time*/
    volatile uint32_t GLBCTRL;  /*0xc   IIC全局控制寄存器*/
    volatile uint32_t CMD;      /*0x10   IIC命令寄存器*/
    volatile uint32_t INTEN;    /*0x14  IIC中断使能寄存器*/
    volatile uint32_t INTCLR;   /*0x18  IIC中断清除寄存器*/
    volatile uint32_t SLAVDR;   /*0x1c IIC SLAVE地址寄存器*/
    volatile uint32_t TXDR;     /*0x20  IIC发送数据寄存器*/
    volatile uint32_t RXDR;     /*0x24  IIC接收数据寄存器*/
    volatile uint32_t TIMEOUT;  /*0x28  IIC超时寄存器*/
    volatile uint32_t STATUS;   /*0x2c  IIC状态寄存器*/
    volatile uint32_t BUSMON;   /*0x30  IIC总线信号监测寄存器*/
}I2C_TypeDef;


/*SCU寄存器定义*/
typedef struct
{
    volatile uint32_t SYS_CTRL_CFG;          /*0x0, */
    volatile uint32_t M0_STCALIB_CFG;        /*0x4, */
    volatile uint32_t INT_VEC_TAB_ADDR_CFG;  /*0x8, */
    volatile uint32_t EXT_INT_CFG;           /*0xc*/
    volatile uint32_t REV_SYR_CFG[16];

    volatile uint32_t SYSCFG_LOCK_CFG;       /*0x50*/
    volatile uint32_t RSTCFG_LOCK;           /*0x54, reset  reg lock reg*/
    volatile uint32_t CKCFG_LOCK;            /*0x58, clock config lock reg*/
    volatile uint32_t REV_LOCK_CFG[1];

    volatile uint32_t CLK_SEL_CFG;           /*0x60*/
    volatile uint32_t REV_CLK_SEL[1];
    volatile uint32_t SYS_PLL0_CFG;	         /*0x68*/
    volatile uint32_t SYS_PLL1_CFG;          /*0x6C*/
    volatile uint32_t REV_SYS_PLL[2];
    volatile uint32_t PLL_UPDATE_CFG;        /*0x78*/
    volatile uint32_t REV_PLL_UPDATE[1];

    volatile uint32_t CLKDIV_PARAM0_CFG;     /*0x80, */
    volatile uint32_t CLKDIV_PARAM1_CFG;     /*0x84, */
    volatile uint32_t CLKDIV_PARAM2_CFG;     /*0x88, */
    volatile uint32_t CLKDIV_PARAM3_CFG;     /*0x8C, */
    volatile uint32_t REV_CLKDIV_PAPAM[8];
    volatile uint32_t PARAM_EN0_CFG;         /*0xB0, */
    volatile uint32_t REV_PARAM_EN[8];

    volatile uint32_t IIS1_CLK_CFG;          /*0xD4, */
    volatile uint32_t IIS2_CLK_CFG;          /*0xD8, */
    volatile uint32_t IIS3_CLK_CFG;          /*0xDC, */
    volatile uint32_t RC_CLK_CFG;            /*0xE0*/
    volatile uint32_t REV_CLK_CFG[15];

    volatile uint32_t SYS_CLKGATE_CFG;       /*0x120, */
    volatile uint32_t PRE_CLKGATE0_CFG;      /*0x124, */
    volatile uint32_t PRE_CLKGATE1_CFG;      /*0x128, */
    volatile uint32_t REV_PRE_CLKGATE[17];

    volatile uint32_t SOFT_RST_PARAM;        /*0x170*/
    volatile uint32_t SOFT_SYSRST_CFG;       /*0x174*/
    volatile uint32_t SCU_STATE_REG;         /*0x178*/
    volatile uint32_t REV_RST_CFG[4];

    volatile uint32_t SOFT_PLLRST_CFG;       /*0x18c*/
    volatile uint32_t SOFT_PRERST0_CFG;      /*0x190*/
    volatile uint32_t SOFT_PRERST1_CFG;	     /*0x194*/
    volatile uint32_t REV_RST_CFG1[18];

    volatile uint32_t WAKEUP_MASK_CFG;       /*0x1e0*/
    volatile uint32_t EXT0_FILTER_CFG;       /*0x1e4*/
    volatile uint32_t EXT1_FILTER_CFG;       /*0x1e8*/
    volatile uint32_t REV_EXT_CFG[2];
    volatile uint32_t INT_STATE_REG;         /*0x1f4*/
    volatile uint32_t REV_EXT_CFG1[10];

    volatile uint32_t IO_REUSE0_CFG;         /*0x220*/
    volatile uint32_t IO_REUSE1_CFG;         /*0x224*/
    volatile uint32_t REV_IO_REUSE[2];
    volatile uint32_t AD_IO_REUSE0_CFG;      /*0x230*/
    volatile uint32_t REV_AD_IO_REUSE[1];
    volatile uint32_t IOPULL0_CFG;           /*0x238*/
    volatile uint32_t IOPULL1_CFG;           /*0x23c*/
	
	uint32_t recieve1[9]; 
	
    volatile uint32_t CODEC_TEST_CFG;
}SCU_TypeDef;


/*ADC寄存器定义*/
typedef struct
{
    volatile unsigned int ADCCTRL;//0x00
    volatile unsigned int ADCINTMASK;
    volatile unsigned int ADCINTFLG;
    volatile unsigned int ADCINTCLR;
    volatile unsigned int ADCSOFTSOC;//0x10
    volatile unsigned int ADCSOCCTRL;
    volatile unsigned int ADCRESULT[8];
    volatile unsigned int CHxPERIOD[4];
    volatile unsigned int CH0MINVALUE;
    volatile unsigned int CH0MAXVALUE;
    volatile unsigned int CH1MINVALUE;
    volatile unsigned int CH1MAXVALUE;
    volatile unsigned int CH2MINVALUE;
    volatile unsigned int CH2MAXVALUE;
    volatile unsigned int CH3MINVALUE;
    volatile unsigned int CH3MAXVALUE;
    volatile unsigned int ADCSTAT;
}ADC_TypeDef;




typedef struct
{
    volatile unsigned int UARTRdDR;         //0x0
    volatile unsigned int UARTWrDR;         //0x04
    volatile unsigned int UARTRxErrStat;    //0x08
    volatile unsigned int UARTFlag;         //0x0c
    volatile unsigned int UARTIBrd;         //0x10
    volatile unsigned int UARTFBrd;         //0x14
    volatile unsigned int UARTLCR;          //0x18
    volatile unsigned int UARTCR;           //0x1c
    volatile unsigned int UARTFIFOlevel;    //0x20
    volatile unsigned int UARTMaskInt;      //0x24
    volatile unsigned int UARTRIS;          //0x28
    volatile unsigned int UARTMIS;          //0x2c
    volatile unsigned int UARTICR;          //0x30
    volatile unsigned int UARTDMACR;        //0x34
    volatile unsigned int UARTTimeOut;      //0x38
    volatile unsigned int REMCR;            //0x3c
    volatile unsigned int REMTXDATA;        //0x40
    volatile unsigned int REMRXDATA;        //0x44
    volatile unsigned int REMINTCLR;        //0x48
    volatile unsigned int REMINTSTAE;       //0x4c
    volatile unsigned int BYTE_HW_MODE;     //0x50
}UART_TypeDef;



typedef struct
{
	volatile unsigned int DMACCxSrcAddr;
	volatile unsigned int DMACCxDestAddr;
	volatile unsigned int DMACCxLLI;
	volatile unsigned int DMACCxControl;
	volatile unsigned int DMACCxConfiguration;
	unsigned int reserved[3];
}DMACChanx_TypeDef;

typedef struct
{
	volatile unsigned int DMACIntStatus;
	volatile unsigned int DMACIntTCStatus;
	volatile unsigned int DMACIntTCClear;
	volatile unsigned int DMACIntErrorStatus;
	volatile unsigned int DMACIntErrClr;
	volatile unsigned int DMACRawIntTCStatus;
	volatile unsigned int DMACRawIntErrorStatus;
	volatile unsigned int DMACEnbldChns;
	volatile unsigned int DMACSoftBReq;
	volatile unsigned int DMACSoftSReq;
	volatile unsigned int DMACSoftLBReq;
	volatile unsigned int DMACSoftLSReq;
	volatile unsigned int DMACConfiguration;
	volatile unsigned int DMACSync;
	unsigned int reserved1[50];
	DMACChanx_TypeDef DMACChannel[8];
	unsigned int reserved2[195];
	volatile unsigned int DMACITCR;
	volatile unsigned int DMACITOP[3];
	unsigned int reserved3[693];
	volatile unsigned int DMACPeriphID[4];
	volatile unsigned int DMACPCellID[4];
}DMAC_TypeDef;


typedef struct
{
	unsigned int SrcAddr;
	unsigned int DestAddr;
	unsigned int NextLLI;
	unsigned int Control;
}DMAC_LLI;


typedef struct
{
    volatile unsigned int IISxDMARADDR;
    volatile unsigned int IISxDMARNUM;
    volatile unsigned int IISxDMATADDR0;
    volatile unsigned int IISxDMATNUM0;
    volatile unsigned int IISxDMATADDR1;
    volatile unsigned int IISxDMATNUM1;
}IISDMAChanx_TypeDef;

typedef struct
{
    volatile unsigned int IISDMACTRL;
    IISDMAChanx_TypeDef   IISxDMA[3];
    volatile unsigned int IISDMAPTR;
    volatile unsigned int IISDMASTATE;
    volatile unsigned int IISDMACLR;
    volatile unsigned int IISDMAIISCLR;
    volatile unsigned int IISDMARADDR[3];
    volatile unsigned int RX_VAD_CTRL;
	volatile unsigned int RX_LAST_ADDR;
	volatile unsigned int IIS_END_NUM_EN;
	volatile unsigned int IIS_END_NUM;
	volatile unsigned int DMA_REQ_CLR_STATE;
	volatile unsigned int DMATADDR[3];
}IISDMA_TypeDef;


typedef struct{
    volatile unsigned int IISTXCTRL;
    volatile unsigned int IISRXCTRL;
    volatile unsigned int IISGBCTRL;
    volatile unsigned int IISCHSEL;
    volatile unsigned int IISCHKEN;
    volatile unsigned int IISINT;
    volatile unsigned int IISCNT;
}IIS_TypeDef;


typedef struct
{
    volatile unsigned int reg0;
    unsigned int resver1;
    volatile unsigned int reg2;
    volatile unsigned int reg3;
    volatile unsigned int reg4;
    volatile unsigned int reg5;
    unsigned int resver2;
    volatile unsigned int reg7;
    volatile unsigned int reg8;
    volatile unsigned int reg9;
    volatile unsigned int rega;
    unsigned int resver4[22];
    volatile unsigned int reg21;
    volatile unsigned int reg22;
    volatile unsigned int reg23;
    volatile unsigned int reg24;
    volatile unsigned int reg25;
    volatile unsigned int reg26;
    volatile unsigned int reg27;
    volatile unsigned int reg28;
    volatile unsigned int reg29;
    volatile unsigned int reg2a;
    volatile unsigned int reg2b;
    volatile unsigned int reg2c;
    volatile unsigned int reg2d;
    volatile unsigned int reg2e;
    volatile unsigned int reg2f;
    volatile unsigned int reg30;
    unsigned int resver6[15];
    volatile unsigned int reg40;
    volatile unsigned int reg41;
    volatile unsigned int reg42;
    volatile unsigned int reg43;
    volatile unsigned int reg44;
    volatile unsigned int reg45;
    volatile unsigned int reg46;
    volatile unsigned int reg47;
    volatile unsigned int reg48;
    volatile unsigned int reg49;
    unsigned int resver7;
    volatile unsigned int reg4b;
    volatile unsigned int reg4c;
    volatile unsigned int reg4d;
    volatile unsigned int reg4e;
}CODEC_TypeDef;



typedef struct
{
    volatile uint32_t ALC_INT_STATUS;   /*0x00*/
    volatile uint32_t ALC_INT_EN;       /*0x04*/
    volatile uint32_t ALC_INT_CLR;      /*0x08*/
    volatile uint32_t GLB_CTRL;         /*0x0c*/
    volatile uint32_t LEFT_CTRL;        /*0x10*/
    volatile uint32_t L_STEP_TMR_ATK;    /*0x14*/
    volatile uint32_t L_STEP_TMR_DCY;    /*0x18*/
    volatile uint32_t L_CFG_ADDR;       /*0x1c*/
    volatile uint32_t L_MIN_MAX_DB;     /*0x20*/
    volatile uint32_t L_DB_STATUS;      /*0x24*/
    volatile uint32_t L_ATK_DCY_STATUS; /*0x28*/
    uint32_t reserve;
    volatile uint32_t RIGHT_CTRL;        /*0x30*/
    volatile uint32_t R_STEP_TMR_ATK;    /*0x34*/
    volatile uint32_t R_STEP_TMR_DCY;    /*0x38*/
    volatile uint32_t R_CFG_ADDR;       /*0x3c*/
    volatile uint32_t R_MIN_MAX_DB;     /*0x40*/
    volatile uint32_t R_DB_STATUS;      /*0x44*/
    volatile uint32_t R_ATK_DCY_STATUS; /*0x48*/

    volatile uint32_t PAUSE_STATUS;     /*0x4c*/
}ALC_TypeDef;


/*!< Peripheral memory map */

#define VAD_GATE 0x12ED45
#define FE_GATE	0x12e563
#define DNN_GATE 0x9829e

/* APB peripherals*/
#define AHB_BASE                (0xa5a5a5a5)
#define APB_BASE                (0x5a5a5a5a)
#define M4_SYSTICK_BASE         (0x77777777)
#define SD_DIV_BASE             (0x33333333)
#define DNN_DIV_BASE            (0x22222222)
#define TARCE_CLOCK_DIV         (0x55555555)
#define PSRAM_CLOCK_DIV         (0x43434343)
#define HAL_MCLK1_BASE          (0x99999999)
#define HAL_MCLK2_BASE          (0xaaaaaaaa)
#define HAL_MCLK3_BASE          (0xbbbbbbbb)
#define HAL_VAD_FFT_REG_BASE    (0xcccccccc)




#define HAL_QSPI0_BASE   (0x4004c000)

#define HAL_UART1_BASE   (0x4004a000)
#define HAL_UART0_BASE   (0x40049000)

#define HAL_IIS3_BASE    (0x40048000)
#define HAL_IIS2_BASE    (0x40047000)
#define HAL_IIS1_BASE    (0x40046000)

#define HAL_IIS0_BASE    (0x40045000)

#define HAL_GPIO4_BASE   (0x40044000)
#define HAL_GPIO3_BASE   (0x40043000)
#define HAL_GPIO2_BASE   (0x40042000)
#define HAL_GPIO1_BASE   (0x40041000)
#define HAL_GPIO0_BASE   (0x40040000)

#define HAL_CODEC_BASE   (0x4003f000)
#define HAL_ALC_BASE     (0x40017000)

#define HAL_IIC0_BASE    (0x4003c000)

#define HAL_TWDG_BASE    (0x4003b000)
#define HAL_IWDG_BASE    (0x4003a000)

#define HAL_TIMER3_BASE  (0x40039000)
#define HAL_TIMER2_BASE  (0x40038000)
#define HAL_TIMER1_BASE  (0x40037000)
#define HAL_TIMER0_BASE  (0x40036000)

#define HAL_PWM5_BASE    (0X40035000)
#define HAL_PWM4_BASE    (0x40034000)
#define HAL_PWM3_BASE    (0x40033000)
#define HAL_PWM2_BASE    (0x40032000)
#define HAL_PWM1_BASE    (0x40031000)
#define HAL_PWM0_BASE    (0x40030000)

#define HAL_SCACHE_BASE  (0x4001e000)
#define HAL_ICACHE_BASE  (0x4001c000)

#define HAL_FFT_BASE     (0x4001a000)
#define HAL_SPI1_BASE    (0x40019000)
#define HAL_ALC_BASE     (0x40017000)
#define HAL_DNN_BASE     (0X40015000)
#define HAL_IISDMA1_BASE (0x40014000)
#define HAL_IISDMA0_BASE (0x40013000)
#define HAL_ADC_BASE     (0x40012000)
#define HAL_GDMA_BASE    (0x40011000)
#define HAL_SCU_BASE     (0x40010000)

#define HAL_VAD_BASE     (0x4001b200)

#define HAL_PSRAM_BASE   (0x40016000)

#define SPI0FIFO_BASE    (0x60000000)
#define SPI1FIFO_BASE    (0x61000000)
#define UART0FIFO_BASE   (0x63000000)
#define UART1FIFO_BASE   (0x64000000)


/** addtogroup Peripheral_declaration
  * @{
  */


#define SCU                     ((SCU_TypeDef*)HAL_SCU_BASE)
#define IIC0                    ((I2C_TypeDef*)HAL_IIC0_BASE)
#define ADC 					((ADC_TypeDef*)HAL_ADC_BASE)
#define UART0                   ((UART_TypeDef*)HAL_UART0_BASE)
#define UART1                   ((UART_TypeDef*)HAL_UART1_BASE)
#define	TWDG	                ((TWDG_TypeDef *)HAL_TWDG_BASE)
#define CODEC                   ((CODEC_TypeDef*)HAL_CODEC_BASE)
#define ALC                     ((ALC_TypeDef*)HAL_ALC_BASE)
#define DMAC                    ((DMAC_TypeDef*)HAL_GDMA_BASE)
#define IISDMA0                 ((IISDMA_TypeDef*)HAL_IISDMA0_BASE)
#define IISDMA1                 ((IISDMA_TypeDef*)HAL_IISDMA1_BASE)
#define IIS0                    ((IIS_TypeDef* )HAL_IIS0_BASE)
#define IIS1                    ((IIS_TypeDef* )HAL_IIS1_BASE)
#define IIS2                    ((IIS_TypeDef* )HAL_IIS2_BASE)
#define IIS3                    ((IIS_TypeDef* )HAL_IIS3_BASE)


/**
  * @}
  */


/******************************************************************************/
/*                         Peripheral Registers_Bits_Definition               */
/******************************************************************************/
/*SCU  SYS_CTRL*/
#define PLL_CONFIG_BEFORE       (0x1 << 0)
#define PLL_CONFIG_AFTER        (~(0x1 <<0))

/*sys clock gate define*/
#define SLEEPDEEP_MODE_GATE     0x11
#define SLEEPING_MODE_GATE      0x22
#define CM4_GATE                0x33
#define CM4CORE_GATE            0x44
#define STCLK_GATE              0x55
#define SRAM0_GATE              0x66
#define SRAM1_GATE              0x77
#define SRAM2_GATE              0x88
#define SRAM3_GATE              0x99
#define PWM8_GATE_BASE          0xaa
#define TRACE_GATE              0xbb
/*sys and device reset define*/
#define PLL_RST_BASE            0xcc




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/************************ (C) COPYRIGHT chipintelli *****END OF FILE****/











