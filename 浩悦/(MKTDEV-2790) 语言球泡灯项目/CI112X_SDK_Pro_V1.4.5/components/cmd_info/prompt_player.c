/**
 * @file prompt_player.c
 * @brief 
 * @version 0.1
 * @date 2019-04-30
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */

#include <stdio.h>
#include "command_file_reader.h"
#include "command_info.h"
#include "audio_play_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ci_log.h"
#include "prompt_player.h"
#include "ci112x_audio_play_card.h"
#include "audio_play_device.h"

#include "asr_api.h"
#include "audio_in_manage.h"
#include "ci112x_audio_capture.h"

typedef 
struct voice_play_info_st
{
    cmd_handle_t cmd_handle;
    int select_index;                           //!选择播报索引
    play_done_callback_t play_done_callback;    //!播放结束的回调函数
}voice_play_info_t;

typedef 
struct prompt_player_st
{
    FunctionalState enabled_flag;               //!提示音播报是否使能
    QueueHandle_t play_queue;                   //!播放队列
    SemaphoreHandle_t semaphore;    
    bool mute_voice_in_flag;                    //!语音输入mute标志
    uint8_t combination_number;
    uint8_t combination_index;
    uint8_t pause_asr_count;
    
    uint32_t combination_list[MAX_COMBINATION_COUNT];
}prompt_player_t;


static prompt_player_t prompt_player = 
{
    ENABLE,     //enabled_flag
    0,          //play_queue
    NULL,       //semaphore
    false,      //mute_voice_in_flag
    0,          //combination_number
    0,          //combination_index    
    0,          //pause_asr_count
};

static void resume_asr(void);

/**
 * @brief Get the mute voice in state object, DENOISE will used this function
 *
 * @return true mute voice in, data is 0
 * @return false voice in is normal
 */
bool get_mute_voice_in_state( void )
{
    return prompt_player.mute_voice_in_flag;
}

static void combination_callback(int32_t play_cb_state);

static void pop_from_play_queue()
{
    voice_play_info_t voice_play_info;
    BaseType_t rst = xQueueReceive(prompt_player.play_queue, &voice_play_info, 0);
    if (pdTRUE == rst)
    {
        resume_asr();
        if (voice_play_info.play_done_callback)
        {
            voice_play_info.play_done_callback(voice_play_info.cmd_handle);
        }
    }
}

static void clean_play_queue()
{
    voice_play_info_t voice_play_info;
    while (pdTRUE == xQueueReceive(prompt_player.play_queue, &voice_play_info, 0))
    {
        resume_asr();
        if (voice_play_info.play_done_callback)
        {
            voice_play_info.play_done_callback(voice_play_info.cmd_handle);
        }
    }
    prompt_player.combination_number = 0;
    prompt_player.combination_index = 0;
}


static int prompt_play_inner(voice_play_info_t *p_voice_play_info, bool from_callback)
{
    uint16_t voice_index = ((command_info_t*)p_voice_play_info->cmd_handle)->voice_index;
    uint8_t option_number = get_voice_option_number(p_voice_play_info->cmd_handle);   
    uint8_t select_index = 0;
    
    if (option_number <= 0)
    {
        pop_from_play_queue();
        return 1;
    }
    else if (option_number > 1)
    {
        voice_select_type_t select_type = get_voice_select_type(p_voice_play_info->cmd_handle);        
        if (p_voice_play_info->select_index >= 0 && p_voice_play_info->select_index < option_number)
        {
            select_index = p_voice_play_info->select_index;
        }
        else if (select_type == VOICE_SELECT_RANDOM)
        {
            select_index = rand() % option_number;
        }
        else
        {
            select_index = 0;
        }
    }
    
    uint8_t combination_number;
    for (int i = 0;i < select_index;i++)
    {
        combination_number = get_combination_voice_number(voice_index);
        voice_index += combination_number;
    }
    combination_number = get_combination_voice_number(voice_index);
    if (combination_number > 0 )
    {
        if (combination_number <= MAX_COMBINATION_COUNT)
        {
            for (int i = 0;i < combination_number;i++)
            {
                prompt_player.combination_list[i] = get_voice_addr_by_index(voice_index+i);
            }
            prompt_player.combination_number = combination_number;
            prompt_player.combination_index = 0;

            /*audio PA on*/
            #if (PLAYER_CONTROL_PA)
            //audio_play_hw_start(ENABLE);
            audio_play_hw_pa_da_ctl(ENABLE);
            vTaskDelay(pdMS_TO_TICKS(100));
            #else
            audio_play_hw_pa_da_ctl(ENABLE);
            #endif
#if 1            
            pause_audio_play_prompt(prompt_player.combination_list[prompt_player.combination_index++], 1, combination_callback);
#else
            if (combination_number > 1)
            {
                pause_audio_play_prompt((uint32_t)prompt_player.combination_list, combination_number, combination_callback);
            }
            else
            {
                combination_number = 1;
                pause_audio_play_prompt(prompt_player.combination_list[prompt_player.combination_index++], 1, combination_callback);
            }
#endif
        }
        else
        {
            ci_logerr(CI_LOG_ERROR,"too many combination voice\n");
            clean_play_queue();
        }
    }
    else
    {
        pop_from_play_queue();
        return 2;
    }
    return 0;
}

void pause_asr(void)
{
    //xSemaphoreTake(muteMutex, portMAX_DELAY);
    if (prompt_player.pause_asr_count == 0)
    {
        #if PAUSE_VOICE_IN_WITH_PLAYING
        prompt_player.mute_voice_in_flag = true;
            #if !USE_OUTSIDE_IIS_TO_ASR
            audio_cap_mute(AUDIO_CAP_INNER_CODEC,ENABLE);/*mute: but audio continue in*/
			asrtop_asr_system_pause();
            #else
            audio_cap_mute(AUDIO_CAP_OUTSIDE_CODEC,ENABLE);/*mute: but audio continue in*/
			asrtop_asr_system_pause();
            #endif
        #endif

    }
    prompt_player.pause_asr_count++;
    //xSemaphoreGive(muteMutex);
}


/**
 * @brief resume voice in, so can recover ASR, call this after play done,
 *
 */
void  resume_asr(void)
{
    //xSemaphoreTake(muteMutex, portMAX_DELAY);    
    if (prompt_player.pause_asr_count > 0)
    {
        prompt_player.pause_asr_count--;
        if (prompt_player.pause_asr_count == 0)
        {
            #if PAUSE_VOICE_IN_WITH_PLAYING
                #if !USE_OUTSIDE_IIS_TO_ASR
                audio_cap_mute(AUDIO_CAP_INNER_CODEC,DISABLE);
		    	asrtop_asr_system_continue();
                #else
                audio_cap_mute(AUDIO_CAP_OUTSIDE_CODEC,DISABLE);
		    	asrtop_asr_system_continue();
                #endif
                prompt_player.mute_voice_in_flag = false;
            #endif
        }
    }
    //xSemaphoreGive(muteMutex);
}

static void combination_callback(int32_t play_cb_state)
{
    voice_play_info_t voice_play_info;
    
    if (prompt_player.semaphore)
    {
        xSemaphoreTake(prompt_player.semaphore, portMAX_DELAY);
    }

    if (AUDIO_PLAY_CB_STATE_PLAY_THRESHOLD == play_cb_state &&
        (prompt_player.combination_index >= prompt_player.combination_number))
    {
        #if OPEN_ASR_IN_ADVANCE
        resume_asr();
        #endif
        if (prompt_player.semaphore)
        {
            xSemaphoreGive(prompt_player.semaphore);
        }
        return;
    }

    if ((prompt_player.combination_index >= prompt_player.combination_number) 
            || (AUDIO_PLAY_CB_STATE_PAUSE_BEFORE_THRESHOLD == play_cb_state)
            || (AUDIO_PLAY_CB_STATE_PAUSE_AFTER_THRESHOLD == play_cb_state)
            || (play_cb_state < 0))
    {
        //调用上一个播报音的结束回调
        BaseType_t rst = xQueueReceive(prompt_player.play_queue, &voice_play_info, 0);
        if (pdTRUE == rst)
        {
            if ((AUDIO_PLAY_CB_STATE_PAUSE_BEFORE_THRESHOLD == play_cb_state) 
                || (play_cb_state < 0)
                || ((prompt_player.combination_index < prompt_player.combination_number) && (AUDIO_PLAY_CB_STATE_PAUSE_AFTER_THRESHOLD == play_cb_state)))
            {
                #if OPEN_ASR_IN_ADVANCE
                resume_asr();
                #endif
            }
            if (voice_play_info.play_done_callback)     
            {
                #if !OPEN_ASR_IN_ADVANCE
                resume_asr();
                #endif
                voice_play_info.play_done_callback(voice_play_info.cmd_handle);
            }
        }
        
        //播放下一个顺序播报音
        rst = xQueuePeek(prompt_player.play_queue, &voice_play_info, 0);
        if (pdTRUE == rst && (play_cb_state == AUDIO_PLAY_CB_STATE_DONE))
        {
            if (prompt_play_inner(&voice_play_info, true) != 0)
            {
                clean_play_queue();
            }
        }        
        else
        {
            clean_play_queue();

            #if (PLAYER_CONTROL_PA)
            /*audio PA on*/
            //audio_play_hw_stop(ENABLE);
            //gpio_set_output_low_level(AUDIO_PLAY_CARD_DEFAULT_GPIO_BASE,AUDIO_PLAY_CARD_DEFAULT_GPIO_PIN);
            audio_play_hw_pa_da_ctl(DISABLE);
            #else
            audio_play_hw_pa_da_ctl(DISABLE);
            #endif
        }
    }
    else if (prompt_player.combination_index < prompt_player.combination_number && play_cb_state == AUDIO_PLAY_CB_STATE_DONE)
    {
        //播放下一个组合播报音
        pause_audio_play_prompt(prompt_player.combination_list[prompt_player.combination_index++], 1, combination_callback);
    }

    if (prompt_player.semaphore)
    {
        xSemaphoreGive(prompt_player.semaphore);
    }
}


static uint32_t voice_play_info_add_to_queue(voice_play_info_t *p_voice_play_info)
{
    if (errQUEUE_FULL == xQueueSend(prompt_player.play_queue,p_voice_play_info, 0))
    {
        ci_logerr(LOG_CMD_INFO, "prompt queue full\n");
        return 1;
    }
    return 0;
}

void prompt_player_enable(FunctionalState state)
{
#if PROMPT_CLOSE_EN
    prompt_player.enabled_flag = state;
#endif
}


uint32_t prompt_play_by_cmd_handle(
            cmd_handle_t cmd_handle, 
            int select_index, 
            play_done_callback_t play_done_callback,
            bool preemptive)
{
    #if !AUDIO_PLAYER_ENABLE
        if(play_done_callback)
        {
           play_done_callback(cmd_handle);
        }
        return 0;
    #endif
        
    uint32_t ret = 1;

    if (prompt_player.semaphore == NULL)
    {
        prompt_player.semaphore = xSemaphoreCreateMutex();
    }
    if (prompt_player.semaphore)
    {
        xSemaphoreTake(prompt_player.semaphore, portMAX_DELAY);
    }

    if (!prompt_player.play_queue)
    {
        prompt_player.play_queue = xQueueCreate(5, sizeof(voice_play_info_t));
        if (!prompt_player.play_queue)
        {
            ci_logerr(LOG_CMD_INFO, "not enough memory\n");
        }
    }

    if (!prompt_player.enabled_flag)
    {
        cmd_handle = cmd_info_find_command_by_string("<beep>");
    }

    if (cmd_handle == INVALID_HANDLE || !prompt_player.play_queue)
    {
        if (play_done_callback)
        {
            play_done_callback(cmd_handle);
        }
        if (prompt_player.semaphore)
        {
            xSemaphoreGive(prompt_player.semaphore);
        }
        return ret;
    }

    voice_play_info_t voice_play_info;
    if (prompt_player.combination_number > 0 && preemptive)     //当前是否正在播报,需要打断
    {
        if (RETURN_ERR == pause_play(NULL,NULL))
        {
            vTaskDelay(1);
            pause_play(NULL,NULL);
        }
        int timeout = 2000;        //2秒
        if (prompt_player.semaphore)
        {
            xSemaphoreGive(prompt_player.semaphore);
        }
        while(prompt_player.combination_number > 0 && timeout > 0)
        {
            timeout--;
            vTaskDelay(1);
        }
        if (prompt_player.semaphore)
        {
            xSemaphoreTake(prompt_player.semaphore, portMAX_DELAY);
        }  
    }

    voice_play_info.cmd_handle = cmd_handle;
    voice_play_info.select_index = select_index;
    voice_play_info.play_done_callback = play_done_callback;

    ret = voice_play_info_add_to_queue(&voice_play_info);
    if (ret != 0)
    {
        if (voice_play_info.play_done_callback)
            voice_play_info.play_done_callback(voice_play_info.cmd_handle);
    }
    else
    {
        pause_asr();
        if (preemptive || prompt_player.combination_number <= 0)
        {
            prompt_play_inner(&voice_play_info, false);
        }
        ret = 0;
    }
    
    if (prompt_player.semaphore)
    {
        xSemaphoreGive(prompt_player.semaphore);
    }
    return ret;
}

uint32_t prompt_play_by_cmd_id(
            uint16_t cmd_id, 
            int select_index, 
            play_done_callback_t play_done_callback,
            bool preemptive)
{
    cmd_handle_t cmd_handle = cmd_info_find_command_by_id(cmd_id);
    return prompt_play_by_cmd_handle(cmd_handle, select_index, play_done_callback, preemptive);
}

uint32_t prompt_play_by_semantic_id(
            uint32_t semantic_id, 
            int select_index, 
            play_done_callback_t play_done_callback,
            bool preemptive)
{
    cmd_handle_t cmd_handle = cmd_info_find_command_by_semantic_id(semantic_id);
    return prompt_play_by_cmd_handle(cmd_handle, select_index, play_done_callback, preemptive);
}

uint32_t prompt_play_by_cmd_string(
            char* cmd_string, 
            int select_index, 
            play_done_callback_t play_done_callback,
            bool preemptive)
{
    cmd_handle_t cmd_handle = cmd_info_find_command_by_string(cmd_string);
    return prompt_play_by_cmd_handle(cmd_handle, select_index, play_done_callback, preemptive);
}


typedef struct multi_id_info_st
{
    uint32_t id;
    uint16_t select_index;
}multi_id_info_t;

typedef enum id_type_st
{
    ID_TYPE_CMD,
    ID_TYPE_SEMANTIC,
}id_type_t;

uint32_t prompt_play_by_multi_id(multi_id_info_t *p_play_info, int number, id_type_t id_type, play_done_callback_t play_done_callback)
{
    uint32_t ret = 1;
    if (prompt_player.semaphore == NULL)
    {
        prompt_player.semaphore = xSemaphoreCreateMutex();
    }
    if (prompt_player.semaphore)
    {
        xSemaphoreTake(prompt_player.semaphore, portMAX_DELAY);
    }
    if (!prompt_player.play_queue)
    {
        prompt_player.play_queue = xQueueCreate(5, sizeof(voice_play_info_t));
    }

    if ((!prompt_player.play_queue) || (!prompt_player.enabled_flag) || (number > MAX_COMBINATION_COUNT))
    {
        if (play_done_callback)
        {
            play_done_callback(0);
        }
        if (prompt_player.semaphore)
        {
            xSemaphoreGive(prompt_player.semaphore);
        }
        return ret;
    }

    voice_play_info_t voice_play_info;
    if (prompt_player.combination_number > 0)     //当前是否正在播报,需要打断
    {
        if (RETURN_ERR == pause_play(NULL,NULL))
        {
            vTaskDelay(1);
            pause_play(NULL,NULL);
        }
        if (prompt_player.semaphore)
        {
            xSemaphoreGive(prompt_player.semaphore);
        }

        int timeout = 2000;        //2秒
        while(prompt_player.combination_number > 0 && timeout > 0)
        {
            timeout--;
            vTaskDelay(1);
        }
        if (prompt_player.semaphore)
        {
            xSemaphoreTake(prompt_player.semaphore, portMAX_DELAY);
        }
    }
    
    voice_play_info.cmd_handle = 0;
    voice_play_info.select_index = 0;
    voice_play_info.play_done_callback = play_done_callback;    
    ret = voice_play_info_add_to_queue(&voice_play_info);
    prompt_player.combination_number = 0;
    for (int i = 0;i < number;i++)
    {
    	cmd_handle_t cmd_handle;
	    if (id_type == ID_TYPE_CMD)
	    {
            cmd_handle = cmd_info_find_command_by_id(p_play_info[i].id);
	    }
	    else if (id_type == ID_TYPE_SEMANTIC)
	    {
            cmd_handle = cmd_info_find_command_by_semantic_id(p_play_info[i].id);
    	}
        uint16_t voice_index = ((command_info_t*)cmd_handle)->voice_index;
        uint8_t option_number = get_voice_option_number(cmd_handle);
        uint8_t select_index = p_play_info[i].select_index;
        uint8_t combination_number;
        if ((option_number > 1) && (select_index > 0) && (select_index < option_number))
        {
            for (int j = 0;j < select_index;j++)
            {
                combination_number = get_combination_voice_number(voice_index);
                voice_index += combination_number;
            }
        }
        combination_number = get_combination_voice_number(voice_index);
        for (int j = 0;j < combination_number;j++)
        {
            prompt_player.combination_list[prompt_player.combination_number++] = get_voice_addr_by_index(voice_index+j);
        }
    }
    prompt_player.combination_index = 0;

    /*audio PA on*/
    #if (PLAYER_CONTROL_PA)
    //audio_play_hw_start(ENABLE);
    audio_play_hw_pa_da_ctl(ENABLE);
    vTaskDelay(pdMS_TO_TICKS(100));
    #endif
    pause_asr();
    pause_audio_play_prompt(prompt_player.combination_list[prompt_player.combination_index++], 1, combination_callback);

    if (prompt_player.semaphore)
    {
        xSemaphoreGive(prompt_player.semaphore);
    }
    ret = 0;
    return ret;
}

uint32_t prompt_play_by_multi_cmd_id(prompt_play_info_t *p_play_info, int number, play_done_callback_t play_done_callback)
{
    return prompt_play_by_multi_id((multi_id_info_t *)p_play_info, number, ID_TYPE_CMD, play_done_callback);
}

/*uint32_t prompt_play_by_multi_semantic_id(prompt_play_info2_t *p_play_info, int number, play_done_callback_t play_done_callback)
{
    return prompt_play_by_multi_id((multi_id_info_t *)p_play_info, number, ID_TYPE_SEMANTIC, play_done_callback);
}*/

uint32_t prompt_is_playing()
{
    if (prompt_player.combination_number > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint32_t prompt_stop_play()
{
    voice_play_info_t voice_play_info;
    stop_play(NULL,NULL);
    int timeout = 2000;
    while(prompt_player.combination_number > 0 && timeout > 0)
    {
        timeout--;
        vTaskDelay(1);
    }
    return 0;
}
