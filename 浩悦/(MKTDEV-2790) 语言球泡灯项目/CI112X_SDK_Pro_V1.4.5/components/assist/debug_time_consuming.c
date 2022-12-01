/**
 * @file debug_time_consuming.c
 * @brief 
 * @version 1.0
 * @date 2019-11-22
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#include  <stdint.h>
#include "debug_time_consuming.h"
#include "ci112x_system.h"
#include "ci112x_timer.h"
#include "ci112x_scu.h"
#include "ci_log.h"
#include "platform_config.h"

#define MAX_COUNT (0xFFFFFFFF)
#define TIMER0_ONEUS_COUNT (get_apb_clk()/1000000)
#define TIMER1_ONEUS_COUNT (get_apb_clk()/1000000)
#define TIMER2_ONEUS_COUNT (get_apb_clk()/1000000)

/**
 * @brief 初始化调试timer
 *
 */
void init_timer0(void)
{
    Scu_SetDeviceGate(HAL_TIMER0_BASE,ENABLE);
    timer_init_t init;
    init.mode = timer_count_mode_single;
    init.div = timer_clk_div_0;
    init.width = timer_iqr_width_2;
    init.count = MAX_COUNT;
    timer_init(TIMER0,init);
    timer_start(TIMER0);
    timer_stop(TIMER0);
}

/**
 * @brief 调试timer开始计时
 *
 */
void timer0_start_count(void)
{
    timer_stop(TIMER0);
    timer_set_count(TIMER0,MAX_COUNT);
    timer_start(TIMER0);
}

/**
 * @brief 停止timer计时
 *
 * @param flag log输出标志
 */
void timer0_end_count(uint8_t* flag)
{
    uint32_t count;
    static uint32_t index = 0;
    static uint32_t all_count = 0;
    index++;
    timer_get_count(TIMER0,(unsigned int*)&count);
    count = MAX_COUNT - count;
    count /= TIMER0_ONEUS_COUNT;
    ci_logdebug(LOG_SYS_INFO,"%s once:%dus\n",flag,count);
    all_count += count;
    ci_logdebug(LOG_SYS_INFO,"%s average:%dus\n",flag,all_count/index);	
}
    
    
/**
 * @brief 停止timer计时
 *
 * @param flag log输出标志
 */
void timer0_end_count_only_print_time_us(void)
{
    uint32_t count;
    static uint32_t index = 0;
    static uint32_t all_count = 0;
    index++;
    timer_get_count(TIMER0,(unsigned int*)(&count));
    count = MAX_COUNT - count;
    count /= TIMER0_ONEUS_COUNT;
    ci_logdebug(LOG_SYS_INFO,"%d\n",count);
}


void init_timer1(void)
{
    Scu_SetDeviceGate(HAL_TIMER1_BASE,ENABLE);
    timer_init_t init;
    init.mode = timer_count_mode_single;
    init.div = timer_clk_div_0;
    init.width = timer_iqr_width_2;
    init.count = MAX_COUNT;
    timer_init(TIMER1,init);
    timer_start(TIMER1);
    timer_stop(TIMER1);
}

void timer1_start_count(void)
{
    timer_stop(TIMER1);
    timer_set_count(TIMER1,MAX_COUNT);
    timer_start(TIMER1);
}


void timer1_end_count(uint8_t* flag)
{
    uint32_t count;
    static uint32_t index = 0;
    static uint32_t all_count = 0;
    index++;
    timer_get_count(TIMER1,(unsigned int*)&count);
    count = MAX_COUNT - count;
    count /= TIMER1_ONEUS_COUNT;
    ci_logdebug(LOG_SYS_INFO,"%s once:%dus\n",flag,count);
    all_count += count;
    ci_logdebug(LOG_SYS_INFO,"%s average:%dus\n",flag,all_count/index);
}

void init_timer2(void)
{
    Scu_SetDeviceGate(HAL_TIMER2_BASE,ENABLE);
    timer_init_t timer2_init;
    timer2_init.mode = timer_count_mode_single;
    timer2_init.div = timer_clk_div_0;
    timer2_init.width = timer_iqr_width_2;
    timer2_init.count = MAX_COUNT;
    timer_init(TIMER2,timer2_init);
    timer_start(TIMER2);
    timer_stop(TIMER2);
}

void timer2_start_count(void)
{
    timer_stop(TIMER2);
    timer_set_count(TIMER2,MAX_COUNT);
    timer_start(TIMER2);
}


void timer2_end_count(uint8_t* flag)
{
    uint32_t count;
    static uint32_t index = 0;
    static uint32_t all_count = 0;
    index++;
    timer_get_count(TIMER2,(unsigned int*)&count);
    count = MAX_COUNT - count;
    count /= TIMER2_ONEUS_COUNT;
    ci_logdebug(LOG_SYS_INFO,"%s once:%dus\n",flag,count);
    all_count += count;
    ci_logdebug(LOG_SYS_INFO,"%s average:%dus\n",flag,all_count/index);
}

/****************************************************************************
 * read the *.h file please
 ****************************************************************************/
int ci_timer_val( void )
{
    static const unsigned int limit = 256; // LT, just a value, no other meanning
    unsigned int count = 0;
    timer_get_count(TIMER0,&count);

    static int reset_count_flag = 0;
    if (reset_count_flag == 0)
    {
        if (count <= limit)
        {
            reset_count_flag = 1;
            timer_stop(TIMER0);
            timer_set_count(TIMER0, MAX_COUNT );
            timer_start(TIMER0);
            reset_count_flag = 0;
        }
    }

    return (count + (TIMER0_ONEUS_COUNT>>1)) / TIMER0_ONEUS_COUNT; // we make sure that int type is enough, no overflow
}
