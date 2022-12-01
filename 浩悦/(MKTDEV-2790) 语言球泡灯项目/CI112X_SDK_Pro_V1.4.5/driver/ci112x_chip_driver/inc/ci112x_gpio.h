/**
 * @file ci112x_gpio.h
 * @brief  GPIO驱动文件
 * @version 0.1
 * @date 2019-05-07
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */

#ifndef _CX111_GPIO_H
#define _CX111_GPIO_H

#include "ci112x_system.h"
#include "ci112x_scu.h"

/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_gpio ci112x_gpio
 * @brief CI112X芯片GPIO驱动
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO控制器定义
 */
typedef enum
{
    GPIO0 = HAL_GPIO0_BASE,/*!< GPIO0控制器 */
    GPIO1 = HAL_GPIO1_BASE,/*!< GPIO1控制器 */
    GPIO2 = HAL_GPIO2_BASE,/*!< GPIO2控制器 */
    GPIO3 = HAL_GPIO3_BASE,/*!< GPIO3控制器 */
    GPIO4 = HAL_GPIO4_BASE,/*!< GPIO4控制器 */
}gpio_base_t;

/**
 * @brief GPIO pin脚定义
 */
typedef enum
{
    gpio_pin_0 = (0x1 << 0),/*!< Pin0脚 */
    gpio_pin_1 = (0x1 << 1),/*!< Pin1脚 */
    gpio_pin_2 = (0x1 << 2),/*!< Pin2脚 */
    gpio_pin_3 = (0x1 << 3),/*!< Pin3脚 */
    gpio_pin_4 = (0x1 << 4),/*!< Pin4脚 */
    gpio_pin_5 = (0x1 << 5),/*!< Pin5脚 */
    gpio_pin_6 = (0x1 << 6),/*!< Pin6脚 */
    gpio_pin_7 = (0x1 << 7),/*!< Pin7脚 */
    gpio_pin_all = 0xFF,/*!< 所有Pin脚 */
}gpio_pin_t;

/**
 * @brief GPIO中断触发模式定义
 */
typedef enum
{
    high_level_trigger       = 1,/*!< 高电平触发中断 */
    low_level_trigger        = 2,/*!< 低电平触发中断 */
    up_edges_trigger         = 3,/*!< 上升沿触发中断 */
    down_edges_trigger       = 4,/*!< 下降沿触发中断 */
    both_edges_trigger       = 5,/*!< 双边沿触发中断 */
}gpio_trigger_t;

/**
 * @brief GPIO信息
 */
typedef struct
{
    gpio_base_t base;/*!< GPIO基地址 */
    gpio_pin_t pin;/*!< GPIO pin */
}gpio_info_t;

typedef void(*gpio_irq_callback_t)(void);

typedef struct gpio_irq_callback_list_s
{
    gpio_irq_callback_t gpio_irq_callback;
    struct gpio_irq_callback_list_s * next;
}gpio_irq_callback_list_t;

/*-------------------以下API可同时操作一个或多个pin脚-------------------------*/
void gpio_set_output_mode(gpio_base_t gpio,gpio_pin_t pins);
void gpio_set_input_mode(gpio_base_t gpio,gpio_pin_t pins);
uint8_t gpio_get_direction_status(gpio_base_t gpio,gpio_pin_t pins);
void gpio_irq_mask(gpio_base_t gpio,gpio_pin_t pins);
void gpio_irq_unmask(gpio_base_t gpio,gpio_pin_t pins);
void gpio_irq_trigger_config(gpio_base_t gpio,gpio_pin_t pins,gpio_trigger_t trigger);
void gpio_set_output_high_level(gpio_base_t gpio,gpio_pin_t pins);
void gpio_set_output_low_level(gpio_base_t gpio,gpio_pin_t pins);
uint8_t gpio_get_input_level(gpio_base_t gpio,gpio_pin_t pins);

/*----------------------以下API一次只操作一个pin脚----------------------------*/
uint8_t gpio_get_direction_status_single(gpio_base_t gpio,gpio_pin_t pins);
uint8_t gpio_get_irq_raw_status_single(gpio_base_t gpio,gpio_pin_t pins);
uint8_t gpio_get_irq_mask_status_single(gpio_base_t gpio,gpio_pin_t pins);
void gpio_clear_irq_single(gpio_base_t gpio,gpio_pin_t pins);
void gpio_set_output_level_single(gpio_base_t gpio,gpio_pin_t pins,uint8_t level);
uint8_t gpio_get_input_level_single(gpio_base_t gpio,gpio_pin_t pins);

/*------------------------以下API为中断处理函数-------------------------------*/
void registe_gpio_callback(gpio_base_t base, gpio_irq_callback_list_t *gpio_irq_callback_node);
void GPIO0_IRQHandler(void);
void GPIO1_IRQHandler(void);
void GPIO2_IRQHandler(void);
void GPIO3_IRQHandler(void);
void GPIO4_IRQHandler(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
