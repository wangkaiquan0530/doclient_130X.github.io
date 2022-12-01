/**
 * @file command_info.c
 * @brief 
 * @version 0.1
 * @date 2019-04-30
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */


#include "stdio.h"
#include "command_info.h"
#include "dichotomy_find.h"
#include "command_file_reader.h"
#include "ci_flash_data_info.h"
#include "FreeRTOS.h"
#include "string.h"
#include "asr_api.h"
#include "ci_log.h"


#define cmd_info_malloc(x)          pvPortMalloc(x)
#define cmd_info_free(x)            vPortFree(x)


typedef struct cmd_info_variable_st
{
    uint32_t command_number;
    uint32_t special_word_number;
    uint32_t voice_number;
    command_info_t * p_cmd_table;	
    special_wait_count_t * p_special_word_table;
    command_string_index_t * p_cmd_str_index_table;
    char * p_string_table;
    voice_info_t * p_voice_info_table;
    uint32_t voice_patition_addr;
} cmd_info_variable_t;


cmd_info_variable_t cmd_info_variable = {0};
uint8_t g_is_changing_mode_group = 0;   //当前是否在切换模型中，1：正在切换，0：没有切换或切换完成。

static uint32_t cmd_info_change_cur_model_group_inner(uint8_t model_group_id);

uint32_t cmd_info_init(uint32_t cmd_file_addr_in_flash, uint32_t voice_patition_addr, uint8_t model_group_id)
{
    g_is_changing_mode_group = 1;
    uint32_t ret = cmd_file_reader_init(cmd_file_addr_in_flash);
    if (0 == ret)
    {
        cmd_info_variable.voice_patition_addr = voice_patition_addr;
        #if USE_SEPARATE_WAKEUP_EN      
          
        ret = cmd_info_change_cur_model_group_inner(cmd_file_get_max_model_group_id());
        #else
        ret = cmd_info_change_cur_model_group_inner(DEFAULT_MODEL_GROUP_ID);
        #endif
    }
    g_is_changing_mode_group = 0;
    return ret;
}


static int voice_addr_find_callback(void *pValue, int index, void *CallbackPara)
{
    file_header_t * p_file_header = (file_header_t *)CallbackPara;
    int rst = (int)(*(uint16_t*)pValue) - (int)p_file_header[index].file_id;
    if (rst > 0)
        return 1;
    else if (rst < 0)
        return -1;
    else
        return 0;
}


static uint32_t cmd_info_change_cur_model_group_inner(uint8_t model_group_id)
{
    uint32_t ret = 1;    
    if (0 == cmd_file_change_cur_model_group(model_group_id))
    {
        if (cmd_info_variable.p_cmd_table)
        {
            cmd_info_destroy();
        }

        cmd_info_variable.command_number = cmd_file_get_command_number();
        cmd_info_variable.p_cmd_table = (command_info_t*)cmd_info_malloc(sizeof(command_info_t)*cmd_info_variable.command_number);
        if (!cmd_info_variable.p_cmd_table)
        {
            ci_logerr(LOG_CMD_INFO, "not enough memory\n");
        }
        cmd_file_read_command_table(cmd_info_variable.p_cmd_table);

        cmd_info_variable.p_cmd_str_index_table = (command_string_index_t*)cmd_info_malloc(sizeof(command_string_index_t)*cmd_info_variable.command_number);
        if (!cmd_info_variable.p_cmd_str_index_table)
        {
            ci_logerr(LOG_CMD_INFO, "not enough memory\n");
        }
        cmd_file_read_string_index_table(cmd_info_variable.p_cmd_str_index_table);

        cmd_info_variable.special_word_number = cmd_file_get_special_word_number();
        if (cmd_info_variable.special_word_number)
        {
            cmd_info_variable.p_special_word_table = (special_wait_count_t*)cmd_info_malloc(sizeof(special_wait_count_t)*cmd_info_variable.special_word_number);
            if (!cmd_info_variable.p_special_word_table)
            {
                ci_logerr(LOG_CMD_INFO, "not enough memory\n");
            }
            cmd_file_read_special_word_table(cmd_info_variable.p_special_word_table);
        }

        int size = cmd_file_get_string_table_size();
        cmd_info_variable.p_string_table = (char*)cmd_info_malloc(size);
        if (!cmd_info_variable.p_string_table)
        {
            ci_logerr(LOG_CMD_INFO, "not enough memory\n");
        }
        cmd_file_read_string_table(cmd_info_variable.p_string_table);

        cmd_info_variable.voice_number = cmd_file_get_voice_number();
        cmd_info_variable.p_voice_info_table = (voice_info_t*)cmd_info_malloc(sizeof(voice_info_t)*cmd_info_variable.voice_number);
        if (!cmd_info_variable.p_voice_info_table)
        {
            ci_logerr(LOG_CMD_INFO, "not enough memory\n");
        }
        cmd_file_read_voice_table(cmd_info_variable.p_voice_info_table);

        //初始化voice文件地址
        //Get Voice Address table
        uint32_t voice_group_id;
        cmd_file_get_cur_model_id(NULL, NULL, &voice_group_id);

        uint32_t group_addr = get_group_addr(cmd_info_variable.voice_patition_addr,voice_group_id);     //取得播报音分组的地址
        if (group_addr != (uint32_t)-1)
        {
            file_table_t * p_file_table = get_file_table(group_addr);       //取得播报音文件列表
            if (p_file_table)
            {
                for (int i = 0;i < cmd_info_variable.voice_number;i++)
                {
                    uint16_t voice_id = cmd_info_variable.p_voice_info_table[i].voice_id;
                    int rst = DichotomyFind((void*)&voice_id, 
                                            0, 
                                            p_file_table->file_number - 1,
                                            voice_addr_find_callback, &p_file_table->file_header[0]);
                    if (rst >= 0)
                    {
                        cmd_info_variable.p_voice_info_table[i].voice_data_addr = 
                            p_file_table->file_header[rst].file_addr + cmd_info_variable.voice_patition_addr;
                    }
                }
                release_file_table(p_file_table);
                ret = 0;
                ci_loginfo(LOG_CMD_INFO, "change to model group %d\n", model_group_id);
            }
            else
            {
                ci_loginfo(LOG_CMD_INFO, "not enough memory\n");
            }
        }
    }
    return ret;
}


uint32_t cmd_info_change_cur_model_group(uint8_t model_group_id)
{
#if USE_NEW_CHANGE_MODE
    int ret = 0;
    static unsigned char record_mode_id = DEFAULT_MODEL_GROUP_ID;
    
    if(model_group_id == record_mode_id)
    {
        mprintf("no need change asr mode\n");
        return ret;
    }
    ret = asrtop_asr_switch_fst(model_group_id,NULL);
    if(ret == -1)
    {
        mprintf("change asr mode err\n");
    }
    else
    {
        record_mode_id = model_group_id;
        mprintf("change asr mode %d\n",model_group_id);
    }
    return ret;
#else
    static unsigned char last_mode_group_id = DEFAULT_MODEL_GROUP_ID;
    uint32_t ret = 1;
    if (last_mode_group_id != model_group_id)
    {   
        last_mode_group_id = model_group_id;
        g_is_changing_mode_group = 1;

		extern int set_change_model_state(void);
		set_change_model_state();
		
        cmd_info_change_cur_model_group_inner(model_group_id);

        uint32_t lg_model_addr;
        uint32_t lg_model_size;
        uint32_t ac_model_addr;
        uint32_t ac_model_size;
        get_current_model_addr(&ac_model_addr, &ac_model_size, &lg_model_addr, &lg_model_size);

        asrtop_asr_system_create_model(0x50000000+lg_model_addr, lg_model_size, 0x50000000+ac_model_addr, ac_model_size,0);
        g_is_changing_mode_group = 0;
        ret = 0;
    }
    else
    {
        ret = 0;
    }
    return ret;
#endif
}


uint32_t cmd_info_get_cur_model_id(uint32_t *p_dnn_id, uint32_t *p_asr_id, uint32_t *p_voice_group_id)
{
    return cmd_file_get_cur_model_id(p_dnn_id, p_asr_id, p_voice_group_id);
}

void cmd_info_destroy()
{
    if (cmd_info_variable.p_cmd_table)
    {
        cmd_info_free(cmd_info_variable.p_cmd_table);
        cmd_info_variable.p_cmd_table = NULL;
    }

    if (cmd_info_variable.p_special_word_table)
    {
        cmd_info_free(cmd_info_variable.p_special_word_table);
        cmd_info_variable.p_special_word_table = NULL;
    }

    if (cmd_info_variable.p_cmd_str_index_table)
    {
        cmd_info_free(cmd_info_variable.p_cmd_str_index_table);
        cmd_info_variable.p_cmd_str_index_table = NULL;
    }

    if (cmd_info_variable.p_string_table)
    {
        cmd_info_free(cmd_info_variable.p_string_table);
        cmd_info_variable.p_string_table = NULL;
    }

    if (cmd_info_variable.p_voice_info_table)
    {
        cmd_info_free(cmd_info_variable.p_voice_info_table);
        cmd_info_variable.p_voice_info_table = NULL;
    }
}


static int cmd_string_find_callback(void *pValue, int index, void *CallbackPara)
{
    cmd_info_variable_t * p_cmd_info_variable = (cmd_info_variable_t *)CallbackPara;
    char *p_str = p_cmd_info_variable->p_string_table + p_cmd_info_variable->p_cmd_str_index_table[index].command_string_offset;
    int rst = strcmp(pValue, p_str);
    if (rst > 0)
        return 1;
    else if (rst < 0)
        return -1;
    else
        return 0;
}


static int cmd_id_find_callback(void *pValue, int index, void *CallbackPara)
{
    cmd_info_variable_t * p_cmd_info_variable = (cmd_info_variable_t *)CallbackPara;
    uint16_t t_id = p_cmd_info_variable->p_cmd_table[index].command_id;
    int rst = (int)pValue - (int)t_id;
    if (rst > 0)
        return 1;
    else if (rst < 0)
        return -1;
    else
        return 0;
}


cmd_handle_t cmd_info_find_command_by_id(uint16_t cmd_id)
{
    if (!g_is_changing_mode_group)
    {
        int rst = DichotomyFind((void*)(int)cmd_id, 0, cmd_info_variable.command_number - 1, cmd_id_find_callback, &cmd_info_variable);
        if (rst >= 0)
        {
            return (cmd_handle_t)&(cmd_info_variable.p_cmd_table[rst]);
        }
    }
    return INVALID_HANDLE;
}


cmd_handle_t cmd_info_find_command_by_string(const char * cmd_string)
{
    if (!g_is_changing_mode_group)
    {
        int rst = DichotomyFind((void*)cmd_string, 0, cmd_info_variable.command_number - 1, cmd_string_find_callback, &cmd_info_variable);
        if (rst >= 0)
        {
            uint16_t cmd_id = cmd_info_variable.p_cmd_str_index_table[rst].command_id;
            return cmd_info_find_command_by_id(cmd_id);
        }
    }
    return INVALID_HANDLE;
}


cmd_handle_t cmd_info_find_command_by_semantic_id(uint32_t semantic_id)
{
    if (!g_is_changing_mode_group)
    {
        for (int i = 0;i < cmd_info_variable.command_number;i++)
        {
            if (cmd_info_variable.p_cmd_table[i].semantic_id == semantic_id)
            {
                return (cmd_handle_t)&cmd_info_variable.p_cmd_table[i];
            }
        }
    }
    return INVALID_HANDLE;
}


uint16_t cmd_info_get_command_id(cmd_handle_t cmd_handle)
{
    if (!g_is_changing_mode_group)
    {
        return (cmd_handle != INVALID_HANDLE) ? ((command_info_t*)cmd_handle)->command_id : INVALID_SHORT_ID;
    }
    else
    {
        return INVALID_SHORT_ID;
    }
}

char * cmd_info_get_command_string(cmd_handle_t cmd_handle) 
{
    if (!g_is_changing_mode_group)
    {
        uint16_t cmd_id = cmd_info_get_command_id(cmd_handle);
        if (cmd_id != INVALID_SHORT_ID)
        {
            for (int i = 0;i < cmd_info_variable.command_number;i++)
            {
                if (cmd_info_variable.p_cmd_str_index_table[i].command_id == cmd_id)
                {
                    return cmd_info_variable.p_string_table + cmd_info_variable.p_cmd_str_index_table[i].command_string_offset;
                }
            }
        }
    }
    return NULL;
}

uint8_t cmd_info_get_cmd_score(cmd_handle_t cmd_handle)
{
    if (!g_is_changing_mode_group)
    {
        return (cmd_handle != INVALID_HANDLE) ? ((command_info_t*)cmd_handle)->score : 0;
    }
    else
    {
        return 0xFF;
    }
}


uint32_t cmd_info_get_semantic_id(cmd_handle_t cmd_handle)
{
    if (!g_is_changing_mode_group)
    {
        return (cmd_handle != INVALID_HANDLE) ? ((command_info_t*)cmd_handle)->semantic_id : INVALID_LONG_ID;
    }    
    else
    {
        return INVALID_LONG_ID;
    }
    
}


uint8_t cmd_info_get_cmd_flag(cmd_handle_t cmd_handle)
{
    if (!g_is_changing_mode_group)
    {
        return (cmd_handle != INVALID_HANDLE) ? ((command_info_t*)cmd_handle)->flags : 0;
    }
    else
    {
        return 0;
    }
    
}


uint32_t cmd_info_is_special_word(cmd_handle_t cmd_handle)
{
    if((cmd_handle != INVALID_HANDLE) && (!g_is_changing_mode_group))
    {
        if (((command_info_t*)cmd_handle)->flags & CMD_FLAG_SPECIAL_WORD)
        {
            return 1;
        }
    }
    return 0;
}

uint32_t cmd_info_is_wakeup_word(cmd_handle_t cmd_handle)
{
    if((cmd_handle != INVALID_HANDLE) && (!g_is_changing_mode_group))
    {
        if (((command_info_t*)cmd_handle)->flags & CMD_FLAG_WAKEUP_WORD)
        {
            return 1;
        }
    }
    return 0;
}

uint32_t cmd_info_is_combo_word(cmd_handle_t cmd_handle)
{
    if ((cmd_handle != INVALID_HANDLE) && (!g_is_changing_mode_group))
    {
        if (((command_info_t*)cmd_handle)->flags & CMD_FLAG_COMBO_WORD)
        {
            return 1;
        }
    }
    return 0;
}

uint32_t cmd_info_is_expected_word(cmd_handle_t cmd_handle)
{
    if ((cmd_handle != INVALID_HANDLE) && (!g_is_changing_mode_group))
    {
        if (((command_info_t*)cmd_handle)->flags & CMD_FLAG_EXPECTED_WORD)
        {
            return 1;
        }
    }
    return 0;
}

uint32_t cmd_info_is_unexpected_word(cmd_handle_t cmd_handle)
{
    if ((cmd_handle != INVALID_HANDLE) && (!g_is_changing_mode_group))
    {
        if (((command_info_t*)cmd_handle)->flags & CMD_FLAG_UNEXPECTED_WORD)
        {
            return 1;
        }
    }
    return 0;
}


static int wait_count_find_callback(void *pValue, int index, void *CallbackPara)
{
    cmd_info_variable_t * p_cmd_info_variable = (cmd_info_variable_t *)CallbackPara;
    uint16_t t_id = p_cmd_info_variable->p_special_word_table[index].command_id;
    int rst = (int)pValue - (int)t_id;
    if (rst > 0)
        return 1;
    else if (rst < 0)
        return -1;
    else
        return 0;
}

int32_t cmd_info_get_special_wait_count(cmd_handle_t cmd_handle)
{
    if (!g_is_changing_mode_group)
    {
        uint32_t cmd_id = ((command_info_t*)cmd_handle)->command_id;
        int rst = DichotomyFind((void*)cmd_id, 0, cmd_info_variable.special_word_number-1, wait_count_find_callback, & cmd_info_variable);
        if (rst >= 0)
        {
            return cmd_info_variable.p_special_word_table[rst].wait_count;
        }
    }
    return 0;
}


uint8_t get_combination_voice_number(uint16_t start_index)
{
    if (!g_is_changing_mode_group)
    {
        int number = 1;
        uint16_t group = cmd_info_variable.p_voice_info_table[start_index].voice_group & 0x7FFF;
        for (uint16_t i = start_index + 1; i < cmd_info_variable.voice_number; i++)
        {
            if ((cmd_info_variable.p_voice_info_table[i].voice_group & 0x7FFF)== group && \
                !(cmd_info_variable.p_voice_info_table[i].voice_group & 0x8000))
            {
                number++;
            }
            else
            {
                break;
            }
        }
        return number;
    }
    return 0;
}


uint32_t get_voice_addr_by_index(uint16_t voice_index)
{
    if ((voice_index < cmd_info_variable.voice_number) && (!g_is_changing_mode_group))
    {
        return cmd_info_variable.p_voice_info_table[voice_index].voice_data_addr;
    }
    return INVALID_ADDR;
}

uint8_t get_voice_option_number(cmd_handle_t cmd_handle)
{
    uint16_t voice_index = ((command_info_t*)cmd_handle)->voice_index;
    uint16_t group = cmd_info_variable.p_voice_info_table[voice_index].voice_group;
    return (group >> 7) & 0x007F;  
}

voice_select_type_t get_voice_select_type(cmd_handle_t cmd_handle)
{
    uint16_t voice_index = ((command_info_t*)cmd_handle)->voice_index;
    uint16_t group = cmd_info_variable.p_voice_info_table[voice_index].voice_group;
    return (group & 0x4000) ? VOICE_SELECT_RANDOM : VOICE_SELECT_USER;
}

uint32_t is_changing_model()
{
    return g_is_changing_mode_group;
}




