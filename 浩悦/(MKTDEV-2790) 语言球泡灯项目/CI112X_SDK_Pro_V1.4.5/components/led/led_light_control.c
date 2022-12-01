/**
  ******************************************************************************
  * @file    led_light_control.c
  * @version V1.0.0
  * @brief   A sample code use CI112X PWM control LED for night light, use CI112X
  *          PWM control LED for toys blink light
  * @date    2018.05.23
  * @brief
  ******************************************************************************
  **/

#include <stdint.h>
#include "led_light_control.h"
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

/**************************************************************************
                    type define
****************************************************************************/
#define PWM_MAX_CYCLE   100

/*night light pwm config*/
#define NIGHT_LIGHT_W_PWM_PIN_NAME PWM4_PAD
#define NIGHT_LIGHT_W_PWM_PIN_PORT GPIO1
#define NIGHT_LIGHT_W_PWM_PIN_BASE HAL_GPIO1_BASE
#define NIGHT_LIGHT_W_PWM_PIN_NUMBER gpio_pin_5
#define NIGHT_LIGHT_W_PWM_FUNCTION SECOND_FUNCTION
#define NIGHT_LIGHT_W_PWM_NAME HAL_PWM4_BASE


#define BLINK_LIGHT_IO_PIN_BASE HAL_GPIO1_BASE
#define BLINK_LIGHT_IO_PIN_NAME PWM5_PAD
#define BLINK_LIGHT_IO_PIN_PORT GPIO1
#define BLINK_LIGHT_IO_PIN_NUMBER gpio_pin_6



#define LED_ON  0
#define LED_OFF 1

typedef struct
{
    uint32_t led_onoff;
    uint32_t time_ms;
}led_show_state_t;



/**************************************************************************
                    function prototype
****************************************************************************/






/**************************************************************************
                    global
****************************************************************************/
xTimerHandle blink_led_timer = NULL;

static uint8_t led_flick_index = 0;

const led_show_state_t blink_eye[] =
{
    [0] = {LED_ON,600},
    [1] = {LED_OFF,500},
    [2] = {LED_ON,600},
    [3] = {LED_OFF,500},
    [4] = {LED_OFF,50},
};


/**************************************************************************
                    pwm led
****************************************************************************/
static void pwm_out_pad_enable(void)
{
    Scu_SetIOReuse(NIGHT_LIGHT_W_PWM_PIN_NAME,NIGHT_LIGHT_W_PWM_FUNCTION);
}


static void pwm_out_pad_disable(void)
{
    Scu_SetIOReuse(NIGHT_LIGHT_W_PWM_PIN_NAME,FIRST_FUNCTION);/*gpio function*/
    gpio_set_output_level_single(NIGHT_LIGHT_W_PWM_PIN_PORT, NIGHT_LIGHT_W_PWM_PIN_NUMBER, 0);
}


/**
 * @brief night initial, config PWM
 * 
 */
void night_light_init(void)
{
    pwm_init_t pwm_config;

    Scu_SetDeviceGate(NIGHT_LIGHT_W_PWM_PIN_BASE,ENABLE);

    Scu_SetIOReuse(NIGHT_LIGHT_W_PWM_PIN_NAME,NIGHT_LIGHT_W_PWM_FUNCTION);
    Scu_SetDeviceGate(NIGHT_LIGHT_W_PWM_NAME,ENABLE);
    
    pwm_config.clk_sel = 0;
    pwm_config.freq = 25000;
    pwm_config.duty = 0;
    pwm_config.duty_max = 100;
    pwm_init((pwm_base_t)NIGHT_LIGHT_W_PWM_NAME,pwm_config);
    pwm_start((pwm_base_t)NIGHT_LIGHT_W_PWM_NAME);
    pwm_out_pad_enable();
}



/**
 * @brief night light set brightness,
 * 
 * @param onoff 1:on , 0:off
 * @param val   0~100
 */
void night_light_set_brightness(uint32_t onoff,uint32_t val)
{
    float Y,Yin;
    if(0 == onoff)
    {
        pwm_set_duty((pwm_base_t)NIGHT_LIGHT_W_PWM_NAME,0,PWM_MAX_CYCLE);
        return;
    }

    if(val > 99)
    {
        val = 99;
    }

    Yin = val/100.00;
    if(Yin > 1.0)
    {
        Yin = 1.0;
    }
    Y = powf( Yin, (float)2.0 ) ;
    Y = Y*PWM_MAX_CYCLE;
    mprintf("Y is %f\n",Y);
    pwm_set_duty((pwm_base_t)NIGHT_LIGHT_W_PWM_NAME,(unsigned int)Y,PWM_MAX_CYCLE);
}


/**************************************************************************
                    blink led
****************************************************************************/
void SetLedAndTime(uint8_t on_off,uint16_t ms)
{
    gpio_set_output_level_single(BLINK_LIGHT_IO_PIN_PORT, BLINK_LIGHT_IO_PIN_NUMBER,on_off);
    xTimerChangePeriod(blink_led_timer,pdMS_TO_TICKS(ms),0);
    xTimerStart(blink_led_timer,0);
}


void blink_led_timer_callback(TimerHandle_t p)
{
    SetLedAndTime(blink_eye[led_flick_index].led_onoff,blink_eye[led_flick_index].time_ms);
    led_flick_index++;

    //led_flick_index %= (sizeof(blink_eye)/sizeof(blink_eye));
    if(led_flick_index >= sizeof(blink_eye)/sizeof(led_show_state_t))
    {
        xTimerStop(blink_led_timer,0);
        led_flick_index = 0;
    }
}


/**
 * @brief blink light initial, config LED, and a RTOS soft timer.
 * 
 */
void blink_light_init(void)
{
    Scu_SetDeviceGate(BLINK_LIGHT_IO_PIN_BASE,ENABLE);
    Scu_SetIOReuse(BLINK_LIGHT_IO_PIN_NAME,FIRST_FUNCTION);
    gpio_set_output_mode(BLINK_LIGHT_IO_PIN_PORT,BLINK_LIGHT_IO_PIN_NUMBER);
    gpio_set_output_level_single(BLINK_LIGHT_IO_PIN_PORT, BLINK_LIGHT_IO_PIN_NUMBER,0);

    blink_led_timer = xTimerCreate("Blink_led_timer", pdMS_TO_TICKS(10),pdFALSE, (void *)0, blink_led_timer_callback);
    if(NULL == blink_led_timer)
    {
        mprintf("timer error!\n");
    }
}


/**
 * @brief blink light start one time, simulate blink eye one time, for toys use
 * 
 */
void blink_light_on(void)
{
    led_flick_index = 0;
    xTimerChangePeriod(blink_led_timer,pdMS_TO_TICKS(10),0);
    xTimerStart(blink_led_timer,0);
}


void vad_light_init(void)
{
#if VAD_LED_EN
    Scu_SetDeviceGate(VAD_LIGHT_IO_PIN_BASE,ENABLE);
    Scu_SetIOReuse(VAD_LIGHT_IO_PIN_NAME,FIRST_FUNCTION);/*gpio function,gpio3*/
    Scu_SetIOPull(VAD_LIGHT_IO_PIN_NAME, DISABLE);
    gpio_set_output_mode(VAD_LIGHT_IO_PIN_PORT, VAD_LIGHT_IO_PIN_NUMBER);
    gpio_set_output_level_single(VAD_LIGHT_IO_PIN_PORT, VAD_LIGHT_IO_PIN_NUMBER,0);
#endif
}


void vad_light_on(void)
{
#if VAD_LED_EN
    gpio_set_output_level_single(VAD_LIGHT_IO_PIN_PORT, VAD_LIGHT_IO_PIN_NUMBER,1);
#endif
}

void vad_light_off(void)
{ 
#if VAD_LED_EN
    gpio_set_output_level_single(VAD_LIGHT_IO_PIN_PORT, VAD_LIGHT_IO_PIN_NUMBER,0);
#endif
}

