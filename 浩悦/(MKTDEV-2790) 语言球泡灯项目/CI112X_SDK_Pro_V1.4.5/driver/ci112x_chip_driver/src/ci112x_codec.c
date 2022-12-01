/**
 * @file ci112x_codec.c
 * @brief CI112X inner codec的驱动函数
 * @version 0.1
 * @date 2019-03-29
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */

#include "ci112x_codec.h"
#include "ci112x_alc.h"
#include "ci112x_scu.h"
#include "ci112x_system.h"
#include "ci_log.h"
#include "ci112x_gpio.h"
#include <math.h>
#include <string.h>

#define USE_CI112X_OWN_ALC 0//目前没有用到CI112X自带的ALC，为了省代码，用这个宏可以不编译alc.c

/**
 * @brief 当主频为168时，延时100us
 *
 * @param cnt RV相当于280us
 */
static void _delay(unsigned int cnt)
{
    volatile unsigned int i,j;
    for(i=0;i<cnt;i++)
    {
        for(j=0;j<4200;j++)
        {
        }
    }
}


/**
 * @brief ADC电流源强制启动
 * 
 */
void inner_codec_up_ibas_adc(void)
{
	CODEC->reg29 |= 1<<0;
	_delay(10);
	CODEC->reg29 &= ~(1<<0);
	_delay(10);
}


/**
 * @brief DAC电流源强制启动
 * 
 */
void inner_codec_up_ibas_dac(void)
{
	CODEC->reg2a |= 1<<0;
	_delay(10);
	CODEC->reg2a &= ~(1<<0);
	_delay(10);
}


/**
 * @brief INNER CODEC reset
 * 
 */
void inner_codec_reset(void)
{
    CODEC->reg0 = 0;
    _delay(50);
    CODEC->reg0 = 0x3;
    _delay(50);
}

/**
 * @brief CODEC作为ADC的模式选择
 *
 * @param mode Master模式或者Slave模式；
 * @param frame_Len 数据宽度；
 * @param word_len 有效数据长度；
 * @param data_fram 数据格式
 */
void inner_codec_adc_mode_set(inner_codec_mode_t mode,inner_codec_frame_1_2len_t frame_Len,
                    inner_codec_valid_word_len_t word_len,inner_codec_i2s_data_famat_t data_fram)
{
    CODEC->reg3 &= ~(0x03<<4);
    CODEC->reg3 |= (mode<<4);

    CODEC->reg3 &= ~(0x03<<2);
    CODEC->reg3 |= (frame_Len<<2);

    CODEC->reg2 &= ~(0x03<<5);
    CODEC->reg2 |= (word_len<<5);

    CODEC->reg2 &= ~(0x03<<3);
    CODEC->reg2 |= (data_fram<<3);
}


/**
 * @brief CODEC作为DAC的模式选择
 *
 * @param mode Master模式或者Slave模式；
 * @param frame_Len 数据宽度；
 * @param word_len 有效数据长度；
 * @param data_fram 数据格式
 */
void inner_codec_dac_mode_set(inner_codec_mode_t mode,inner_codec_frame_1_2len_t frame_Len,
                    inner_codec_valid_word_len_t word_len,inner_codec_i2s_data_famat_t data_fram)
{
    CODEC->reg3 &= ~(0x03<<6);
    CODEC->reg3 |= (mode<<6);

    CODEC->reg5 &= ~(0x03<<2);
    CODEC->reg5 |= (frame_Len<<2);

    CODEC->reg4 &= ~(0x03<<5);
    CODEC->reg4 |= (word_len<<5);

    CODEC->reg4 &= ~(0x03<<3);
    CODEC->reg4 |= (data_fram<<3);
}


/**
 * @brief CODEC模块上电
 *
 * @param current pre-charge/dis-charge的电流大小
 */
void inner_codec_power_up(inner_codec_current_t current)
{
    CODEC->reg2a &= ~(0x03<<4);
    CODEC->reg2a |= (0x01<<4);

    CODEC->reg22 = 0x01;
    CODEC->reg28 = 0x01;
    _delay(50);

    CODEC->reg23 |= (0x01<<5);//
    CODEC->reg2b |= (0x01<<3);//DAC ref

    CODEC->reg22 = 0xff;
    CODEC->reg28 = 0xff;
    _delay(100);

    CODEC->reg22 = current;
    CODEC->reg28 = current;
}


/**
 * @brief 关闭CODEC模块电源
 *
 */
void inner_codec_power_off(void)
{
    CODEC->reg22 = 0x01;
    CODEC->reg28 = 0x01;
    _delay(5);
    CODEC->reg23 &= ~(0x01<<5);
    CODEC->reg2b &= ~(0x01<<3);//DAC ref

    CODEC->reg22 = 0xff;
    CODEC->reg28 = 0xff;
    _delay(100);
}


/**
 * @brief 配置CODEC模块的高通滤波器
 *
 * @param gate 高通滤波器开关
 * @param Hz 高通滤波器的截止频率
 */
void inner_codec_hightpass_config(inner_cedoc_gate_t gate,inner_codec_highpass_cut_off_t Hz)
{
    CODEC->rega  |= (0x01<<6);

    CODEC->rega &= ~(0x01<<2);
    CODEC->rega |= ((!gate)<<2);
    CODEC->rega &= ~(0x03<<0);
    if(INNER_CODEC_GATE_ENABLE == gate)
    {
        CODEC->rega |= Hz<<0;
    }
    _delay(50);
}


/**
 * @brief 设置左通道的MIC增益
 * 
 * @param gain 
 */
void inner_codec_set_mic_gain_left(inner_codec_mic_amplify_t gain)
{
    CODEC->reg25 &= ~(3 << 6);
    CODEC->reg25 |= (gain << 6);
    _delay(50);
}


/**
 * @brief 使能CODEC中的ADC:
 *        1.使用此函数之前先初始化CODEC_ADC_Config_TypeDef结构体并填写参数；
 *        2.ALCL_Gain和ALCR_Gain的上下限分别是28.5dB和-18dB，步长1.5。
 *
 * @param ADC_Config CODEC_ADC_Config_TypeDef结构体指针
 */
void inner_codec_adc_enable(inner_codec_adc_config_t *ADC_Config)
{
    float Reg_ALC_Gain = 0;
    // inner_codec_up_ibas_adc();
    CODEC->reg24 |= (0x01<<7);
    CODEC->reg24 |= (0x01<<5);
    CODEC->reg23 |= (0x3f<<0);
    _delay(10);
    inner_codec_up_ibas_adc();
    CODEC->reg26 |= (0x01<<4);
    _delay(50);

    CODEC->reg26 |= (0x01<<5);
    CODEC->reg29 |= (0x0f<<4);
    CODEC->reg24 |= (0x01<<6);

    CODEC->reg25 &= ~(0x03<<6);
    CODEC->reg25 |= (ADC_Config->codec_adcl_mic_amp<<6);
    _delay(50); 

    CODEC->reg24 |= (0x01<<4);
    CODEC->reg25 &= ~(0x0f<<0);
    CODEC->reg25 |= (5<<0);//ADC电流配置

    if(28.5 < ADC_Config->pga_gain_l)
    {
        ADC_Config->pga_gain_l = 28.5;
    }
    else if(-18 > ADC_Config->pga_gain_l)
    {
        ADC_Config->pga_gain_l = -18;
    }
    Reg_ALC_Gain = (ADC_Config->pga_gain_l+18)/1.5;
    CODEC->reg27  = (int)Reg_ALC_Gain;
}


/**
 * @brief 调节CODEC ADC的数字增益
 *
 * @param l_dig_gain 左声道增益大小
 * @param r_dig_gain 右声道增益大小
 */
void inner_codec_adc_dig_gain_set(unsigned int l_dig_gain,unsigned int r_dig_gain)
{
    CODEC->reg8 = l_dig_gain;
    // CODEC->reg9 = r_dig_gain;
}


/**
 * @brief 调节CODEC ADC的数字增益，左通道
 *
 * @param l_dig_gain 左声道增益大小
 */
void inner_codec_adc_dig_gain_set_left(uint8_t gain)
{
    CODEC->reg8 = gain;
}




/**
 * @brief 关闭CODEC中的ADC
 *
 * @param cha 左右通道、全部通道的选择；
 * @param EN 选择是否关闭ADC
 */
void inner_codec_adc_disable(inner_codec_cha_sel_t cha, inner_cedoc_gate_t EN)
{
   if(INNER_CODEC_GATE_ENABLE == EN)    //dis左通道的ADC
    {
        CODEC->reg24 &= ~(0x07<<4);
        CODEC->reg26 &= ~(0x03<<4);
        CODEC->reg29 &= ~(0x0f<<4);
    }
}


/**
 * @brief 配置CODEC中的DAC增益,-39dB到7.5dB，步长1.5dB，真实为32档。
 *
 * @param l_gain LeftADC中 HPDRV的增益，0--100；
 * @param r_gain RightADC中 HPDRV的增益，0--100。
 */
void inner_codec_dac_gain_set(int32_t l_gain,int32_t r_gain)
{
    float step = 100.0f/31.0f;
    uint32_t reg_l_gain = 0;
    if(l_gain >= 100)
    {
        l_gain = 100;
    }

    if(l_gain < 0)
    {
        l_gain = 0;
    }

    reg_l_gain = (uint32_t)( (float)l_gain/step );

    CODEC->reg2f  = reg_l_gain;

    _delay(300);
}


void inner_codec_dac_first_enable(void)
{
    CODEC->reg2a |= (0x01<<7);
    _delay(10);
    inner_codec_up_ibas_dac();
    //_delay(50);
    CODEC->reg2a |= (0x01<<6);

    //_delay(50);
    CODEC->reg2e |= (0x01<<4);//1

    //_delay(50);
    CODEC->reg2e |= (0x01<<5);//2

    //_delay(50);
    CODEC->reg2b |= (0x1<<7);//3
    //_delay(50);

    CODEC->reg2b |= (0x1<<6);

    //_delay(50);
    CODEC->reg2b |= (0x1<<5);

    //_delay(50);
    CODEC->reg2b |= (0x1<<4);

    //_delay(50);
    CODEC->reg2e |= (0x01<<6);

    CODEC->reg2c = 0x1f;//0x1f HPOUT的驱动能力最小
}


/**
 * @brief 使能DAC
 * 
 */
void inner_codec_dac_enable(void)
{
//	CODEC->reg2b |= (0x01<<3);//4
//	_delay(2000);

    CODEC->reg2a |= (0x01<<7);

    //_delay(50);
    CODEC->reg2a |= (0x01<<6);

    //_delay(50);
    CODEC->reg2e |= (0x01<<4);//1

    //_delay(50);
    CODEC->reg2e |= (0x01<<5);//2

    //_delay(50);
    CODEC->reg2b |= (0x1<<7);//3
    //_delay(50);

    CODEC->reg2b |= (0x1<<6);

    //_delay(50);
    CODEC->reg2b |= (0x1<<5);

    //_delay(50);
    CODEC->reg2b |= (0x1<<4);

    //_delay(50);
    CODEC->reg2e |= (0x01<<6);

    CODEC->reg2c = 0x1f;//0x1f HPOUT的驱动能力最小
}


void inner_codec_hpout_mute(void)
{
    uint32_t reg_data = CODEC->reg2e;
    reg_data &= ~(1<<6);
    CODEC->reg2e = reg_data;
}


void inner_codec_hpout_mute_disable(void)
{
    uint32_t reg_data = CODEC->reg2e;
    reg_data |= (1<<6);
    CODEC->reg2e = reg_data;
}


/**
 * @brief 关闭DAC
 *
 * @param cha 声道选择
 * @param EN 开关
 */
void inner_codec_dac_disable(inner_codec_cha_sel_t cha, inner_cedoc_gate_t EN)
{
   if(INNER_CODEC_GATE_ENABLE == EN)    //dis左通道的ADC
    {
        CODEC->reg2e &= ~(0x07<<4);
        CODEC->reg2a &= ~(0x03<<4);
        CODEC->reg2a |= (0x01<<4);
        CODEC->reg2a &= ~(0x01<<6);
        CODEC->reg2b &= ~(0xf<<4);
    }
}


/**
 * @brief 配置CODEC中的ALC左通道的参数
 * 
 * @param ALC_Type inner_codec_alc_config_t类型的结构体指针
 */
void inner_codec_left_alc_pro_mode_config(inner_codec_alc_config_t* ALC_Type)
{
    float   reg_pga_gain = 0.0f;

    unsigned int reg_max_level=0, reg_min_level=0;
    
    #if 0
    if(6.0f < ALC_Type->max_level)
    {
        ALC_Type->max_level = 0.0f;
    }
    else if(ALC_Type->max_level < ALC_Type->min_level)
    {
        CI_ASSERT(0,"Val of max_level or min_level erroe!\n");
    }
    else if(-90.3f > ALC_Type->max_level)
    {
        ALC_Type->max_level = -90.3f;
    }

    if(6 < ALC_Type->min_level)
    {
        ALC_Type->min_level = 0.0f;
    }
    else if(-90.3f > ALC_Type->min_level)
    {
        ALC_Type->min_level = -90.3f;
    }
    #endif

    reg_max_level = ALC_Type->max_level;//(unsigned int)(powf(10.0f,(ALC_Type->max_level/20.0f))*0x7fff);
    reg_min_level = ALC_Type->min_level;//(unsigned int)(powf(10.0f,(ALC_Type->min_level/20.0f))*0x7fff);

    CODEC->reg45 = reg_max_level;
    CODEC->reg46 = reg_max_level>>8;
    CODEC->reg47 = reg_min_level;
    CODEC->reg48 = reg_min_level>>8;

    CODEC->reg44 &= ~(7<<0);
    CODEC->reg44 |= (ALC_Type->samplerate<<0);
    CODEC->reg44 |= (7<<0);//TODO:ALC的采样速度设置为8K，最低的采样

    CODEC->reg44 &= ~(0xf<<4);


    CODEC->reg49 |= (0x01<<6);
    CODEC->rega  |= (0x01<<5);

    CODEC->rega  |= (0x01<<6);

    if(ALC_Type->alcmode != INNER_CODEC_ALC_MODE_NORMAL)
    {
        CODEC->reg40 |= (0x01<<6);
    }
    else
    {
        CODEC->reg40 &= ~(0x01<<6);
    }
    CODEC->reg40 &= ~(0x03<<4);
    CODEC->reg40 |= ((ALC_Type->alcmode&0x03)<<4);

    CODEC->reg40 &= ~(0x0f<<0);
    CODEC->reg40 |= ((ALC_Type->holdtime&0x0f)<<0);

    CODEC->reg41 &= ~(0x0f<<4);
    CODEC->reg41 |= ((ALC_Type->decaytime&0x0f)<<4);

    CODEC->reg41 &= ~(0x0f<<0);
    CODEC->reg41 |= ((ALC_Type->attacktime&0x0f)<<0);

    if(28.5 < ALC_Type->pga_gain)
    {
        ALC_Type->pga_gain = 28.5f;
    }
    else if(-18 > ALC_Type->pga_gain)
    {
        ALC_Type->pga_gain = -18.0f;
    }
    reg_pga_gain = (ALC_Type->pga_gain+18.0f)/1.5f;

    CODEC->reg43  |= (((int)reg_pga_gain)&0x1f);

    CODEC->reg49 &= ~(0x07<<0);
    CODEC->reg49 |= ((ALC_Type->min_pga_gain&0x07)<<0);

    CODEC->reg49 &= ~(0x07<<3);
    CODEC->reg49 |= ((ALC_Type->max_pga_gain&0x07)<<3);

    CODEC->reg4b = 0x1f;
    CODEC->reg4c = 0x1;
    CODEC->reg4d = 0x1;
}





/**
 * @brief ALC判断的信号来自高通滤波器之前，还是之后
 *
 * @param judge 选择inner CODEC的判断信号来自ALC之前还是之后
 */
void inner_codec_alc_judge_sel(inner_codec_alc_judge_t judge)
{
    CODEC->rega  &= ~(0x01<<6);

    CODEC->rega  |= ((1&judge)<<6);
}





/**
 * @brief 使能ALC的左通道
 *
 * @param gate ALC开关
 * @param is_alc_ctr_pga 是否使用ALC控制PGA的增益
 */
void inner_codec_left_alc_enable(inner_cedoc_gate_t gate,inner_codec_use_alc_control_pgagain_t is_alc_ctr_pga)
{
    uint32_t reg_tmp;

    reg_tmp = CODEC->rega;
    reg_tmp &= ~(0x01<<5);
    reg_tmp |= (is_alc_ctr_pga << 5);
    CODEC->rega = reg_tmp;

    CODEC->reg44 |= 1<<3;

    reg_tmp = CODEC->reg49;
    reg_tmp &= ~(0x01<<6);
    reg_tmp |= ((0x01&gate)<<6);
    CODEC->reg49 = reg_tmp;
}


/**
 * @brief 打开或关闭CODEC中的noise gate的开关
 *
 * @param cha 左右声道选择；
 * @param Threshold 噪声门限大小；
 * @param gate 噪声门限开关
 */
void inner_codec_noise_gate_set(inner_codec_cha_sel_t cha,inner_codec_noise_gate_threshold_t Threshold,inner_cedoc_gate_t gate)
{
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        CODEC->reg42 &= ~(0x07<<0);
        CODEC->reg42 |= (Threshold<<0);

        CODEC->reg42 &= ~(0x01<<3);
        CODEC->reg42 |= (gate<<3);
//    }
    
}


/**
 * @brief 打开或关闭CODEC中的超过满量程的87.5%之后快速减小增益这种模式
 *
 * @param cha 声道选择
 * @param gate 开关
 */
void inner_codec_87_5_fast_decrement_set(inner_codec_cha_sel_t cha, inner_cedoc_gate_t gate)
{
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        CODEC->reg42 &= ~(0x01<<4);
        CODEC->reg42 |= (gate<<4);
//    }
    
}


/**
 * @brief 打开或关闭CODEC中的zero cross的开关
 *
 * @param cha 左右声道选择；
 * @param gate 开关
 */
void inner_codec_zero_cross_set(inner_codec_cha_sel_t cha, inner_cedoc_gate_t gate)
{
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        CODEC->reg42 &= ~(0x01<<6);
        CODEC->reg42 |= (gate<<6);

        CODEC->reg24 &= ~(0x01<<4);
        CODEC->reg24 |= (gate<<4);
//    }
}


/**
 * @brief 关闭CODEC中的ALC
 *
 * @param cha 通道选择
 * @param ALC_Gain 关闭ALC之后，使用寄存器0x27控制ALC增益的大小
 */
void inner_codec_alc_disable(inner_codec_cha_sel_t cha,float ALC_Gain)
{
    float Reg_ALC_Gain = 0;

//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        CODEC->reg49 &= ~(0x01<<6);
        CODEC->rega  &= ~(0x01<<5);
        if(28.5 < ALC_Gain)
        {
            ALC_Gain = 28.5;
        }
        else if(-18 > ALC_Gain)
        {
            ALC_Gain = -18;
        }
        Reg_ALC_Gain = (ALC_Gain+18)/1.5;
        CODEC->reg27  = (int)Reg_ALC_Gain;
//    }
}


/**
 * @brief 向CODEC reg27 reg28寄存器写PGA的增益
 *
 * @param cha 左通道或者右通道
 * @param gain PGA增益的大小
 */
void inner_codec_pga_gain_config_via_reg27_28(inner_codec_cha_sel_t cha,uint32_t gain)
{
    if(gain > 0x1f)
    {
        CI_ASSERT(0,"gain err!\n");
    }
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        CODEC->reg27 = gain;
//    }
}


void inner_codec_pga_gain_config_via_reg27_28_db(inner_codec_cha_sel_t cha,float gain_db)
{
    float gain = gain_db;
    if(28.5f < gain)
    {
        gain = 28.5f;
    }
    else if(-18.0f > gain)
    {
        gain = -18.0f;
    }
    uint32_t reg_gain = (uint32_t)((gain + 18.0f)/1.5f);
    inner_codec_pga_gain_config_via_reg27_28(cha,reg_gain);
}



void inner_codec_pga_gain_config_via_reg43_53(inner_codec_cha_sel_t cha,uint32_t gain)
{
    uint32_t gain_reg_data = gain & 0x1f;
    uint32_t reg43_data = CODEC->reg43;
//    uint32_t reg53_data = CODEC->reg53;
    if(gain > 0x1f)
    {
        CI_ASSERT(0,"gain err!\n");
    }
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        reg43_data &= ~(0x1f);
        reg43_data |= gain_reg_data;
        reg43_data &= ~(1<<5);
        CODEC->reg43 = reg43_data;
//    }
}


void inner_codec_pga_gain_config_via_reg43_53_db(inner_codec_cha_sel_t cha,float gain_db)
{
    float gain = gain_db;
    if(28.5f < gain)
    {
        gain = 28.5f;
    }
    else if(-18.0f > gain)
    {
        gain = -18.0f;
    }
    uint32_t reg_gain = (uint32_t)((gain + 18.0f)/1.5f);
    inner_codec_pga_gain_config_via_reg43_53(cha,reg_gain);
}



/**
 * @brief 获取CODEC MIC增益的大小
 *
 * @param cha 通道选择
 */
inner_codec_mic_amplify_t inner_codec_get_mic_gain(inner_codec_cha_sel_t cha)
{
    volatile uint32_t tmp = 0;
    inner_codec_mic_amplify_t mic_amp;
    tmp = CODEC->reg25;
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        tmp = CODEC->reg25 & (0x3 << 6);
        mic_amp = (inner_codec_mic_amplify_t)(tmp >> 6);
//    }
    return mic_amp;
}


/**
 * @brief 向CODEC MIC增益控制寄存器写值
 *
 * @param cha 左通道或者右通道
 * @param gain MIC增益的大小
 */
void inner_codec_set_mic_gain(inner_codec_cha_sel_t cha,inner_codec_mic_amplify_t gain)
{
//    if(INNER_CODEC_LEFT_CHA == cha)
//    {
        CODEC->reg25 &= ~(3 << 6);
        CODEC->reg25 |= (gain << 6);
//    }
    _delay(50);
}



/**
 * @brief inner CODEC ALC配置左通道
 * 
 * @param ALC_str inner_codec_alc_use_config_t类型结构体指针
 * @param ALC_cha 需要开启的ALC通道配置
 */
void inner_codec_alc_left_config(inner_codec_alc_use_config_t* ALC_str)
{
    inner_codec_alc_config_t ALC_Config_str_l;
    memset(&ALC_Config_str_l,0,sizeof(inner_codec_alc_config_t));
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    ALC_Config_str_l.attacktime = ALC_str->attack_time;
    ALC_Config_str_l.decaytime = ALC_str->decay_time;
    ALC_Config_str_l.holdtime = ALC_str->hold_time;
    ALC_Config_str_l.max_pga_gain = ALC_str->alc_maxgain;
    ALC_Config_str_l.min_pga_gain = ALC_str->alc_mingain;
    ALC_Config_str_l.pga_gain = 0;
    ALC_Config_str_l.max_level = ALC_str->max_level;
    ALC_Config_str_l.min_level = ALC_str->min_level;
    ALC_Config_str_l.alcmode = INNER_CODEC_ALC_MODE_JACKWAY3;
    ALC_Config_str_l.samplerate = ALC_str->sample_rate;
    #else
    ALC_Config_str_l.attacktime = INNER_CODEC_ALC_ATTACK_TIME_64MS;
    ALC_Config_str_l.decaytime = INNER_CODEC_ALC_DECAY_TIME_500US;
    ALC_Config_str_l.holdtime = INNER_CODEC_ALC_HOLD_TIME_4MS;
    ALC_Config_str_l.max_pga_gain = ALC_str->alc_maxgain;
    ALC_Config_str_l.min_pga_gain = ALC_str->alc_mingain;
    ALC_Config_str_l.max_level = ALC_str->max_level;
    ALC_Config_str_l.min_level = ALC_str->min_level;
    ALC_Config_str_l.alcmode = INNER_CODEC_ALC_MODE_JACKWAY3;
    ALC_Config_str_l.samplerate = ALC_str->sample_rate;
    #endif

    inner_codec_left_alc_pro_mode_config(&ALC_Config_str_l);
    
    #if !AUDIO_CAPTURE_USE_CONST_PARA
    inner_codec_noise_gate_set(INNER_CODEC_LEFT_CHA,ALC_str->noise_gate_threshold,ALC_str->noise_gate);

    inner_codec_87_5_fast_decrement_set(INNER_CODEC_LEFT_CHA,ALC_str->fast_decrece_87_5);

    inner_codec_zero_cross_set(INNER_CODEC_LEFT_CHA,ALC_str->zero_cross);
    #else
    inner_codec_noise_gate_set(INNER_CODEC_LEFT_CHA,INNER_CODEC_NOISE_GATE_THRE_81dB,INNER_CODEC_GATE_ENABLE);

    inner_codec_87_5_fast_decrement_set(INNER_CODEC_LEFT_CHA,INNER_CODEC_GATE_DISABLE);

    inner_codec_zero_cross_set(INNER_CODEC_LEFT_CHA,INNER_CODEC_GATE_ENABLE);
    #endif
    
    inner_codec_use_alc_control_pgagain_t pga_control = INNER_CODEC_USE_ALC_CONTROL_PGA_GAIN;
    
    if(INNER_CODEC_GATE_DISABLE == ALC_str->alc_gate)
    {//ALC关闭，一定不使用ALC控制PGA增益
        pga_control = INNER_CODEC_WONT_USE_ALC_CONTROL_PGA_GAIN;
    }
    else if((INNER_CODEC_GATE_ENABLE == ALC_str->alc_gate) && (INNER_CODEC_GATE_ENABLE == ALC_str->use_ci112x_alc))
    {//CODEC 打开，使用CODEC自己的ALC，使用ALC控制
        pga_control = INNER_CODEC_WONT_USE_ALC_CONTROL_PGA_GAIN;
    }
    else
    {
        pga_control = INNER_CODEC_USE_ALC_CONTROL_PGA_GAIN;
    }

    inner_codec_left_alc_enable(ALC_str->alc_gate,pga_control);
    
    inner_codec_alc_judge_sel(INNER_CODEC_ALC_JUDGE_AFTER_HIGHPASS);
    
    if(0 != ALC_str->digt_gain)
    {
        inner_codec_adc_dig_gain_set_left(ALC_str->digt_gain);
    }

    if(INNER_CODEC_GATE_ENABLE == ALC_str->use_ci112x_alc)
    {
        #if USE_CI112X_OWN_ALC
        Scu_SetDeviceGate(HAL_ALC_BASE,ENABLE);
        eclic_irq_enable(ALC_IRQn);

        alc_aux_config_t ALC_AUX_L_Config_Str;
        memset(&ALC_AUX_L_Config_Str,0,sizeof(alc_aux_config_t));
        ALC_AUX_L_Config_Str.step_grd = ALC_STEP_GRD_0_5dB;
        ALC_AUX_L_Config_Str.step_cnt = ALC_STEP_CNT_3;
        ALC_AUX_L_Config_Str.db_offset = ALC_str->digt_gain - 106;
        ALC_AUX_L_Config_Str.step_tmr_atk = 6833*(1<<ALC_str->attack_time);//164*10000;  //162*1000对应1ms,20500对应125us,6833=20500/3
        ALC_AUX_L_Config_Str.step_tmr_dcy = 27333*(1<<ALC_str->decay_time);//164*10000;  //162*1000对应1ms
        ALC_AUX_L_Config_Str.cfg_addr = 0x4003f020;
        ALC_AUX_L_Config_Str.min_db_alert = 0x0d;//
        ALC_AUX_L_Config_Str.max_db_alert = 0x1e;//只有当CODEC输出的PGA的增益值大于（>）设定的值的时候，才会触发这两个中断
        ALC_AUX_L_Config_Str.atk_limit_num = 3;
        ALC_AUX_L_Config_Str.dcy_limit_num = 3;

        alc_aux_left_config(&ALC_AUX_L_Config_Str);

        /*配置ALC的中断*/
        alc_aux_int_t ALC_AUX_Int_str = {0};
        ALC_AUX_Int_str.crs_down_min_db_int_en = DISABLE;
        ALC_AUX_Int_str.crs_up_max_db_int_en = DISABLE;
        ALC_AUX_Int_str.gain_change_int_en = DISABLE;
        ALC_AUX_Int_str.gain_in_change_int_en = DISABLE;
        ALC_AUX_Int_str.atk_limit_int_en = DISABLE;
        ALC_AUX_Int_str.dcy_limit_int_en = DISABLE;

        alc_aux_intterupt_config(&ALC_AUX_Int_str,ALC_Left_Cha);

        /*配置ALC的全局寄存器*/
        alc_aux_globle_config_t ALC_AUX_Glb_Str;
        memset(&ALC_AUX_Glb_Str,0,sizeof(alc_aux_globle_config_t));
        ALC_AUX_Glb_Str.left_gain_sel = ALC_L_GAIN_FROM_CODEC_L;
        ALC_AUX_Glb_Str.right_gain_sel = ALC_R_GAIN_FROM_CODEC_R;
        ALC_AUX_Glb_Str.detect_num = 40;//ALC(AHB)/MCLK*2为最佳，只在选择ALC_DETECT_MD_NUM模式，此值有效，最多6位2进制
        ALC_AUX_Glb_Str.detect_mode = ALC_DETECT_MD_GAIN_CHANGE;//ALC_DETECT_MD_GAIN_CHANGE;//ALC_DETECT_MD_NUM;

        alc_aux_globle_config(&ALC_AUX_Glb_Str);
        alc_aux_left_cha_en(ENABLE);
        alc_aux_globle_enable(ENABLE);  
        #else
        CI_ASSERT(0,"please open the define\n");
        #endif
    }
}






uint8_t inner_codec_get_alc_gain_left(void)
{
    return CODEC->reg4c;
}

void inner_codec_left_mic_reinit(void)
{
    uint32_t reg_data = CODEC->reg24;
    reg_data &= ~(1<<6);
    CODEC->reg24 = reg_data;
    _delay(30);
    reg_data |= (1<<6);
    CODEC->reg24 = reg_data;
}



/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
