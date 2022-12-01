/**
  ******************************************************************************
  * @文件    asr_api.h
  * @版本    V1.0.1
  * @日期    2019-08-02
  * @概要  asr 系统外部接口使用及说明
  ******************************************************************************
  * @注意
  *
  * 版权归chipintelli公司所有，未经允许不得使用或修改
  *
  ******************************************************************************
  */ 
  
#include "asr_api.h"
#include <stdlib.h>
#include "sdk_default_config.h"
#include "ci_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ci_flash_data_info.h"

extern int get_voice_snr_info(int * v, int* n,int*s);

#define USE_DSPK_EN (0)
#define USE_ASR_DY_SKP_EN (0)
#define USE_LARGE_DIM_MDL (0)
#define USE_CMP_TIWCE_CONTINUE_MODE	(0)

#define USE_MAX_STOP_CONFIDENCE (0)
#define USE_ASRLITE_EN (0)
#define DEBUG_PRINT_VOICE_INFO (0)


#define V_DIV_N_MIN_TH (140)
#define V_DIV_N_MAX_TH (168)
#define N_QI_MIN_TH (13200)
#define V_DIV_N_MIN_AND_N_QI_MIN_TH (160)

int dy_adj_cmpt_callback(void)
{
    int v = 0;
    int n = 0;
    int s = 0;
    float normal_snr = 0;
    int voice_noise = 0;
    int rdy = get_voice_snr_info(&v,&n,&s);
    if(!rdy)
    {
      return -2;
    }

    #if DEBUG_PRINT_VOICE_INFO
    mprintf("v = %d,n=%d,s=%d\n",v,n,s);
    #endif

    if((s <= V_DIV_N_MIN_TH)|| ((n < N_QI_MIN_TH) && (s < V_DIV_N_MIN_AND_N_QI_MIN_TH)))
    {//--
      return -1;
    }
    else if( (s > V_DIV_N_MIN_TH) && (s <= V_DIV_N_MAX_TH) )
    {//keep
      return 0;
    }
    else if( s > V_DIV_N_MAX_TH)
    {//++
      return 1;
    }else
    {//keep
      return 0;
    }
}

int dy_cfg()
{
#if ADAPTIVE_THRESHOLD
    dynmic_confidence_en_cfg(1);
    dynmic_confidence_config(-20,10,1);
#else
    dynmic_confidence_en_cfg(0);
#endif
    return 0;
}
  
  

/**
 * @brief asr系统启动任务
 * 
 */
void asr_system_startup_task(void* p)
{
    
#if USE_LARGE_DIM_MDL
    extern int asr_use_large_dim_mdl_cfg(int);
    asr_use_large_dim_mdl_cfg(1);
#endif 
    
    /* 1.create asr sys task */
    int ret = asrtop_taskmanage_create();

#ifdef ASR_SKIP_FRAME_CONFIG

	#if (ASR_SKIP_FRAME_CONFIG != 2)
		extern int asrtop_dskp_cfg(void);
		asrtop_dskp_cfg();
	#if (ASR_SKIP_FRAME_CONFIG == 1)
		asr_dynamic_skip_open();
	#endif
#endif

#endif

    dy_cfg();


    /* 2.load model flash to ram send_startup_msg_to_decoder */
    unsigned int lm_model_addr;
    unsigned int lm_model_szie;

    unsigned int ac_model_addr;
    unsigned int ac_model_size;

    get_current_model_addr((uint32_t *)&ac_model_addr, (uint32_t *)&ac_model_size, (uint32_t *)&lm_model_addr, (uint32_t *)&lm_model_szie);

    unsigned int start_tick = xTaskGetTickCount();

    //ci_loginfo(LOG_USER,"asr_init start\n");
    
    int err = asrtop_asr_system_start(lm_model_addr,lm_model_szie,ac_model_addr,ac_model_size,0);

	if(err < 0)
	{
		mprintf("asr create err\n");
	}

    unsigned int end_tick = xTaskGetTickCount();

    mprintf("asr_init done[%d ticks]\n",end_tick - start_tick);
    
    char asr_ver_buf[100]={0};
    
    get_asr_sys_verinfo(asr_ver_buf);
    
    mprintf("asr_ver:[%s]\n",asr_ver_buf);
	
#if  USE_ASRLITE_EN

    int lite_ac_mdl_id = 1;
    int lite_lg_mdl_id = 2;

    get_dnn_addr_by_id(lite_ac_mdl_id,&ac_model_addr,&ac_model_size);

    get_dnn_addr_by_id(lite_lg_mdl_id,&lm_model_addr,&lm_model_szie);
    
    asrtop_asr_system_litecreate(lm_model_addr,lm_model_szie,ac_model_addr,ac_model_size,NULL);
	
#endif

#if USE_CMP_TIWCE_CONTINUE_MODE
	extern int cmp_twice_confidence_mode_cfg(int cmp_twice_confidence_mode);
	cmp_twice_confidence_mode_cfg(1);
#endif

#if USE_MAX_STOP_CONFIDENCE
	extern int max_stop_confidence_cfg(int max_stop_confidence);
	max_stop_confidence_cfg(40);
#endif

#if DEBUG_INFO_MEM
    for(;;)
    {
    	extern void get_fmem_status(void);
    	extern void get_mem_status(void);

    	get_fmem_status();
    	get_mem_status();
    	vTaskDelay(2000);
    }
#endif

#if HAS_POWER_FREQUENCY_DISTURB
    extern int config_vad_logscale(float scale_rate);
    config_vad_logscale(1.18);
    extern int tdvad_vadend_frames_update_cfg(int vadend_count);
    tdvad_vadend_frames_update_cfg(38);
    extern int fast_stop_vad_config(int cfg_en,int max_frm,int pcm_th);
    fast_stop_vad_config(1,30,3000);
#endif

    extern int tdvad_vadend_timeout_cfg(int frame_ms);
	tdvad_vadend_timeout_cfg(800);

#if ASR_TWICE_CLOSE
	extern int cmp_twice_config(short min_cfd,short twice_cfd,int max_twice_time);
	cmp_twice_config(90,90,5000);
#endif

#if ASR_DETAIL_RESULT
	extern int open_asr_detail_result(void);
	open_asr_detail_result();
#endif

    vTaskDelete(NULL);

}


/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
  
