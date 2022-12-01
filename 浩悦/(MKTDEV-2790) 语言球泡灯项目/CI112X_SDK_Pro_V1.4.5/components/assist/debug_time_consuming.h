/**
 * @file debug_time_consuming.h
 * @brief 
 * @version 1.0
 * @date 2019-11-22
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef _DEBUG_TIME_CONSUMING_H_
#define _DEBUG_TIME_CONSUMING_H_

#include "ci_debug_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


#if CONFIG_DEBUG_EN
#define DEBUG_TIMER_INIT()     init_timer0()
#define DEBUG_TIMER_START()    timer0_start_count()
#define DEBUG_TIMER_END(x)     timer0_end_count(x)
#else
#define INIT_DEBUG_TIMER()     do{}while(0)
#define DEBUG_TIMER_START()    do{}while(0)
#define DEBUG_TIMER_END(x)     do{}while(0)
#endif

/**
 * @ingroup assist
 * @defgroup time_consuming 代码性能调试timer
 * @brief 用于调试记录代码运行效率的timer
 * @{
 */
extern void init_timer0(void);
extern void timer0_start_count(void);
extern void timer0_end_count(uint8_t* flag);
void timer0_end_count_only_print_time_us(void);


/**
 * @brief Get the value of TIMER0
 * @note 1. The type of return value is int, so we can't set the max count of TIMER0 > 0x7FFF_FFFF.
 *       2. Non-reenterable
 *
 * @return The current value of TIMER0
 */
int ci_timer_val( void ); // LT add


/** @} */

#ifdef __cplusplus
}
#endif


#endif

