/**
 * @file alc_auto_switch.h
 * @brief 
 * @version V1.0.0
 * @date 2021.05.14
 * 
 * @copyright Copyright (c) 2021 Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef ALC_AUTO_SWITCH_H
#define ALC_AUTO_SWITCH_H

/**
 * @addtogroup alc_auto_switch
 * @{
 */

       /**
     * @brief  初始化函数
     *
     * @return void, 无返回;
     */
     extern void* alc_auto_switch_create(void);


     /**
      * @brief  调用函数
      * @param  hd handle
      * @return int, 运行成功返回0;
      */
     extern int switch_alc_state_automatic(void* hd,short* pcm,int length);
     
     
     /**
      * @brief  门限设置
      * @param  hd handle
      * @param  pcm_back_on     alc_on状态下进入alc off的背景能量门限值
      * @param  pcm_back_off    alc_off状态下进入alc on的背景能量门限值
      * @param  pcm_max_on      alc_on状态下进入alc off的最大能量门限值
      * @param  pcm_max_off     alc_off状态下进入alc on的最大能量门限值
      */
    extern void set_th_for_alc_auto_switch(void* hd,float pcm_back_on,float pcm_back_off, float pcm_max_on,float pcm_max_off );
    
    
    /**
     * @brief  门限设置
     * @param  hd handle
     * @param  pcm_back_on     alc_on状态下进入alc off的背景能量门限值
     * @param  pcm_back_off    alc_off状态下进入alc on的背景能量门限值
     */
     extern void set_th_for_stationary(void* hd,float pcm_back_on,float pcm_back_off);
     
     
     /**
      * @brief  门限设置
      * @param  hd handle
      * @param  pcm_max_on      alc_on状态下进入alc off的最大能量门限值
      * @param  pcm_max_off     alc_off状态下进入alc on的最大能量门限值
      */
     extern void set_th_for_non_stationary(void* hd,float pcm_max_on,float pcm_max_off);


     /**
      * @brief  alc 开关状态
      * @return int 1表示开启状态 0表示关闭状态
      */
     extern int get_alc_state();
     
     /**
     * @brief  alc 开关状态
     * @return float 返回一个背景能量参考值，仅在开启alc自动切换功能的前提下有效，否则返回0
     */
     extern float get_back_energy_from_alc();
     
     
     /**
     * @brief  alc 禁止模式控制
     * @param  hd handle
     * @param  mode 0为禁止稳态噪声下生效，1为禁止非稳态噪声下生效
     * @return 无 
     */
     extern void ban_alc_function(void* hd,int mode);


#endif //ALC_AUTO_SWITCH_H