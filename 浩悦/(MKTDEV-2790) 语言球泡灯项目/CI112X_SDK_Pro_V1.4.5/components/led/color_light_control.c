/**
 * @file   color_light_control.c
 * @brief   A sample code use CI112X PWM control RGB or RGBW light, used HSV
 *         color space.
 * @version V1.0.0
 * @date 2018-05-23
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */

#include <stdint.h>
#include "color_light_control.h"
#include "ci112x_pwm.h"
#include "ci112x_scu.h"
#include "ci112x_gpio.h"

#include "ci112x_uart.h"
#include "ci_log.h"
#include <math.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include "ci112x_pwm.h"


/**************************************************************************
                    type define
****************************************************************************/
#define PWM_MAX_CYCLE   1000

#if 0/*fpga differt pad*/
/*color light pwm config*/
#define COLOR_LIGHT_R_PWM_PIN_NAME PWM0_PAD
#define COLOR_LIGHT_R_PWM_PIN_PORT GPIO1
#define COLOR_LIGHT_R_PWM_PIN_BASE HAL_GPIO1_BASE
#define COLOR_LIGHT_R_PWM_PIN_NUMBER gpio_pin_1
#define COLOR_LIGHT_R_PWM_FUNCTION SECOND_FUNCTION
#define COLOR_LIGHT_R_PWM_NAME PWM0

#define COLOR_LIGHT_G_PWM_PIN_NAME PWM1_PAD
#define COLOR_LIGHT_G_PWM_PIN_PORT GPIO1
#define COLOR_LIGHT_G_PWM_PIN_BASE HAL_GPIO1_BASE
#define COLOR_LIGHT_G_PWM_PIN_NUMBER gpio_pin_2
#define COLOR_LIGHT_G_PWM_FUNCTION SECOND_FUNCTION
#define COLOR_LIGHT_G_PWM_NAME PWM1

#define COLOR_LIGHT_B_PWM_PIN_NAME PWM2_PAD
#define COLOR_LIGHT_B_PWM_PIN_PORT GPIO1
#define COLOR_LIGHT_B_PWM_PIN_BASE HAL_GPIO1_BASE
#define COLOR_LIGHT_B_PWM_PIN_NUMBER gpio_pin_3
#define COLOR_LIGHT_B_PWM_FUNCTION SECOND_FUNCTION
#define COLOR_LIGHT_B_PWM_NAME PWM2
#else
/*color light pwm config*/
#define COLOR_LIGHT_R_PWM_PIN_NAME PWM5_PAD
#define COLOR_LIGHT_R_PWM_PIN_PORT GPIO1
#define COLOR_LIGHT_R_PWM_PIN_BASE HAL_GPIO1_BASE
#define COLOR_LIGHT_R_PWM_PIN_NUMBER gpio_pin_6
#define COLOR_LIGHT_R_PWM_FUNCTION SECOND_FUNCTION
#define COLOR_LIGHT_R_PWM_NAME PWM5

#define COLOR_LIGHT_G_PWM_PIN_NAME PWM4_PAD
#define COLOR_LIGHT_G_PWM_PIN_PORT GPIO1
#define COLOR_LIGHT_G_PWM_PIN_BASE HAL_GPIO1_BASE
#define COLOR_LIGHT_G_PWM_PIN_NUMBER gpio_pin_5
#define COLOR_LIGHT_G_PWM_FUNCTION SECOND_FUNCTION
#define COLOR_LIGHT_G_PWM_NAME PWM4

#define COLOR_LIGHT_B_PWM_PIN_NAME PWM3_PAD
#define COLOR_LIGHT_B_PWM_PIN_PORT GPIO1
#define COLOR_LIGHT_B_PWM_PIN_BASE HAL_GPIO1_BASE
#define COLOR_LIGHT_B_PWM_PIN_NUMBER gpio_pin_4
#define COLOR_LIGHT_B_PWM_FUNCTION SECOND_FUNCTION
#define COLOR_LIGHT_B_PWM_NAME PWM3

#endif


#define MAX3(a,b,c) (((a) > (b)) ? ( ((a) > (c)) ? (a) : (c) ) : ( ((b) > (c)) ? (b) : (c) ))
#define MIN3(a,b,c) (((a) > (b)) ? ( ((b) > (c)) ? (c) : (b) ) : ( ((a) > (c)) ? (c) : (a) ))

/**************************************************************************
                    function prototype
****************************************************************************/



/**************************************************************************
                    global
****************************************************************************/
static float current_color_h;
static float current_color_s;
static float current_color_v;


/**************************************************************************
                    color led
****************************************************************************/


/**
 * @brief initial, config IO and PWM used by light.
 *
 */
void color_light_init(void)
{
    pwm_init_t pwm_config;

    Scu_SetDeviceGate(COLOR_LIGHT_R_PWM_PIN_BASE,ENABLE);
    Scu_SetDeviceGate(COLOR_LIGHT_G_PWM_PIN_BASE,ENABLE);
    Scu_SetDeviceGate(COLOR_LIGHT_B_PWM_PIN_BASE,ENABLE);

    /*R*/
    Scu_SetIOReuse(COLOR_LIGHT_R_PWM_PIN_NAME,COLOR_LIGHT_R_PWM_FUNCTION);
    Scu_SetDeviceGate(COLOR_LIGHT_R_PWM_NAME,ENABLE);

    pwm_config.clk_sel = 0;
    pwm_config.freq = 10000;
    pwm_config.duty = 500;
    pwm_config.duty_max = PWM_MAX_CYCLE;
    pwm_init(COLOR_LIGHT_R_PWM_NAME,pwm_config);
    pwm_start(COLOR_LIGHT_R_PWM_NAME);
    Scu_SetIOReuse(COLOR_LIGHT_R_PWM_PIN_NAME,COLOR_LIGHT_R_PWM_FUNCTION);/*pwm pad enable*/

    /*G*/
    Scu_SetIOReuse(COLOR_LIGHT_G_PWM_PIN_NAME,COLOR_LIGHT_G_PWM_FUNCTION);
    Scu_SetDeviceGate(COLOR_LIGHT_G_PWM_NAME,ENABLE);
    
    pwm_config.clk_sel = 0;
    pwm_config.freq = 10000;
    pwm_config.duty = 500;
    pwm_config.duty_max = PWM_MAX_CYCLE;
    pwm_init(COLOR_LIGHT_G_PWM_NAME,pwm_config);
    pwm_start(COLOR_LIGHT_G_PWM_NAME);
    Scu_SetIOReuse(COLOR_LIGHT_G_PWM_PIN_NAME,COLOR_LIGHT_G_PWM_FUNCTION);/*pwm pad enable*/

    /*B*/
    Scu_SetIOReuse(COLOR_LIGHT_B_PWM_PIN_NAME,COLOR_LIGHT_B_PWM_FUNCTION);
    Scu_SetDeviceGate(COLOR_LIGHT_B_PWM_NAME,ENABLE);
    
    pwm_config.clk_sel = 0;
    pwm_config.freq = 10000;
    pwm_config.duty = 500;
    pwm_config.duty_max = PWM_MAX_CYCLE;
    pwm_init(COLOR_LIGHT_B_PWM_NAME,pwm_config);
    pwm_start(COLOR_LIGHT_B_PWM_NAME);
    Scu_SetIOReuse(COLOR_LIGHT_B_PWM_PIN_NAME,COLOR_LIGHT_B_PWM_FUNCTION);/*pwm pad enable*/
}


void HSV_to_RGB(float * r,float * g,float * b,float h,float s,float v)
{
    int i;
    float f,p,q,t;

    h /= 60; //sector 0 to 5
    i = (int)floorf(h);
    f = h - i;   //factorial part of h
    p = v *(1 - s);
    q = v *(1 - s*f);
    t = v*(1 - s*(1-f));

    switch(i)
    {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        default://case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
   }
}


#if 1/*smple 1*/
void gamma_correct_RGB(float *R, float *G, float *B)
{
    if ( *R > 0.003f )
    {
        *R = (1.22f * (float)( powf(*R, ( 1.0f/1.5f ) )) - 0.040f);
    }
    else
    {
        *R = 0;
    }

    if ( *G > 0.003f)
    {
        *G = (1.22f * (float)( powf(*G, ( 1.0f/1.5f ) )) - 0.040f);
    }
    else
    {
        *G = 0;
    }

    if ( *B > 0.003f )
    {
        *B = (1.09f * (float)( powf(*B, ( 1.0f/1.5f ) )) - 0.050f);
    }
    else
    {
        *B = 0;
    }
}
#else/*smple 2*/
void gamma_correct_RGB(float *R, float *G, float *B)
{
    if ( *R > 0.003f )
    {
        *R = ( ( powf(*R, 2.2f )) );
    }
    else
    {
        *R = 0;
    }

    if ( *G > 0.003f)
    {
        *G = (( powf(*G, 2.2f )));
    }
    else
    {
        *G = 0;
    }

    if ( *B > 0.003f )
    {
        *B = (( powf(*B, 2.2f )));
    }
    else
    {
        *B = 0;
    }
}
#endif


void temp_compensate_RGB(float *R, float *G, float *B)
{
}


void group_wavelen_compensate_RGB(float *R, float *G, float *B)
{
}


void group_lumin_compensate_RGB(float *R, float *G, float *B)
{
}

//#define USED_RGBW_LED_SHOW (0)

#ifdef USED_RGBW_LED_SHOW
static void RGB_to_RGBW( uint32_t *R, uint32_t *G, uint32_t *B, uint32_t *W )
{
  uint32_t w;
  if ( MAX3(*R,*G,*B) == 0 )
  {
    *W = 0;
    return;
  }
  w = (uint32_t)( ( (uint32_t)MIN3(*R,*G,*B) + MAX3(*R,*G,*B)  ) / ( MAX3(*R,*G,*B)  ) );
  *R *= w;
  *G *= w;
  *B *= w;
  *W = MIN3(*R,*G,*B);
  *W = (*W > PWM_MAX_CYCLE) ? PWM_MAX_CYCLE : *W;
  *R -= *W;
  *G -= *W;
  *B -= *W;
}
#endif

static uint32_t led_can_set0_flag = 0;
void light_pwm_set_duty (pwm_base_t channel, uint32_t promill)
{
#define MIN_PROMILL 0

    if(1==led_can_set0_flag)
    {

    }
    else
    {
        if(promill <= MIN_PROMILL)
        {
            promill = MIN_PROMILL;
        }
    }

    //promill = PWM_MAX_CYCLE - promill;/*invert*/
    pwm_set_duty(channel,promill,PWM_MAX_CYCLE);
}


static void light_update_color(void)
{
    float h,s,v;
    float f_R,f_G,f_B;
    uint32_t R,G,B;
    #ifdef USED_RGBW_LED_SHOW/*for RGBW light*/
    uint32_t W;
    #endif
    float Y,LinearY;

    h = current_color_h;
    s = current_color_s;
    v = current_color_v;

    /*par check*/
    h = h>=360.00f ? 0.0f: h;
    s = s>1.0f ? 1.0f : s;
    v= v>1.0f ? 1.0f : v;

    LinearY = v;

    #if 1/*gamma correct the level*/
    Y = powf( LinearY, (float)2.0 ) ;
    #else/*no gamma correct*/
    Y = LinearY;
    #endif

    #if 0/*correct some color*/
    if((h<270.00f)&&(h>210.00f))
    {
        h = powf((h-210.00f)/60.00f,2.5f) * 60.00f + 210.00f ;
    }
    #endif

    s = powf( s, (float)0.45 ) ;

    HSV_to_RGB(&f_R,&f_G,&f_B,h,s,Y);

    //color correction
    //gamma_correct_RGB(&f_R, &f_G, &f_B);
    temp_compensate_RGB(&f_R, &f_G, &f_B);
    group_wavelen_compensate_RGB(&f_R, &f_G, &f_B);
    group_lumin_compensate_RGB(&f_R, &f_G, &f_B);

    // truncate results exceeding 0..1
    f_R = (f_R > 1.0f ? 1.0f : (f_R < 0.0f ? 0.0f : f_R));
    f_G = (f_G > 1.0f ? 1.0f : (f_G < 0.0f ? 0.0f : f_G));
    f_B = (f_B > 1.0f ? 1.0f : (f_B < 0.0f ? 0.0f : f_B));

    R = (uint32_t)(f_R * (PWM_MAX_CYCLE-1));
    G = (uint32_t)(f_G * (PWM_MAX_CYCLE-1));
    B = (uint32_t)(f_B * (PWM_MAX_CYCLE-1));

    #ifdef USED_RGBW_LED_SHOW/*for RGBW light*/
    RGB_to_RGBW( &R, &G, &B, &W );//wwf

    /*gamma correct*/
    W = (uint32_t)(powf((float)W/PWM_MAX_CYCLE,(float)2.2) * PWM_MAX_CYCLE);
    #endif

    light_pwm_set_duty (COLOR_LIGHT_R_PWM_NAME,R);
    light_pwm_set_duty (COLOR_LIGHT_G_PWM_NAME,G);
    light_pwm_set_duty (COLOR_LIGHT_B_PWM_NAME,B);
    #ifdef USED_RGBW_LED_SHOW/*for RGBW light*/
    //pwm_set_duty (WHITE_LED, W);//wwf
    #endif
}


/**
 * @brief set color, HSV all need.
 *
 * @param h : hue 0~360
 * @param s : saturation 0.0~1.0
 * @param v : value 0.0~1.0
 */
void color_light_control(float h,float s,float v)
{
    current_color_h = h;
    current_color_s = s;
    current_color_v = v;

    light_update_color();
}


/**
 * @brief set color, only update hue and saturation
 *
 * @param h : hue 0~360
 * @param s : saturation 0.0~1.0
 */
void color_light_set_color(float h,float s)
{
    current_color_h = h;
    current_color_s = s;

    light_update_color();
}


/**
 * @brief set clolr, only update value(lightness)
 *
 * @param v : value of HSV  0.0~1.0
 */
void color_light_set_level(float v)
{
    current_color_v = v;

    light_update_color();
}


