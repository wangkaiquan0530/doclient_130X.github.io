/**
 * @file one_wire.h
 * @brief DHT11传感器的头文件
 * @version 0.1
 * @date 2019-07-02
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */

#ifndef __ONE_WIRE_H__
#define __ONE_WIRE_H__

/**
 * @addtogroup one_wire
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
                            include
-----------------------------------------------------------------------------*/
#include "stdint.h"
#include "string.h"
#include "ci112x_timer.h"
#include "ci112x_gpio.h"
#include "ci112x_scu.h"
#include "ci_log.h"

/*-----------------------------------------------------------------------------
                            define
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                            extern
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                        struct / enum / union
-----------------------------------------------------------------------------*/
/* 单总线配置结构体 */
typedef struct
{
    gpio_base_t one_wire_gpio_group;/*!< GPIO组 */
    gpio_pin_t one_wire_gpio_pin;/*!< GPIO_PINX */
    PinPad_Name one_wire_gpio_pad;/*!< PAD名称 */
    IOResue_FUNCTION one_wire_gpio_func;/*!< 第几功能是GPIO */
}one_wire_config;

/*-----------------------------------------------------------------------------
                            global
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                        function declare
-----------------------------------------------------------------------------*/
int8_t one_wire_init(one_wire_config* config);
void one_wire_reset(void);
void one_wire_write(uint8_t *data,uint32_t len);
void one_wire_read(uint8_t *data,uint32_t len);
void one_wire_timer_handler(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif

/*-----------------------------------------------------------------------------
                            end of the file
-----------------------------------------------------------------------------*/