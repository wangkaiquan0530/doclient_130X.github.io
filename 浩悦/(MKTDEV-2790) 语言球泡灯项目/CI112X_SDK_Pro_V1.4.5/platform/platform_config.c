/**
 * @file platform_config.c
 * @brief 统一初始化io、中断优先级等
 * @version 1.0
 * @date 2019-06-12
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#include "platform_config.h"
#include "ci112x_system.h"
#include "ci112x_scu.h"
#include "ci112x_gpio.h"
#include "ci112x_core_eclic.h"
#include "sdk_default_config.h"
#include "ci_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define OSC_CLK   (12288000U)
static uint32_t ipcore_clk = 0;
static uint32_t ahb_clk = 0;
static uint32_t apb_clk = 0;
static uint32_t systick_clk = 0;
static uint32_t iispll_clk = 0;
static bool sg_pa_control_level_flag = false;//PA控制电平

/**
 *                             
 *                   AIN AIN AIN AIN VCC HPO VCM AGN HPO AVD MIC MIC AGN MIC  A  MIC MIC
 *                    0   1   2   3  33  UTR     DRV UTV DRV PRV NR  DRV BIA VDD NL  PL
 *                    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *                 +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *                 |  68  67  66  65  64  63  62  61  60  59  58  57  56  55  54  53  52  |
 *   PLL_AVDD12 ---+ 1                                                                 51 +---IIC1_SDA
 *   PLL_AVSS12 ---+ 2                                                                 50 +---IIC1_SCL
 *   XIN        ---+ 3                                                                 49 +---IIS1_MCLK
 *   XOUT       ---+ 4                                                                 48 +---IIS1_SCLK
 *   UART0_TX   ---+ 5                                                                 47 +---IIS1_SDO
 *   UART0_RX   ---+ 6                    ____________________  _____                  46 +---IIS1_LRCLK
 *   VCC12      ---+ 7                   / ____/  _<  <  / __ \/ ___/                  45 +---IIS1_SDI
 *   VCC33      ---+ 8                  / /    / / / // / / / / __ \                   44 +---VCC33
 *   I2C0_SDA   ---+ 9                 / /____/ / / // / /_/ / /_/ /                   43 +---VCC12
 *   I2C0_SCL   ---+ 10                \____/___//_//_/\____/\____/                    42 +---UART2_RX
 *   SPI1_CS    ---+ 11                                                                41 +---UART2_TX
 *   SPI1_DIN   ---+ 12                                                                40 +---UART1_RX
 *   SPI1_DOUT  ---+ 13                                                                39 +---UART1_TX
 *   SPI1_CLK   ---+ 14                                                                38 +---SWD_DAT
 *   SDC_CDATA1 ---+ 15                                                                37 +---SWD_CLK
 *   SDC_CDATA0 ---+ 16                                                                36 +---RSTn
 *   SDC_CCLK   ---+ 17                                                                35 +---TEST
 *                 |  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  |
 *                 +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *                    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *                   SDC SDC SDC SDC PWM PWM PWM PWM PWM PWM VCC SPI SPI SPI SPI SPI SPI
 *                   CMD D3  D2  DET  0   1   2   3   4   5  33  0CS 0D1 0D2 0D0 CLK 0D3
 * 
 */
static void init_pin_mux(void)
{

}

int vad_gpio_pad_cfg(void)
{
    //vad_light_init();
	return 0;
}

int vad_start_mark(void)
{
	//vad_light_on();
	return 0;
}

int vad_end_mark(void)
{
	//vad_light_off();
	return 0;
}

/**
 * @brief 配置系统中断优先级
 * 
 */
static void init_irq_pri(void)
{
    eclic_irq_set_priority(TWDG_IRQn, 7, 0);
    eclic_irq_set_priority(IWDG_IRQn, 7, 0);
    eclic_irq_set_priority(FFT_IRQn, 6, 0);
    eclic_irq_set_priority(VAD_IRQn, 6, 0);
    eclic_irq_set_priority(DNN_IRQn, 6, 0);
    eclic_irq_set_priority(ADC_IRQn, 6, 0);
    eclic_irq_set_priority(GDMA_IRQn, 6, 0);
    eclic_irq_set_priority(IIS_DMA0_IRQn, 6, 0);
    eclic_irq_set_priority(SCU_IRQn, 6, 0);
    eclic_irq_set_priority(EXTI0_IRQn, 6, 0);
    eclic_irq_set_priority(EXTI1_IRQn, 6, 0);
    eclic_irq_set_priority(TIMER0_IRQn, 6, 0);
    eclic_irq_set_priority(TIMER1_IRQn, 6, 0);
    eclic_irq_set_priority(TIMER2_IRQn, 6, 0);
    eclic_irq_set_priority(TIMER3_IRQn, 6, 0);
    eclic_irq_set_priority(UART0_IRQn, 6, 0);
    eclic_irq_set_priority(UART1_IRQn, 6, 0);
    eclic_irq_set_priority(IIC0_IRQn, 6, 0);
    eclic_irq_set_priority(GPIO0_IRQn, 6, 0);
    eclic_irq_set_priority(GPIO1_IRQn, 6, 0);
    eclic_irq_set_priority(GPIO2_IRQn, 6, 0);
    eclic_irq_set_priority(GPIO3_IRQn, 6, 0);
    eclic_irq_set_priority(GPIO4_IRQn , 6, 0);
    eclic_irq_set_priority(IIS1_IRQn, 6, 0);
    eclic_irq_set_priority(IIS2_IRQn, 6, 0);
    eclic_irq_set_priority(IIS3_IRQn, 6, 0);
}


/**
 * @brief 配置总线时钟
 * 
 */
void init_clk_div(void)
{   
    /* PLL 328M ip_core 180M */
    //scu_pll_12d288_config(360000000);  //bootloader已配置完成
    set_ipcore_clk(MAIN_FREQUENCY);
    
    /* AHB 180M */
    set_ahb_clk(MAIN_FREQUENCY);

    /* APB 90M */
    set_apb_clk(MAIN_FREQUENCY/2);

    /* 内核timer时钟 7.5M x 2(双边沿) */
    set_systick_clk(MAIN_FREQUENCY/12);

}


/**
  * @功能:I2C引脚I/O初始化
  * @
  * @返回值:
  * @
  */
void i2c_io_init(void)
{
    Scu_SetIOReuse(I2C0_SCL_PAD,SECOND_FUNCTION);
    Scu_SetIOReuse(I2C0_SDA_PAD,SECOND_FUNCTION);
}


/**
 * @brief 初始化系统
 * 
 */
void init_platform(void)
{
    init_clk_div();
    init_irq_pri();
    init_pin_mux();
}


/**
 * @brief 获取ipcore时钟
 * 
 * @return uint32_t ipcore时钟
 */
uint32_t get_ipcore_clk(void)
{
    return ipcore_clk;
}


/**
 * @brief 获取AHB时钟
 * 
 * @return uint32_t AHB时钟
 */
uint32_t get_ahb_clk(void)
{
    return ahb_clk;
}


/**
 * @brief 获取APB时钟
 * 
 * @return uint32_t APB时钟
 */
uint32_t get_apb_clk(void)
{
    return apb_clk;
}


/**
 * @brief 获取systick时钟
 * 
 * @return uint32_t systick时钟
 */
uint32_t get_systick_clk(void)
{
    return systick_clk;
}


/**
 * @brief 获取iispll时钟
 * 
 * @return uint32_t iispll时钟
 */
uint32_t get_iispll_clk(void)
{
    return iispll_clk;
}


/**
 * @brief 获取osc时钟
 * 
 * @return uint32_t osc时钟
 */
uint32_t get_osc_clk(void)
{
    return OSC_CLK;
}


/**
 * @brief 设置IPCORE时钟
 * 
 * @param clk IPCORE时钟
 */
void set_ipcore_clk(uint32_t clk)
{
    ipcore_clk = clk;
}


/**
 * @brief 设置AHB时钟
 * 
 * @param clk AHB时钟
 */
void set_ahb_clk(uint32_t clk)
{
    ahb_clk = clk;
}


/**
 * @brief 设置APB时钟
 * 
 * @param clk APB时钟
 */
void set_apb_clk(uint32_t clk)
{
    apb_clk = clk;
}


/**
 * @brief 设置systick时钟
 * 
 * @param clk systick时钟
 */
void set_systick_clk(uint32_t clk)
{
    systick_clk = clk;
}


/**
 * @brief 设置iispll时钟
 * 
 * @param clk iispll时钟
 */
void set_iispll_clk(uint32_t clk)
{
    iispll_clk = clk;
}


void platform_io_init(void)
{
}


void pa_switch_io_init(void)
{
    Scu_SetDeviceGate((uint32_t)PA_CONTROL_GPIO_BASE,ENABLE);
    
    Scu_SetIOPull(PA_CONTROL_PINPAD_NAME,DISABLE);
	Scu_SetIOReuse(PA_CONTROL_PINPAD_NAME,PA_CONTROL_FUN_CHOICE);
    gpio_set_input_mode(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
    uint8_t pa_default_level = gpio_get_input_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
    gpio_set_output_mode(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);

    if(pa_default_level == 0)
    {//PA脚如果是下拉的，PA有效电平就是高电平
        sg_pa_control_level_flag = true;
    }
    else
    {
        sg_pa_control_level_flag = false;
    }
    
    #if (!PLAYER_CONTROL_PA)
        if(sg_pa_control_level_flag == true)
        {
            gpio_set_output_high_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
        else
        {
            gpio_set_output_low_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
    #else
        if(sg_pa_control_level_flag == true)
        {
            gpio_set_output_low_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
        else
        {
            gpio_set_output_high_level(PA_CONTROL_GPIO_BASE,PA_CONTROL_GPIO_PIN);
        }
    #endif
}

bool get_pa_control_level_flag(void)
{
    return sg_pa_control_level_flag;
}
