/**
 * @file one_wire.c
 * @brief
 * @version 0.1
 * @date 2019-06-28
 *
 * ！！！注意单总线GPIO脚必须有上拉，读写数据均为低位在前高位在后。
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */
/*-----------------------------------------------------------------------------
                            include
-----------------------------------------------------------------------------*/
#include "ci112x_core_eclic.h"
#include "one_wire.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*-----------------------------------------------------------------------------
                            define
-----------------------------------------------------------------------------*/
/* 用到的硬件TIMER定义 */
#define ONE_WIRE_TIMER (TIMER0)
#define ONE_WIRE_TIMER_IRQ (TIMER0_IRQn)
#define ONE_WIRE_TIMER_US (get_apb_clk()/1000000)
/* 发送复位信号延时定义 */
#define ONE_WIRE_SEND_RESET_TIMER_US (480)/* 发送复位时间 */
#define ONE_WIRE_READ_RESPOND_TIMER_US (70)/* 读取响应时间 */
#define ONE_WIRE_RESPOND_CONTINUE_TIMER_US (480)/* 响应持续时间 */
/* 写数据信号延时定义 */
#define ONE_WIRE_WRITE_PREPARE_TIMER_US (15)/* 写数据准备时间 */
#define ONE_WIRE_WRITE_TIMER_US (30)/* 写数据时间 */
#define ONE_WIRE_WRITE_CONTINUE_TIMER_US (65)/* 写数据持续时间 */
/* 读数据信号延时定义 */
#define ONE_WIRE_READ_PREPARE_TIMER_US (3)/* 读数据准备时间 */
#define ONE_WIRE_READ_TIMER_US (13)/* 读数据时间 */
#define ONE_WIRE_READ_CONTINUE_TIMER_US (63)/* 读数据持续时间 */

/*-----------------------------------------------------------------------------
                            extern
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                        struct / enum / union
-----------------------------------------------------------------------------*/
/* 整体状态类型 */
typedef enum
{
    ONE_WIRE_RESET_STATE = 0,
    ONE_WIRE_WRITE_STATE = 1,
    ONE_WIRE_READ_STATE = 2,
}one_wire_state_t;

/* 复位状态类型 */
typedef enum
{
    ONE_WIRE_RESET_PREPARE_STATE = 0,
    ONE_WIRE_RESET_TIME_STATE = 1,
    ONE_WIRE_RESET_CONTINUE_STATE = 2,
    ONE_WIRE_RESET_END_STATE = 3,
}one_wire_reset_state_t;

/* 写状态类型 */
typedef enum
{
    ONE_WIRE_WRITE_PREPARE_STATE = 0,
    ONE_WIRE_WRITE_TIME_STATE = 1,
    ONE_WIRE_WRITE_CONTINUE_STATE = 2,
    ONE_WIRE_WRITE_END_STATE = 3,
}one_wire_write_state_t;

/* 读状态类型 */
typedef enum
{
    ONE_WIRE_READ_PREPARE_STATE = 0,
    ONE_WIRE_READ_TIME_STATE = 1,
    ONE_WIRE_READ_CONTINUE_STATE = 2,
    ONE_WIRE_READ_END_STATE = 3,
}one_wire_read_state_t;

/*-----------------------------------------------------------------------------
                            global
-----------------------------------------------------------------------------*/
static one_wire_config one_wire = { GPIO2,
                                    gpio_pin_1,
                                    UART1_TX_PAD,
                                    FIRST_FUNCTION };/*!< 默认GPIO配置 */

static one_wire_state_t one_wire_state;/*!< 整体状态 */
static one_wire_reset_state_t one_wire_reset_state;/*!< 复位状态 */
static one_wire_write_state_t one_wire_write_state;/*!< 写数据状态 */
static one_wire_read_state_t one_wire_read_state;/*!< 读数据状态 */

SemaphoreHandle_t one_wire_lock;/*!< 互斥信号量 */

static uint8_t one_wire_data;/*!< 读写数据临时变量 */

/*-----------------------------------------------------------------------------
                            declare
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                            function
-----------------------------------------------------------------------------*/
/**
 * @brief 微秒级延时
 *
 * @param us 微秒数
 */
static void one_wire_delay_us(uint32_t us)
{
    /* 微秒级延时 */
    timer_set_count(ONE_WIRE_TIMER,us * ONE_WIRE_TIMER_US);
    timer_start(ONE_WIRE_TIMER);
}

/**
 * @brief 单总线初始化
 *
 * @param config 配置信息
 *
 * @retval RETURN_ERR
 * @retval RETURN_OK
 */
int8_t one_wire_init(one_wire_config* config)
{
    if(NULL != config)
    {
        /* 保存配置信息 */
        one_wire.one_wire_gpio_group = config->one_wire_gpio_group;
        one_wire.one_wire_gpio_pin = config->one_wire_gpio_pin;
        one_wire.one_wire_gpio_pad = config->one_wire_gpio_pad;
        one_wire.one_wire_gpio_func = config->one_wire_gpio_func;
    }
    /* 初始化GPIO */
    Scu_SetDeviceGate((unsigned int)one_wire.one_wire_gpio_group,ENABLE);
    Scu_Setdevice_Reset((unsigned int)one_wire.one_wire_gpio_group);
    Scu_Setdevice_ResetRelease((unsigned int)one_wire.one_wire_gpio_group);
    Scu_SetIOReuse(one_wire.one_wire_gpio_pad,one_wire.one_wire_gpio_func);
    /* 初始化TIMER */
    Scu_SetDeviceGate(ONE_WIRE_TIMER,ENABLE);
    Scu_Setdevice_Reset(ONE_WIRE_TIMER);
    Scu_Setdevice_ResetRelease(ONE_WIRE_TIMER);
    __eclic_irq_set_vector(ONE_WIRE_TIMER_IRQ,(uint32_t)one_wire_timer_handler);
    eclic_irq_enable(ONE_WIRE_TIMER_IRQ);
    timer_init_t init;
    init.mode = timer_count_mode_single;
    init.div = timer_clk_div_0;
    init.width = timer_iqr_width_2;
    init.count = 0xFFFFFFFF;
    timer_init(ONE_WIRE_TIMER,init);
    one_wire_lock = xSemaphoreCreateCounting(1,0);
    if(NULL == one_wire_lock)
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 发送复位信号
 *
 */
void one_wire_reset(void)
{
    one_wire_state = ONE_WIRE_RESET_STATE;
    one_wire_reset_state = ONE_WIRE_RESET_PREPARE_STATE;
    one_wire_delay_us(1);
    xSemaphoreTake(one_wire_lock,portMAX_DELAY);
}

/**
 * @brief 写一个byte
 *
 * @param data 数据
 */
static void one_wire_write_byte(uint8_t data)
{
    one_wire_state = ONE_WIRE_WRITE_STATE;
    one_wire_write_state = ONE_WIRE_WRITE_PREPARE_STATE;
    one_wire_data = data;
    one_wire_delay_us(1);
    xSemaphoreTake(one_wire_lock,portMAX_DELAY);
}

/**
 * @brief 读一个byte
 *
 * @return uint8_t data
 */
static uint8_t one_wire_read_byte(void)
{
    one_wire_state = ONE_WIRE_READ_STATE;
    one_wire_read_state = ONE_WIRE_READ_PREPARE_STATE;
    one_wire_delay_us(1);
    xSemaphoreTake(one_wire_lock,portMAX_DELAY);
    return one_wire_data;
}

/**
 * @brief 写数据
 *
 * @param data 数据
 * @param len 长度
 */
void one_wire_write(uint8_t *data,uint32_t len)
{
    for (int i = 0; i < len; i++)
    {
        one_wire_write_byte(data[i]);
    }
}

/**
 * @brief 读数据
 *
 * @param data 数据
 * @param len 长度
 */
void one_wire_read(uint8_t *data,uint32_t len)
{
    memset(data,0,len);
    for (int i = 0; i < len; i++)
    {
        data[i] = one_wire_read_byte();
    }
}

/**
 * @brief TIMER中断处理函数
 *
 */
void one_wire_timer_handler(void)
{
    static uint8_t count = 0;
    switch (one_wire_state)
    {
    /* 发送复位信号流程 */
    case ONE_WIRE_RESET_STATE:
        switch (one_wire_reset_state)
        {
        case ONE_WIRE_RESET_PREPARE_STATE:
            gpio_set_output_mode(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            gpio_set_output_low_level(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            one_wire_delay_us(ONE_WIRE_SEND_RESET_TIMER_US);
            one_wire_reset_state = ONE_WIRE_RESET_TIME_STATE;
            break;
        case ONE_WIRE_RESET_TIME_STATE:
            gpio_set_input_mode(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            one_wire_delay_us(ONE_WIRE_READ_RESPOND_TIMER_US);
            one_wire_reset_state = ONE_WIRE_RESET_CONTINUE_STATE;
            break;
        case ONE_WIRE_RESET_CONTINUE_STATE:
            if(gpio_get_input_level_single(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin))
            {
                mprintf("从机未响应 %s %d\n",__FILE__,__LINE__);
                return;
            }
            one_wire_delay_us(ONE_WIRE_RESPOND_CONTINUE_TIMER_US - ONE_WIRE_READ_RESPOND_TIMER_US);
            one_wire_reset_state = ONE_WIRE_RESET_END_STATE;
            break;
        case ONE_WIRE_RESET_END_STATE:
            xSemaphoreGiveFromISR(one_wire_lock,NULL);
            break;
        default:
            break;
        }
        break;
    /* 发送写数据信号流程 */
    case ONE_WIRE_WRITE_STATE:
        switch (one_wire_write_state)
        {
        case ONE_WIRE_WRITE_PREPARE_STATE:
            gpio_set_output_mode(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            gpio_set_output_low_level(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            one_wire_delay_us(ONE_WIRE_WRITE_PREPARE_TIMER_US);
            one_wire_write_state = ONE_WIRE_WRITE_TIME_STATE;
            break;
        case ONE_WIRE_WRITE_TIME_STATE:
            if(one_wire_data)
            {
                gpio_set_input_mode(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            }
            one_wire_delay_us(ONE_WIRE_WRITE_TIMER_US - ONE_WIRE_WRITE_PREPARE_TIMER_US);
            one_wire_write_state = ONE_WIRE_WRITE_CONTINUE_STATE;
            break;
        case ONE_WIRE_WRITE_CONTINUE_STATE:
            one_wire_delay_us(ONE_WIRE_WRITE_CONTINUE_TIMER_US - ONE_WIRE_WRITE_TIMER_US - ONE_WIRE_WRITE_PREPARE_TIMER_US);
            count++;
            if(8 != count)
            {
                one_wire_write_state = ONE_WIRE_WRITE_PREPARE_STATE;
            }
            else
            {
                one_wire_write_state = ONE_WIRE_WRITE_END_STATE;
                count = 0;
            }
            break;
        case ONE_WIRE_WRITE_END_STATE:
            xSemaphoreGiveFromISR(one_wire_lock,NULL);
            break;
        default:
            break;
        }
        break;
    /* 发送读数据信号流程 */
    case ONE_WIRE_READ_STATE:
        switch (one_wire_read_state)
        {
        case ONE_WIRE_READ_PREPARE_STATE:
            one_wire_data = 0;
            gpio_set_output_mode(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            gpio_set_output_low_level(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin);
            one_wire_delay_us(ONE_WIRE_READ_PREPARE_TIMER_US);
            one_wire_read_state = ONE_WIRE_READ_TIME_STATE;
            break;
        case ONE_WIRE_READ_TIME_STATE:
            one_wire_delay_us(ONE_WIRE_READ_TIMER_US - ONE_WIRE_READ_PREPARE_TIMER_US);
            one_wire_read_state = ONE_WIRE_READ_CONTINUE_STATE;
            break;
        case ONE_WIRE_READ_CONTINUE_STATE:
            one_wire_data |= gpio_get_input_level_single(one_wire.one_wire_gpio_group,one_wire.one_wire_gpio_pin) << count;
            one_wire_delay_us(ONE_WIRE_READ_CONTINUE_TIMER_US - ONE_WIRE_READ_TIMER_US - ONE_WIRE_READ_PREPARE_TIMER_US);
            count++;
            if(8 != count)
            {
                one_wire_read_state = ONE_WIRE_READ_PREPARE_STATE;
            }
            else
            {
                one_wire_read_state = ONE_WIRE_READ_END_STATE;
                count = 0;
            }
            break;
        case ONE_WIRE_READ_END_STATE:
            xSemaphoreGiveFromISR(one_wire_lock,NULL);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
