#ifndef CI_AUDIO_WRAPFFT_H
#define CI_AUDIO_WRAPFFT_H

#include "user_config.h"
/*
 * Readme first:
 * 2019.07.02, Max mic_channels 4, Max mic_channels 2,channel_id = [0...5], num_channels <= 6;
 *                    Modify the PRIVATE_NAME when you use the module in different alg modules, or error when linking( redefine function )
 */
/* macros
***************************************************************************************/
#define USE_IFFT (SEND_POSTALG_VOICE_TO_ASR || 0)

#define HARDWARE_FFT_ENABLE  (0)  /*!是否采用硬件fft,1为采用硬件fft,0为采用软件fft*/

typedef struct
{
    float real;
    float image;
} Complex;

typedef struct
{
	int frame_size;   //帧数
	int fft_frm_size; //fft长度
	int frame_shift;  //桢移
	int fft_size;     //fft输出长度
}fft_config_t;

typedef struct
{
    int num_channels;
    int mic_channels;
    int ref_channels;
    int frame_size;
    int frame_shift; 
    int frame_overlap_size;
    int fft_frm_size; //fft长度
    int fft_size;
    int enable_hardware_fft;
    
    short *mic[4];
    short *ref[2];
    short *dst_out;
    
    short *frame_mic_data[4]; 
    short *frame_ref_data[2]; 
    short *fft_window; 
    
    float *fft_mic_out[4];
    float *fft_ref_out[2];
    float *window; 
    short *fix_window;
    float *ifft_result_buf; 
    float *ifft_tmp_buf; 
    float *adjust_gain;
    float *psd;
    float *fft_buf_tmp;

       
} ci_wrapfft_audio;

#ifdef __cplusplus
extern "C" {
#endif

    /****************************************************************************
     * NAME:     ci_wrapfft_development_version
     * FUNCTION: The current version when developing
     * NOTE:     None
     * ARGS:     None
     *           
     * RETURN:   The version id, for example, 2019031601
     ****************************************************************************/
   
  

    int ci_audio_wrafft_version( void );

    //ci_wrapfft_audio* ci_warpfft_create(int mic_channel_num,int ref_channel_num, int frame_size, int enable_hardware_fft);
    
    ci_wrapfft_audio* ci_warpfft_create(int mic_channel_num,int ref_channel_num, fft_config_t * fft_config);
    int ci_wrapfft_ifft( void *handle,  float *fft,short *pcm_out);
   
    int ci_fft_deal(void *handle);
    int ci_warpfft_free( void *handle);

#ifdef __cplusplus
}
#endif


#endif /* CI_AUDIO_WRAPFFT_H */
