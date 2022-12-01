/**
  ******************************************************************************
  * @file    alg_preprocess.h
  * @version V1.0.0
  * @date    2019.04.04
  * @brief 
  ******************************************************************************
  */


#ifndef ALG_PREPROCESS_H
#define ALG_PREPROCESS_H

#ifdef __cplusplus
extern "C" {
#endif


    void alg_init( void );
    int  alg_preprocess_single_ch( short *mic_data, short *dst_data, int frame_length );
    int  alg_preprocess_two_ch( short *mic_data, short *dst_data, int frame_length );
    int  alg_preprocess_four_ch( short *mic_data, short *ref_data, short *dst_data, int frame_length );
    int  alg_preprocess_four_ch_single_aec( short *mic_data, short *ref_data, short *dst_data, int frame_length );


    /**
     * @brief  获取当前角度信息（只有开启了TDOA算法模块才有意义）,
     *         在被clear_current_tdoa_angle（）清除角度之前，一直保持角度信息；
     *         默认情况下，开启TDOA算法模块时，当说唤醒词后更新角度信息，
     *         如果宏TDOA_WHEN_LOCAL_COMMON_CMD开启，当说本地命令词后也会更新角度信息；
     * 
     * @return int, 角度信息，正常的数值范围为 0~180， 若返回其它值则表示错误;
     */    
    int get_current_tdoa_angle( void );


    /**
     * @brief  清除当前角度信息（只有开启了TDOA算法模块才有意义）
     * 
     * @return int, 若返回0，表示成功; 若返回其它值则表示错误;
     */    
    int clear_current_tdoa_angle( void );


#ifdef __cplusplus
}
#endif


#endif /* ALG_PREPROCESS_H */

