/**
 * @file ci_fft_ifft_test.h
 * @brief 
 * @version V1.0.0
 * @date 2020.05.18
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef CI_FFT_IFFT_TEST_H
#define CI_FFT_IFFT_TEST_H

#include <stdint.h>
/**
 * @addtogroup ci_fft_ifft_test
 * @{
 */


/**
 * @brief ci_fft_ifft_test输入模式
 * 
 */
typedef enum
{
    MODE_PCMIN_PCMOUT = 0,      /*!< 输入时域pcm输出时域pcm   */
    MODE_PCMIN_FFTOUT,          /*!< 输入时域pcm输出频域fft   */
    MODE_FFTIN_FFTOUT,          /*!< 输入频域fft输出频域fft   */
    MODE_FFTIN_PCMOUT,          /*!< 输入频域fft输出时域pcm   */
} MODE;




#ifdef __cplusplus
extern "C" {
#endif

  
typedef struct
{
    int16_t* pcmbuf;
    float* win_pcm;
    float* fft_buf;
    float* audioTmp;
}ci_fft_ifft_creat_t;


    /**
     * @brief        初始化模块，使用硬件fft
     * 
     * @param mode   输入输出模式选择, MODE_*, 目前仅支持时域信号的输入时域信号的输出;
     * @return int   0: 初始化成功, 其他：初始化存在异常 
     */
    int ci_fft_ifft_test_create(ci_fft_ifft_creat_t* S);


    /**
     * @brief            降噪处理中<br>
     *                   对于需要的数据输入必须进行内存分配，不需要的数据输入设置为NULL即可<br>
     *                   对于 MODE_PCMIN_PCMOUT: 仅使用 pcm_in 、 pcm_out;<br>
     *                   对于 MODE_PCMIN_FFTOUT: 仅使用 pcm_in 、fft_out;<br>
     *                   对于 MODE_FFTIN_FFTOUT: 仅使用 fft_in 、fft_out,<br>
     *                   对于  MODE_PCMIN_FFTOUT: 仅使用 pcm_in 、fft_out;<br>
     *                   fft*的数据格式类型为: real0_imag0, real1_imag1, ... real255_imag255;<br>
     *                   pcm*的数据格式类型为: short0, short1, ... short shift;<br>
     * @param pcm_in     数据来自mic , 数据是时域信号
     * @param fft_in     数据来自mic , 数据是频域信号
     * @param pcm_out    数据降噪处理后输出 , 数据是时域信号
     * @param fft_out    数据降噪处理后输出 , 数据是频域信号 
     * @return int       0:降噪处理过程正常;其它:降噪处理过程中出现异常 
     */
//    int ci_fft_ifft_test_deal( short *pcm_in, float *fft_in, short *pcm_out, float *fft_out );  
    int ci_fft_ifft_test_deal(ci_fft_ifft_creat_t* S, short *pcm_in, float *fft_in, short *pcm_out, float *fft_out );
	
    
#ifdef __cplusplus
}
#endif

/** @} */


#endif //ci_fft_ifft_test_H

