/**
  ******************************************************************************
  * @file    audio_in_manage.h
  * @version V1.0.0
  * @date    2019.04.04
  * @brief 
  ******************************************************************************
  */

#ifndef _VOICE_IN_MANAGE_H_
#define _VOICE_IN_MANAGE_H_


#ifdef __cplusplus
extern "C" {
#endif

int32_t audio_in_buffer_init(void);

int32_t audio_in_hw_straight_mode_init(void);

void audio_in_preprocess_mode_task(void* p);


#ifdef __cplusplus
}
#endif

#endif
