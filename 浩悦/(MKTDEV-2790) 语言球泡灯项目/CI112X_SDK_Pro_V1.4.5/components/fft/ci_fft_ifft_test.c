
#include "ci_fft_ifft_test.h"
#include "ci_fft.h"
//#include "ci_alg_malloc.h"
#include "ci_log.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "debug_time_consuming.h"



/*!fft大小，实部虚部*/
#define FRM_FFT_LENTH  1024 
/*!帧长大小*/
#define FRM_LENTH      1024 
/*!帧移大小*/
#define FRM_SHIFT      512

#define CHECK_MALLOC_RETURN_IF_FAILED(a) do{ if(a){ return -1;}}while(0)

/* global vars. definitions
***************************************************************************************/


static float *hannig_win = NULL;//[FRM_LENTH];
//static short *hannig_win_hardware = NULL;



static int hw_sin_win_init(short * out_win,int win_len)
{
    int i = 0;
    
    const float step = 3.1415926f / (float)win_len;
    const float t = 0.5f * 3.1415926f / (float)win_len ;
    float v = t;
    for (i=0; i<win_len; i++)
    {
        int r = (int)( 32767.0f * sinf(v));
        if (r > 32767)
        { 
            r = 32767;
        }
        out_win[i] = r;
        v = v + step;
    }
    return 0;
}

static void sin_window( float *w, int wlen )
{
    int i = 0;
    const float step = 3.1415926f / (float)wlen ;
    const float t = 0.5f * 3.1415926f / (float)wlen ;
    float v = t;
    for (i=0; i<wlen; i++)
    {
        w[i] = sinf(v);//  w[i] = 0.54f - 0.46f*cosf(v);hamming
        v = v + step;
    }
}


/****************************************************************************
 * read the *.h file please
 ****************************************************************************/

//就算是多通道，也可以复用一个的buffer初始化
static int8_t ci_fft_ifft_resample_init_0(void)
{
    int8_t ret_malloc = 0;
    hannig_win = (float*)malloc( (FRM_LENTH )*sizeof(float));
    //hannig_win_hardware = (short*)malloc( FRM_LENTH * sizeof(short) );//硬件fft使用的汉宁窗，浮点数
    //if(!hannig_win_hardware || !hannig_win)
    if( !hannig_win)
    {
        ci_logerr(LOG_AUDIO_IN,"%s().%d: malloc failed, ", __FUNCTION__, __LINE__ );
    }

    sin_window(hannig_win,FRM_LENTH);

    //hw_sin_win_init(hannig_win_hardware,FRM_LENTH);
    int ret = 1;
    ret = ci_software_fft_w512_s256_init();//单独
    if (ret != 0)
    {
        mprintf( "ci_fft_create() failed, exit.\n" );
    }
    return 0;
}


static int ci_fft_ifft_test_create_one_cha(ci_fft_ifft_creat_t* S)
{  
    int16_t* pcmbuf;
    float* win_pcm;
    float* fft_buf;
    float* audioTmp;
    int8_t ret = 0;

    pcmbuf     = (short*)malloc( (FRM_LENTH )*sizeof(short));  
    win_pcm = (float*)malloc((FRM_FFT_LENTH )*sizeof(float));
    fft_buf = (float*)malloc((FRM_FFT_LENTH + 2 )*sizeof(float)); 
    audioTmp = (float*)malloc( (FRM_LENTH-FRM_SHIFT )*sizeof(float));
    if(!pcmbuf || !win_pcm || !fft_buf || !audioTmp)
    {
        ci_logerr(LOG_AUDIO_IN,"%s().%d: malloc failed, ", __FUNCTION__, __LINE__ );
    }
    
    S->pcmbuf = pcmbuf;
    S->win_pcm = win_pcm;
    S->fft_buf = fft_buf;
    S->audioTmp = audioTmp;  

    memset(pcmbuf,0,sizeof(short)*FRM_LENTH);
    memset(win_pcm,0,sizeof(float)*FRM_FFT_LENTH);
    memset(fft_buf,0,sizeof(float)*(FRM_FFT_LENTH + 2 ));

    memset(audioTmp,0,sizeof(float)*(FRM_LENTH-FRM_SHIFT));

    return ret;
}

int ci_fft_ifft_test_create(ci_fft_ifft_creat_t* S)
{
    //共用buffer，只初始化一次
    static int8_t fft_ifft_resample_commen_inited = 0;
    if(0 == fft_ifft_resample_commen_inited)
    {
        ci_fft_ifft_resample_init_0();
        fft_ifft_resample_commen_inited = 3;
    }

    ci_fft_ifft_test_create_one_cha(S);
    return 0;
}


/****************************************************************************
 * read the *.h file please
 ****************************************************************************/
#pragma location = ".text_sram"
int ci_fft_ifft_test_deal(ci_fft_ifft_creat_t* S,short *pcm_in, float *fft_in, short *pcm_out, float *fft_out )
{  
    int16_t* pcmbuf = S->pcmbuf;
    float* win_pcm = S->win_pcm;
    float* fft_buf = S->fft_buf;
    float* audioTmp = S->audioTmp;
    float tmp = 0.0f; 
    int ret = 0;
#if  1//ARM_MATH_FUNC_EN   //TODO :OPT9_TEST   org :15us opt  memcpy 6us
     memcpy( pcmbuf+FRM_LENTH-FRM_SHIFT, pcm_in , FRM_SHIFT * sizeof(short) );    
#else   

    //11us--psram
    for(int i=0;i < FRM_SHIFT;i++)
    {
        pcmbuf[FRM_LENTH-FRM_SHIFT +i] = pcm_in[i];
    }
#endif
    //60us--psram
    ret = ci_software_fft_w512_s256( pcmbuf,hannig_win, fft_buf);//输出256
    if (ret != 0)
    {
        mprintf( "%s().fft failed, exit\n", __FUNCTION__ );
        return -__LINE__;
    } 
#if 1   //TODO :OPT10_TEST   org 15us  opt  memcpy 6us
    //4us--psram
    memcpy( pcmbuf , pcmbuf+FRM_SHIFT ,(FRM_LENTH-FRM_SHIFT) * sizeof(short) ); 
#else
    for( int i=0;i < FRM_LENTH-FRM_SHIFT;i++)
    {
        pcmbuf[i] = pcmbuf[FRM_SHIFT+i];   
    }  
#endif
    //7us--psram
    for(int i = 129;i<257;i++)
    {
        fft_buf[2*i] = 0.0f;
        fft_buf[2*i+1] = 0.0f;
    }   
    //74us--psram
    ret = ci_software_ifft_w512_s256( (float const*)fft_buf, win_pcm );
    if (ret != 0)
    {
        return -__LINE__;
    }          
#if 0  
    //143us--psram
    //58us--sram
//    init_timer0();
//    timer0_start_count();
    for(int j = 0; j<FRM_LENTH-FRM_SHIFT; j++)
    {    
        audioTmp[j] = ((win_pcm[j])*hannig_win[j]) + audioTmp[j]; 
        audioTmp[j] = (audioTmp[j]*g[j]);

        if((audioTmp[j]) >32767.0f)
        {	
            audioTmp[j] = 32767.0f;
        }
        else if((audioTmp[j]) < -32768.0f)
        {
            audioTmp[j] = -32768.0f;
        }
    }
//    timer0_end_count("1");

    //7us--psram
    memcpy(win_pcm,audioTmp,(FRM_LENTH-FRM_SHIFT)*sizeof(float));
   //  memcpy(pcm_out,audioTmp,FRM_SHIFT*sizeof(float));//6us
    //42us--psram
    for(int j = 0; j<FRM_LENTH-FRM_SHIFT; j++)
    { 
        audioTmp[j] = (win_pcm[j+FRM_SHIFT]*hannig_win[j+FRM_SHIFT]);//armfft  
        pcm_out[j] = (short)win_pcm[j];
    }
    //111us--psram
    //35us--sram
    for(int j = 0; j<FRM_LENTH-FRM_SHIFT; j++)
    {   
        if((audioTmp[j]) >32767.0f)//32767双精度？，32767.0f会少一点时间？
        {
            audioTmp[j] = 32767.0f;
        }
        else if((audioTmp[j]) < -32768.0f)
        {
            audioTmp[j] = -32768.0f;
        }
    }  
#else
    //init_timer0();
    //timer0_start_count();
    //SRAM 63us
    for ( int i = 0; i < FRM_SHIFT; i++ )
    {
        tmp = (win_pcm[i] * hannig_win[i] + audioTmp[i]);
        int tt = (int)tmp;
        
        if(tt > 32767 )//if(tt > 32767 || (((*((unsigned int*)&tmp) & 0x7)>>2) > 5));need_to_deal
        {
           pcm_out[i] = 32767;
        }
        else if(tt < -32768)//else if(tt < -32768 || (((*((unsigned int*)&tmp) & 0x7)>>2) < -5));;need_to_deal
        {
           pcm_out[i] = -32768;
        }
        else
        {
          pcm_out[i] = (short)tt;         
        }
        audioTmp[i] = ( win_pcm[i+FRM_SHIFT] ) * hannig_win[i+FRM_SHIFT];
    }   
    //timer0_end_count_only_print_time_us();
#endif
    return 0;
}


