/**
 * @file ci112x_scu.h
 * @brief 系统配置
 * @version 1.0
 * @date 2018-04-09
 *
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 *
 */
#ifndef _SCU_H_
#define _SCU_H_

#include "ci112x_system.h"

#ifdef __cplusplus
    extern "C"{
#endif


#define SRAM3_CLKGATE      (0x1 << 1)
#define SRAM0_CLKGATE      (0x1 << 2)
#define SRAM1_CLKGATE      (0x1 << 3)
#define STCLK_CLKGATE      (0x1 << 5)
#define CM4CORE_CLKGATE    (0x1 << 6)
#define CM4_CLKGATE        (0x1 << 7)
#define SLEEP_CLKGATE      (0x1 << 8)
#define SLEEPDEEP_CLKGATE  (0x1 << 9)


/*scu reg PRE_CLKGATE0*/
#define PLL_CLKGATE0       (0x1 << 0)
#define IWDG_CLKGATE0      (0x1 << 1)
#define TWDG_CLKGATE0      (0x1 << 2)
#define IWDG_HALT_CLKGATE0 (0x1 << 3)
#define TWDG_HALT_CLKGATE0 (0x1 << 4)
#define DNN_CLKGATE0       (0x1 << 5)
#define QSPI_CLKGATE0      (0x1 << 8)
#define ADC_CLKGATE0       (0x1 << 9)
#define ALC_CLKGATE0       (0x1 << 10)
#define CODEC_CLKGATE0     (0x1 << 11)
#define IISDMA0_CLKGATE0   (0x1 << 12)
#define GDMA_CLKGATE0      (0x1 << 13)

/*scu reg PRE_CLKGATE1*/
#define MCLK1_CLKGATE1      (0x1 << 0)
#define MCLK2_CLKGATE1      (0x1 << 1)
#define MCLK3_CLKGATE1      (0x1 << 2)
#define IIS1_CLKGATE1       (0x1 << 3)
#define IIS2_CLKGATE1       (0x1 << 4)
#define IIS3_CLKGATE1       (0x1 << 5)
#define TIMER0_CLKGATE1     (0x1 << 6)
#define TIMER1_CLKGATE1     (0x1 << 7)
#define TIMER2_CLKGATE1     (0x1 << 8)
#define TIMER3_CLKGATE1     (0x1 << 9)
#define GPIO0_CLKGATE1      (0x1 << 10)
#define GPIO1_CLKGATE1      (0x1 << 11)
#define GPIO2_CLKGATE1      (0x1 << 12)
#define GPIO3_CLKGATE1      (0x1 << 13)
#define GPIO4_CLKGATE1      (0x1 << 14)
#define UART0_CLKGATE1      (0x1 << 15)
#define UART1_CLKGATE1      (0x1 << 16)
#define PWM0_CLKGATE1       (0x1 << 17)
#define PWM1_CLKGATE1       (0x1 << 18)
#define PWM2_CLKGATE1       (0x1 << 19)
#define PWM3_CLKGATE1       (0x1 << 20)
#define PWM4_CLKGATE1       (0x1 << 21)
#define PWM5_CLKGATE1       (0x1 << 22)
#define IIC0_CLKGATE1       (0x1 << 23)

typedef enum
{
/***-----PAD-----***********-----1st-----****-----2nd-----******----3rd----********----4th----******----内部上下拉----***/
    UART0_TX_PAD    = 0,   /*  GPIO2[0]        UART0_TX                                                   PBCU4RT      */
    UART0_RX_PAD    = 1,   /*  GPIO1[7]        UART0_RX                                                   PBCU4RT      */
    UART1_TX_PAD    = 2,   /*  GPIO2[1]        UART1_TX                                                   PBCU4RT      */
    UART1_RX_PAD    = 3,   /*  GPIO2[2]        UART1_RX                                                   PBCU4RT      */
    I2C0_SCL_PAD    = 4,   /*  GPIO3[0]        I2C0_SCL        UART1_TX                                   PBCU4RT      */
    I2C0_SDA_PAD    = 5,   /*  GPIO3[1]        I2C0_SDA        UART1_RX                                   PBCU4RT      */
    I2S1_MCLK_PAD   = 6,   /*  GPIO2[7]        I2S1_MCLK       I2S3_PROBE_MCLK     I2S2_PROBE_MCLK        PBCD4RT      */
    I2S1_SCLK_PAD   = 7,   /*  GPIO2[6]        I2S1_SCLK       I2S3_PROBE_SCK      I2S2_PROBE_SCK         PBCD4RT      */
    I2S1_SDO_PAD    = 8,   /*  GPIO2[5]        I2S1_SDO        I2S3_PROBE_SDO      I2S2_PROBE_SDO         PBCD4RT      */
    I2S1_LRCLK_PAD  = 9,   /*  GPIO2[4]        I2S1_LRCLK      I2S3_PROBE_LRCK     I2S2_PROBE_LRCK        PBCD4RT      */
    I2S1_SDI_PAD    = 10,  /*  GPIO2[3]        I2S1_SDI                            I2S2_PROBE_SDI         PBCD4RT      */
    GPIO0_6_PAD     = 11,  /*                  GPIO0[6]        UART1_RX                                   PBCD4RT      */
    GPIO0_5_PAD     = 13,  /*                  GPIO0[5]        UART1_TX                                   PBCU4RT      */
    PWM0_PAD        = 15,  /*  GPIO1[1]        PWM0            EXT_INT[0]                                 PBCD4RT      */
    PWM1_PAD        = 16,  /*  GPIO1[2]        PWM1            EXT_INT[1]                                 PBCD4RT      */
    PWM2_PAD        = 17,  /*  GPIO1[3]        PWM2            IPM_CS                                     PBCD4RT      */
    PWM3_PAD        = 18,  /*  GPIO1[4]        PWM3                                                       PBCD4RT      */
    PWM4_PAD        = 19,  /*  GPIO1[5]        PWM4                                                       PBCD4RT      */
    PWM5_PAD        = 20,  /*  GPIO1[6]        PWM5                                                       PBCD4RT      */
    AIN0_PAD        = 21,  /*  GPIO0[0]        PWM0                                                       PBCDA4R      */
    AIN1_PAD        = 22,  /*  GPIO0[1]        PWM1                                                       PBCDA4R      */
    AIN2_PAD        = 23,  /*  GPIO0[2]        PWM2                                                       PBCDA4R      */
    AIN3_PAD        = 24,  /*  GPIO0[3]        PWM3                                                       PBCDA4R      */
    GPIO0_7_PAD     = 25,  /*                  GPIO0[7]        PWM3                                       PBCU4RT      */
    GPIO1_0_PAD     = 26,  /*                  GPIO1[0]        PWM4                                       PBCD4RT      */
    GPIO0_4_PAD     = 27,  /*  GPIO0[4]        PWM2                                                       PBSD4RT      */
    GPIO3_2_PAD     = 28,  /*  GPIO3[2]        PWM4                                                       PBCD4RT      */
}PinPad_Name;


typedef enum
{
    FIRST_FUNCTION = 0,
    SECOND_FUNCTION = 1,
    THIRD_FUNCTION = 2,
    FORTH_FUNCTION = 3,
}IOResue_FUNCTION;

typedef enum
{
    ANALOG_MODE = 0,
    DIGITAL_MODE = 1,
}ADIOResue_MODE;


/**
 * @brief IIS配置SCK/LRCK的值
 *
 */
typedef enum
{
    //!SCK/LRCK = 32
    IIS_BUSSCK_LRCK32=0,
    //!SCK/LRCK = 64
    IIS_BUSSCK_LRCK64=1,
}iis_bus_sck_lrckx_t;


/**
 * @brief IIS主从模式选择
 *
 */
typedef enum
{
    //!IIS做SLAVE
    IIS_SLAVE = 1,
    //!IIS做MASTER
    IIS_MASTER = 0,
}iis_mode_sel_t;


typedef enum
{
    IIS2_TEST_MODE_DATA_FROM_INNER_IIS = 0,//数据来源于内部IIS
    IIS2_TEST_MODE_DATA_FROM_PAD = 1,//数据来源外部PAD
}iis2_test_mode_data_source_t;


/**
 * @brief IIS配置SCK反向
 *
 */
typedef enum
{
    IIS_SCK_EXT_NO_INV = 0,
    IIS_SCK_EXT_INV = 1,
}iis_sck_ext_inv_sel_t;


/**
 * @brief IIS过采样率设置
 *
 */
typedef enum
{
    //!IIS过采样率为128
    IIS_OVERSAMPLE_128Fs=0,
    //!IIS过采样率为192
    IIS_OVERSAMPLE_192Fs=1,
    //!IIS过采样率为256
    IIS_OVERSAMPLE_256Fs=2,
    //!IIS过采样率为384
    IIS_OVERSAMPLE_384Fs=3,
}iis_oversample_t;


typedef enum
{
    IIS_MCLK_OUT_ENABLE = 1,
    IIS_MCLK_OUT_DISABLE = 0,
}iis_mclk_out_en_t;


typedef enum
{
    IIS_CLK_SOURCE_FROM_IPCORE = 0,
    IIS_CLK_SOURCE_FROM_OSC = 1,
}iis_clk_source_sel_t;


/**
 * @brief IIS号选择
 *
 */
typedef enum
{
    //!IIS0
    IISNum0 = 0,
    //!IIS1
    IISNum1 = 1,
    //!IIS2
    IISNum2 = 2,
    //!IIS3
    IISNum3 = 3
}IISNUMx;


typedef struct
{
    IISNUMx device_select; //0: iis0 1:iis1 2: iis2
    int8_t div_param;//分频参数
    iis_mode_sel_t model_sel;//主从模式
    iis_sck_ext_inv_sel_t sck_ext_inv_sel;//SCK反向
    iis_bus_sck_lrckx_t sck_wid;//SCK和LRCK的比值
    iis_oversample_t over_sample;//过采样设置
    iis_mclk_out_en_t mclk_out_en;//mclk是否从PAD输出
    iis_clk_source_sel_t clk_source;//IIS的时钟源选择
}IIS_CLK_CONFIGTypeDef;



//add by hhx 1022
typedef enum
{
    CLK_OSC = 0,
    CLK_IPCORE = 1,
}CLK_SEL;

typedef enum
{
    DRV_0 = 0,
    DRV_90 = 1,
    DRV_180 = 2,
    DRV_270 = 3,
}DRV_SEL;

typedef enum
{
    SAMPLE_0 = 0,
    SAMPLE_90 = 1,
    SAMPLE_180 = 2,
    SAMPLE_270 = 3,
}SAMPLE_SEL;

enum
{
    CODE_RUN_WITH_SRAM = 0,
    CODE_RUN_WITH_PSRAM = 1,
    CODE_RUN_WITH_FLASH = 2,
};

typedef struct
{
    unsigned int osc_div;       //OSC(div_n36)时钟分频
    unsigned int coreclk_div;   //IPCORE(div_n37)时钟分频
    CLK_SEL      clk_sel;       //SDC的时钟选择，OSC或者IPCORE
    DRV_SEL      drv_sel;       //CLK_drv的相位选择
    SAMPLE_SEL   sample_sel;    //CLK_sample的相位选择
    unsigned int drv_sel_delay; //drv delay选择，表示延时的级数
    unsigned int sample_sel_delay;  //sample delay的选择，表示延时的级数
}SDC_CLK_CONFIGTypeDef;//add by hhx 1020

#define MAIN_FREQUENCY           188000000


#define PLL_DIV_BASE            (0x66666666)
#define STCLK_DIV_BASE          (0x44444444)


/*Scu function defines*/
int Scu_SetPll_CKIN(void);
void Unlock_Ckconfig(void);
void Unlock_Rstconfig(void);
void unlock_sysctrl_config(void);
void Lock_Ckconfig(void);
void Lock_Rstconfig(void);
void lock_sysctrl_config(void);

int Scu_Setdiv_Parameter(unsigned int device_base,unsigned int div_num);
int Scu_SetClkgate(unsigned int device_base,int gate);
int Scu_SetDeviceGate(unsigned int device_base, int gate);
void Scu_SetWDG_Halt(void);
void Scu_CleanWDG_Halt(void);
void Scu_SoftwareRst_System(void);
void Scu_SoftwareRst_SystemBus(void);
void Scu_Twdg_RstSys_Config(void);
void Scu_Twdg_RstSysBus_Config(void);
void Scu_Iwdg_RstSys_Config(void);
void Scu_Iwdg_RstSysBus_Config(void);
int Scu_Setdevice_Reset(unsigned int device_base);
int Scu_SetIISCLK_Config(IIS_CLK_CONFIGTypeDef *piis_config);
int Scu_SetIISCLK_Config(IIS_CLK_CONFIGTypeDef *piis_config);
void Scu_MaskScuInterrupt(FunctionalState cmd);
void Scu_SetWakeup_Source(int irq);
void Scu_SetNMIIRQ(int irq);
void Scu_SetIOReuse(PinPad_Name pin,IOResue_FUNCTION io_function);
void Scu_SetADIO_REUSE(PinPad_Name pin,ADIOResue_MODE adio_mode);
void Scu_AllPreipheralRst(void);
int Scu_Setdevice_ResetRelease(unsigned int device_base);
void Scu_AllPreipheralclkclose(void);
void Scu_AllPreipheralclkopen(void);

void scu_pll_12d288_config(uint32_t clk);
void iis_pll_12d288_config(uint32_t clk);

void scu_qspi_clksrc_choice(unsigned int device_base,uint32_t pll_src_en);

void scu_set_psram_clk_phase(uint32_t phase);

void scu_spiflash_noboot_set(void);
void ADC_SetIOReuse(uint8_t channel);
void Scu_SetIOPull(PinPad_Name pin,FunctionalState cmd);
int32_t Scu_GetPSramPadState(void);
void scu_iwdg_osc_clk(void);
void scu_iwdg_ipcore_clk(void);
void iis2_test_mode_set(iis_mode_sel_t mode,iis2_test_mode_data_source_t data);
void scu_run_in_flash(void);
int32_t Scu_GetSysResetState(void);
int8_t Scu_Para_En_Enable(uint32_t device_base);
int8_t Scu_Para_En_Disable(uint32_t device_base);

#ifdef __cplusplus
}
#endif

#endif  /*_SCU_H_ endif*/
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
