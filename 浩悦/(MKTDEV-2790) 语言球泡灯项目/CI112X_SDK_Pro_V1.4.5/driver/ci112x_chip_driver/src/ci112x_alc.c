/**
 * @file ci112x_alc.c
 * @brief CI112X的ALC模块驱动
 * @version 0.1
 * @date 2019-05-10
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */
#include "ci112x_alc.h"
#include "ci112x_system.h"
#include "ci112x_codec.h"
#include "ci_assert.h"


//ALC中断的标志位
#define L_CRS_UP_MAX_DB_INT     1 << 0
#define L_CRS_DOWN_MIN_DB_INT   1 << 1 
#define L_CRS_DOWN_MAX_DB_INT   1 << 4 
#define L_CRS_UP_MIN_DB_INT     1 << 5 
#define L_ATK_LIMIT_INT         1 << 8 
#define L_DCY_LIMIT_INT         1 << 9 
/*CODEC输出增益变化的时候，会输出一个翻转信号，这个中断就是检测这个信号*/
#define L_GAIN_CHANGE_INT       1 << 10
/*这个中断检测CODEC ALC输出的增益变化的信号，只要输出的增益变化就会产生这个中断*/
#define L_GAIN_CHANGE1_INT      1 << 11 
#define L_GAIN_DISCR_INT        1 << 12 
#define L_READ_ERR_INT          1 << 14

#define R_CRS_UP_MAX_DB_INT     1 << 0 <<16
#define R_CRS_DOWN_MIN_DB_INT   1 << 1 <<16
#define R_CRS_DOWN_MAX_DB_INT   1 << 4 <<16
#define R_CRS_UP_MIN_DB_INT     1 << 5 <<16
#define R_ATK_LIMIT_INT         1 << 8 <<16
#define R_DCY_LIMIT_INT         1 << 9 <<16
#define R_GAIN_CHANGE_INT       1 << 10 <<16
#define R_GAIN_CHANGE1_INT      1 << 11 <<16
#define R_GAIN_DISCR_INT        1 << 12 <<16
#define R_READ_ERR_INT          1 << 14 <<16


/**
 * @brief 配置CI110x中的ALC的中断使能
 * 
 * @param ALC_INT_Type alc_aux_int_t类型的结构体指针
 * @param cha 通道选择
 */
void alc_aux_intterupt_config(alc_aux_int_t* ALC_INT_Type,alc_aux_cha_t cha)
{
    if(ALC_Left_Cha == cha)
    {
        ALC->ALC_INT_EN &= ~(0xffff);
        ALC->ALC_INT_EN |= ALC_INT_Type->crs_up_max_db_int_en << 0 | ALC_INT_Type->crs_down_min_db_int_en << 1 
                        |ALC_INT_Type->crs_down_max_db_int_en << 4 | ALC_INT_Type->crs_up_min_db_int_en << 5 
                        |ALC_INT_Type->atk_limit_int_en << 8 | ALC_INT_Type->dcy_limit_int_en << 9 
                        |ALC_INT_Type->gain_change_int_en << 10 | ALC_INT_Type->gain_in_change_int_en << 11 
                        |ALC_INT_Type->gain_discr_int_en << 12 | ALC_INT_Type->read_err_int_en << 14;
    }
    
    else if(ALC_Right_Cha == cha)
    {
        ALC->ALC_INT_EN &= ~(0xffff << 16);
        ALC->ALC_INT_EN |= ALC_INT_Type->crs_up_max_db_int_en << 0 << 16 | ALC_INT_Type->crs_down_min_db_int_en << 1 << 16 
                        |ALC_INT_Type->crs_down_max_db_int_en << 4 << 16 | ALC_INT_Type->crs_up_min_db_int_en << 5 << 16 
                        |ALC_INT_Type->atk_limit_int_en << 8 << 16 | ALC_INT_Type->dcy_limit_int_en << 9 << 16 
                        |ALC_INT_Type->gain_change_int_en << 10 << 16 | ALC_INT_Type->gain_in_change_int_en << 11 << 16 
                        |ALC_INT_Type->gain_discr_int_en << 12 << 16 | ALC_INT_Type->read_err_int_en << 14 << 16;
    }
}


/**
 * @brief 配置CI110x中的ALC左通道的参数（这只是左通道的ALC一些参数的配置，
 * 还有右通道配置的函数，在ALC_Enable函数之前配置。）
 * 
 * @param ALC_Type alc_aux_config_t类型的结构体指针
 */
void alc_aux_left_config(alc_aux_config_t* ALC_Type)
{
    volatile uint32_t tmp = 0;
    
    if(ALC_Type->db_offset > 0xff)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->step_tmr_atk > 0x1ffffff)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->step_tmr_dcy > 0x1ffffff)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->min_db_alert > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->max_db_alert > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->atk_limit_num > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->dcy_limit_num > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    tmp = ALC_Type->step_grd <<0 | ALC_Type->step_cnt << 4 | ALC_Type->db_offset << 8;
    ALC->LEFT_CTRL = tmp;
    ALC->L_STEP_TMR_ATK = ALC_Type->step_tmr_atk;
    ALC->L_STEP_TMR_DCY = ALC_Type->step_tmr_dcy;
    ALC->L_CFG_ADDR = ALC_Type->cfg_addr;
    tmp = ALC_Type->min_db_alert << 0 | ALC_Type->max_db_alert << 8 |
        ALC_Type->atk_limit_num <<16 | ALC_Type->dcy_limit_num << 24;
    ALC->L_MIN_MAX_DB = tmp;
}


/**
 * @brief 配置CI110x中的ALC右通道的参数（这只是右通道的ALC一些参数的配置，
 * 还有左通道配置的函数，在ALC_Enable函数之前配置。）
 * 
 * @param ALC_Type alc_aux_config_t类型的结构体指针
 */
void alc_aux_right_config(alc_aux_config_t* ALC_Type)
{
    volatile uint32_t tmp = 0;
    
    if(ALC_Type->db_offset > 0xff)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->step_tmr_atk > 0x1ffffff)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->step_tmr_dcy > 0x1ffffff)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->min_db_alert > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->max_db_alert > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->atk_limit_num > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    if(ALC_Type->dcy_limit_num > 0x1f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    tmp = ALC_Type->step_grd <<0 | ALC_Type->step_cnt << 4 | ALC_Type->db_offset << 8;
    ALC->RIGHT_CTRL = tmp;
    ALC->R_STEP_TMR_ATK = ALC_Type->step_tmr_atk;
    ALC->R_STEP_TMR_DCY = ALC_Type->step_tmr_dcy;
    ALC->R_CFG_ADDR = ALC_Type->cfg_addr;
    tmp = ALC_Type->min_db_alert << 0 | ALC_Type->max_db_alert << 8 
        |ALC_Type->atk_limit_num <<16 | ALC_Type->dcy_limit_num << 24;
    ALC->R_MIN_MAX_DB = tmp;
}


/**
 * @brief 配置CI110x中的ALC GLB_CTRL寄存器的配置
 * 
 * @param ALC_Glb_Type alc_aux_globle_config_t类型的结构体指针
 */
void alc_aux_globle_config(alc_aux_globle_config_t* ALC_Glb_Type)
{
    ALC->GLB_CTRL &= ~((3 << 4)|(0x7f << 10));
    if(ALC_Glb_Type->detect_num > 0x7f)
    {
        CI_ASSERT(0,"alc config err!\n");
    }
    ALC->GLB_CTRL |= (ALC_Glb_Type->left_gain_sel << 4)
                    |(ALC_Glb_Type->right_gain_sel << 5)|(ALC_Glb_Type->detect_num << 10)
                    |(ALC_Glb_Type->detect_mode << 16);
}


/**
 * @brief AUX_ALC 左通道开关
 * 
 * @param cmd 
 */
void alc_aux_left_cha_en(FunctionalState cmd)
{
    ALC->GLB_CTRL &= ~(1 << 1);
    if(ENABLE == cmd)
    {
        ALC->GLB_CTRL |= (1<<1);
    }
}


/**
 * @brief AUX_ALC 右通道开关
 * 
 * @param cmd 
 */
void alc_aux_right_cha_en(FunctionalState cmd)
{
    ALC->GLB_CTRL &= ~(1 << 2);
    if(ENABLE == cmd)
    {
        ALC->GLB_CTRL |= (1<<2);
    }
}


/**
 * @brief ALC全局使能
 * 
 * @param cmd 打开或关闭
 */
void alc_aux_globle_enable(FunctionalState cmd)
{
    ALC->GLB_CTRL &= ~(1 << 0);
    ALC->GLB_CTRL |= (cmd & 0x1);
}


/**
 * @brief ALC中断回调函数
 * 
 */
void alc_interrupt_handler(void)
{
    volatile uint32_t tmp = 0;
    tmp = ALC->ALC_INT_STATUS;
    inner_codec_mic_amplify_t l_mic_gain = INNER_CODEC_MIC_AMP_0dB;
    inner_codec_mic_amplify_t r_mic_gain = INNER_CODEC_MIC_AMP_0dB;
    if(tmp & L_CRS_UP_MAX_DB_INT)//左通道原始信号幅值较小，需要将模拟增益增加
    {
        ALC->ALC_INT_CLR = L_CRS_UP_MAX_DB_INT;
        l_mic_gain = inner_codec_get_mic_gain(INNER_CODEC_LEFT_CHA);
        if(l_mic_gain < INNER_CODEC_MIC_AMP_20dB)
        {
            l_mic_gain++;
            //inner_codec_set_mic_gain(INNER_CODEC_LEFT_CHA,(inner_codec_mic_amplify_t)l_mic_gain);
        }
    }
    
    if(tmp & L_CRS_DOWN_MIN_DB_INT)//左通道原始信号幅值较大，需将模拟增益调小
    {
        ALC->ALC_INT_CLR = L_CRS_DOWN_MIN_DB_INT;
        l_mic_gain = inner_codec_get_mic_gain(INNER_CODEC_LEFT_CHA);
        if(l_mic_gain > INNER_CODEC_MIC_AMP_0dB)
        {
            l_mic_gain--;
            //inner_codec_set_mic_gain(INNER_CODEC_LEFT_CHA,(inner_codec_mic_amplify_t)l_mic_gain);
        }
    }
    
    if(tmp & R_CRS_UP_MAX_DB_INT)//右通道原始信号幅值较小，需要将模拟增益增加
    {
        ALC->ALC_INT_CLR = R_CRS_UP_MAX_DB_INT;
//        r_mic_gain = inner_codec_get_mic_gain(INNER_CODEC_RIGHT_CHA);
//        if(r_mic_gain < INNER_CODEC_MIC_AMP_20dB)
//        {
//            r_mic_gain++;
//            //inner_codec_set_mic_gain(INNER_CODEC_RIGHT_CHA,(inner_codec_mic_amplify_t)r_mic_gain);
//        }
    }
    
    if(tmp & R_CRS_DOWN_MIN_DB_INT)//右通道原始信号幅值较大，需将模拟增益调小
    {
        ALC->ALC_INT_CLR = R_CRS_DOWN_MIN_DB_INT;
//        r_mic_gain = inner_codec_get_mic_gain(INNER_CODEC_RIGHT_CHA);
        if(r_mic_gain > INNER_CODEC_MIC_AMP_0dB)
        {
            r_mic_gain--;
            //inner_codec_set_mic_gain(INNER_CODEC_RIGHT_CHA,(inner_codec_mic_amplify_t)r_mic_gain);
        }
    }
    #if 0
    if(tmp & L_GAIN_CHANGE_INT)//左通道检测到gain_ch翻转的中断
    {
        ALC->ALC_INT_CLR = L_GAIN_CHANGE_INT;
    }
    if(tmp & L_GAIN_CHANGE1_INT)//左通道检测到gain_in变化的中断
    {
        ALC->ALC_INT_CLR = L_GAIN_CHANGE1_INT;
    }
    
    if(tmp & R_GAIN_CHANGE_INT)//右通道检测到gain_ch翻转的中断
    {
        ALC->ALC_INT_CLR = R_GAIN_CHANGE_INT;
    }
    if(tmp & R_GAIN_CHANGE1_INT)//右通道检测到gain_in变化的中断
    {
        ALC->ALC_INT_CLR = R_GAIN_CHANGE1_INT;
    }
    
    if(tmp & L_ATK_LIMIT_INT)//左通道检测到处于连续ATTACK过程超过设定的次数
    {
        ALC->ALC_INT_CLR = L_ATK_LIMIT_INT;
    }
    if(tmp & L_DCY_LIMIT_INT)//左通道检测到处于连续DECAY过程超过设定的次数
    {
        ALC->ALC_INT_CLR = L_DCY_LIMIT_INT;
    }
    
    if(tmp & R_ATK_LIMIT_INT)//右通道检测到处于连续ATTACK过程超过设定的次数
    {
        ALC->ALC_INT_CLR = R_ATK_LIMIT_INT;
    }
    if(tmp & R_DCY_LIMIT_INT)//右通道检测到处于连续DECAY过程超过设定的次数
    {
        ALC->ALC_INT_CLR = R_DCY_LIMIT_INT;
    }
    #endif
}
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
