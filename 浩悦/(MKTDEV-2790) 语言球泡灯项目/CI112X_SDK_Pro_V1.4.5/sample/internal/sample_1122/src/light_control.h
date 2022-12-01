/**
 * @brief light_control.c header file
 * @file light_control.h
 * @version 1.0
 * @author zhh
 * @date 12/24/2021 16:20:8
 * @par Copyright:
 * Copyright (c) Chipintelli Technology 2000-2021. All rights reserved.
 *
 * @history
 * 1.Date        : 12/24/2021 16:20:8
 *   Author      : zhh
 *   Modification: Created file
 */
#ifndef __LIGHT_CONTROL_H__
#define __LIGHT_CONTROL_H__
/*----------------------------------------------*
 * includes                                     *
 *----------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "ci112x_timer.h"
#include "ci_nvdata_manage.h"
#include "light_drive.h"
/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
#define NVDATA_ID_LIGHT_STATUS     0x70000003

#define LIGHT_OFF_SAVE_EN          0 //关灯状态是否保存 1：保存，   0：不保存

#define LIGHT_CHANGE_COLOR_TIME      (100) //100ms

#define LIGHT_AUTO_OFF_1_MINUTE     (60*1000*1)  //1分钟
#define LIGHT_AUTO_OFF_5_MINUTE     (60*1000*5)  //5分钟
#define LIGHT_AUTO_OFF_10_MINUTE    (60*1000*10) //10分钟

#define LIGHT_CONFIG_INVALID_TIME   (1000*30) //30S

#define LIGHT_DEFAULT_ON_FADE_TIME  (10)
#define LIGHT_DEFAULT_OFF_FADE_TIME (5)

#define LIGHT_DEFAULT_FADE_TIME    (5)   //N*BASE_FADE_TIME
#define LIGHT_DEFAULT_KEEP_TIME    (0)    //N*BASE_FADE_TIME
#define DEFAULT_OFF_KEEP_TIME      (4)    //N*BASE_FADE_TIME
#define LIGHT_KEEP_RESUME_TIME     (6000) //10min  N*BASE_FADE_TIME

#define BASE_FADE_TIME             100 //ms
#define LIGHT_CTR_TIMER_PERIOD     10  //ms


#define LIGHT_STATUS_ON            1
#define LIGHT_STATUS_OFF           0

//亮度范围
#define DEFAULT_MIN_LEVEL          150
#define DEFAULT_MID_LEVEL          500
#define DEFAULT_MAX_LEVEL          1000

//色温范围
#define DEFAULT_WARM_CCT           2700
#define DEFAULT_COOL_CCT           6500
#define DEFAULT_MID_CCT            4600

//默认色温和亮度
#define LIGHT_DEFAULT_CCT          4600  //2700 ~ 6500
#define LIGHT_DEFAULT_LEVEL        1000  //10~1000

#define LIGHT_1_GEAR_LEVEL         150
#define LIGHT_2_GEAR_LEVEL         400
#define LIGHT_3_GEAR_LEVEL         700
#define LIGHT_4_GEAR_LEVEL         1000

#define LIGHT_BRIGHTNESS_UP        1
#define LIGHT_BRIGHTNESS_DOWN      0

#define LIGHT_CCT_UP               1
#define LIGHT_CCT_DOWN             0

#define LIGHT_MIN_VALUE            (DEFAULT_MIN_LEVEL)
#define LIGHT_MAX_VALUE            (DEFAULT_MAX_LEVEL - 1)

#define LightCTR_REGU_DUTY_BIT_MSK  0x00000FFF  // 12 bit level
#define LightCTR_REGU_TIME_BIT_MSK  0x000FFFFF  // 20 bit time
#define LightCTR_REGU_DUTY_BIT      20


/*----------------------------------------------*
 * enum                                         *
 *----------------------------------------------*/
typedef enum
{
	POWER_ON_STATUS_OFF	    = 0x00, //上电关
	POWER_ON_STATUS_ON 	    = 0x01, //上电开
	POWER_ON_STATUS_MEMORY  = 0x02, //上电记忆
}power_on_status_e;

typedef enum
{
    LIGHT_MODE_NATURE  = 0x00,
    LIGHT_MODE_WHITE   = 0x01,
    LIGHT_MODE_YELLOW  = 0x02,
}light_mode_e;
/*----------------------------------------------*
 * struct                                       *
 *----------------------------------------------*/
typedef struct
{
    uint8_t wake_group;
    uint8_t powerOnState;
    uint8_t mode;
    bool onoff;
	uint16_t cct;
    uint16_t level;
}light_status_t;


typedef struct
{
    uint32_t current;
    uint32_t target;
    int32_t duty;
}light_pwm_regulate_t;


typedef struct
{
    bool regulate;
    uint16_t fadeCycle;
    uint16_t keepCycle;
    bool turnOff;
    void (*finishCB)(void);
    light_pwm_regulate_t channel[LIGHT_MAX_PWM_CHANNEL];
}light_regulate_t;

/*----------------------------------------------*
 * function declaration                         *
 *----------------------------------------------*/
void light_output_update(uint16_t toCct, uint16_t toLevel, uint16_t fadeTime, uint16_t keepTime, void(*finishCB)(void));

bool get_light_onoff(void);
uint16_t get_light_cct(void);
uint16_t get_light_level(void);
uint8_t get_light_wake_group(void);

void light_power_on_output(void);

uint8_t light_turn_on_output(void);
uint8_t light_turn_off_output(void);

uint8_t light_brightness_max_output(void);
uint8_t light_brightness_mid_output(void); 
uint8_t light_brightness_min_output(void);
uint8_t light_brightness_up_output(void);
uint8_t light_brightness_down_output(void);
uint8_t light_brightness_percentage_output(uint32_t dest_level);

uint8_t light_cct_up_output(void);
uint8_t light_cct_down_output(void);

uint8_t light_white_output(void);
uint8_t light_yellow_output(void);
uint8_t light_nature_output(void);
uint8_t light_change_output(void);
uint8_t light_mode_output(void);
uint8_t light_reset_factory(void);

void light_status_nv_save(void);
void light_control_init(void);


void light_blink_output(void);

uint8_t light_control_deal(uint32_t semantic_id);
uint8_t light_timing_deal(uint32_t semantic_id);
uint8_t light_poweron_deal(uint32_t semantic_id);
uint8_t light_name_config_deal(uint32_t semantic_id);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __LIGHT_CONTROL_H__ */
