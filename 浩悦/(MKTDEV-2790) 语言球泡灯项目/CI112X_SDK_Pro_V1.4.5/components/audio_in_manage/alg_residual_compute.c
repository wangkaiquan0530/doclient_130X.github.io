#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ci_log.h"
#include "alg_residual_compute.h"


#define FRAME_SHIFT (40)
#define RESI_COMPUTE_ABS(X)  (X>0?X:(-X))


extern int asrtop_dynamic_confidence_mode_cfg(int confidence_mode);


static  short frm = 0;
static float alpha = 0.25f;
static float snr_high = 0.005f;//变化频换
static float snr_low = 0.0f;
static float snr_est = 0;
static int set_snr_para(float snr_high_tmp,float snr_low_tmp)
{
    snr_high = snr_high_tmp;
    snr_low = snr_low_tmp;
    return 0;
    
}
 
static float frame_residual_sum = 0;
static float frame_src_sum = 0;

static float frame_src_sum_compute = 3000*3000*256.0f;

static float frame_residual_sum_compute = 1000*1000*256.0f;
static float frame_src_sum_compute_thr = 8000*8000*256.0f;
static float frame_residual_sum_compute_thr = 100*100*256.0f;

static float get_frame_src_sum_compute(void)
{
    return frame_src_sum_compute;
}

static float get_frame_residual_sum_compute(void)
{
    return frame_residual_sum_compute;
}
static float alg_residual_compute( short *mic_data,  short *dst_data, int frame_length )
{
    
    short *src  = mic_data;
    short *dst  = dst_data;
    short *micl = src;
    int ret = 0;
    
    /*suppress compiler warning of unused parameter*/
    (void)src;
    (void)dst;
    (void)micl;
    (void)ret;
    
    
    
    float frame_residual_sum_ave = 0;
    float frame_src_sum_ave = 0;
    float frame_residual_sum_ave_test = 0;
    float frame_src_sum_ave_test = 0;
    
   
    if(frm < FRAME_SHIFT )
    {
        for (int i = 0; i<256; i++)
        {
             //frame_residual_sum += RESI_COMPUTE_ABS(dst[i]);
             //frame_src_sum += RESI_COMPUTE_ABS(src[i]); 
             frame_residual_sum += dst[i]*dst[i];
             frame_src_sum += src[i]*src[i]; 
              //mprintf("frame_src_sum = %f\n",frame_src_sum);
              //mprintf("frame_residual_sum = %f\n",frame_residual_sum);
        }
        frm++;
    }
    else
    {
        /*计算完FRAME_SHIFT内的残留和原始输入的平均值*/
        frame_residual_sum_ave = frame_residual_sum/frm;
        frame_src_sum_ave = frame_src_sum/frm;
        frame_src_sum_compute = frame_src_sum_ave;
       // mprintf("frame_residual_sum = %f\n",frame_residual_sum);
       // mprintf("frame_src_sum = %f\n",frame_src_sum);
       // mprintf("frm = %d\t frame_src_sum_ave = %f \n",frm,frame_src_sum_ave);
        
        
        
        frame_residual_sum_ave_test = (1-alpha)*frame_residual_sum_ave_test+alpha*frame_residual_sum_ave;
        frame_src_sum_ave_test = (1-alpha)*frame_src_sum_ave_test + alpha*frame_src_sum_ave;
        
        //mprintf("frame_residual_sum_ave_test = %f\t frame_src_sum_ave_test = %f \n",frame_residual_sum_ave_test,frame_src_sum_ave_test);
        
        snr_est = frame_residual_sum_ave/frame_src_sum_ave;
        
       // mprintf("snr_est = %f\t frame_residual_sum_ave = %f \n",snr_est,frame_residual_sum_ave);
        
        frame_residual_sum = 0;
        frame_src_sum = 0;
        frm = 0;
        /*下一个FRAME_SHIFT重新计算*/
        for (int i = 0; i<256; i++)
        {
             frame_residual_sum += dst[i];
             frame_src_sum += src[i];     
        }
        frm ++;
    }

    return snr_est;
}
static int alg_residual_compute_init(void)
{
    set_snr_para(100,20);
    return 0;
}

 int alg_res_compute_set_asr_mode( short *mic_data,  short *dst_data, int frame_length,int mode_change_enable )
{
    short *src  = mic_data;
    short *dst  = dst_data;
    short *micl = src;
    int ret = 0;
    float snr_est_temp = 0;
    float src_sum_compute = 0;
    float src_residual_compute = 0;
    /*suppress compiler warning of unused parameter*/
    (void)src;
    (void)dst;
    (void)micl;
    (void)ret;
   
    if(mode_change_enable == 1)
    {
        
         snr_est_temp = alg_residual_compute( src, dst, frame_length );
         src_sum_compute = get_frame_src_sum_compute();
         src_residual_compute = get_frame_residual_sum_compute();
         
        // mprintf("snr_est_temp = %f\n",snr_est_temp);
         if((snr_est_temp >= snr_high)&&(src_residual_compute > frame_residual_sum_compute_thr))
         {
             asrtop_dynamic_confidence_mode_cfg(-1);//调整模式为最大值模式
             //mprintf("the asrtop_dynamic_confidence_mode_cfg parameter is EASYMODE! \n");
             //mprintf("EASYMODE snr_est_temp = %f\t src_sum_compute=%f\n",snr_est_temp,src_sum_compute);
         }
         else if (snr_est_temp <= snr_low)
         {
             asrtop_dynamic_confidence_mode_cfg(0); //调整模式为平均模式
            // mprintf("NORMODE snr_est_temp = %f\n",snr_est_temp);
         }
         else
         {
             asrtop_dynamic_confidence_mode_cfg(0);//调整模式为平均模式
            // mprintf("NORMODE snr_est_temp = %f\n",snr_est_temp);
         } 
    }
    else if(mode_change_enable == 0)
    {
        //asrtop_dynamic_confidence_mode_cfg(0);//调整模式为平均模式
        asrtop_dynamic_confidence_mode_cfg(0);//TODO test测试模式  调整模式为容易模式
        //mprintf("the asrtop_dynamic_confidence_mode_cfg parameter is EASYMODE! \n");
    }
    else
    {
        asrtop_dynamic_confidence_mode_cfg(0);//调整模式为正常模式
        //mprintf("the mode_change_enable parameter is wrong! \n");
    }
    return 0;
}

