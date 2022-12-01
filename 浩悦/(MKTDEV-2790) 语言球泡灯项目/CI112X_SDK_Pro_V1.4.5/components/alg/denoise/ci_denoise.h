/**
 * @file ci_denoise.h
 * @brief 
 * @version V1.0.0
 * @date 2019.07.09
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef CI_DENOISE_H
#define CI_DENOISE_H

/**
 * @addtogroup denoise
 * @{
 */

/*! 降噪门限设置，默认值为16*/
#define SET_DENOISE_THRESHOLD             (16)
/*! 降噪门限设置判断窗长，默认值为20*/
#define SET_DENOISE_THRESHOLD_WINDOW_SIZE (20)

#ifdef __cplusplus
extern "C" {
#endif

    
    
    /**
     * @brief  获得当前版本号
     * 
     * @return int, 版本号id,例如10000表示1.00.00;
     */
    int ci_denoise_version( void );


    /**
     * @brief        初始化模块
     * 
     * @return int   不等于NULL: 初始化成功, 其他：初始化存在异常
     */
    void* ci_denoise_create(float start_Hz,float end_Hz);


    /**
     * @brief            降噪处理中<br>
     * @param fft_in     数据来自mic , 数据是频域信号
     * @param fft_out    数据降噪处理后输出 , 数据是频域信号
     * @return int       0:降噪处理过程正常;其它:降噪处理过程中出现异常
     */
    int ci_denoise_deal(void* handle, float *fft_in, float *fft_out );
   
    /**
 	 * @brief				降噪门限设置
     * 
     * @param denoise_threshold    门限值决定当前噪声环境下是否进行降噪，默认值为 4.5
     * @window_size         窗长大小用于处理判断是否进行降噪，默认值为20
     * @return int          0:降噪门限设置成功;其它:降噪门限设置失败
     */ 
    int ci_denoise_set_threshold(void* handle, float api_denoise_threshold, int api_decide_frame_len);
    
     /**
     * @brief       获取当前降噪的状态，是否在降噪
     * 
     * @return int  0:没有进行降噪处理;1:进行降噪处理
     */
    int ci_denoise_get_status( void );
    float get_tmp_snr(void);
    float get_noise_estimation(void);
    int ci_denoise_config_performance(void* handle,float denoise_alpha_l,float denoise_alpha_m,float denoise_alpha_s,float denoise_beta,float denoise_G_min);
   
	
    
#ifdef __cplusplus
}
#endif

/** @} */


#endif //CI_DENOISE_H

