/**
 * @file ci_flash_data_info.c
 * @brief flash data struct
 * @version 
 * @date 
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */


#include <stdint.h>
#include <ci112x_system.h>
#include <string.h>
#include <stdlib.h>
#include "ci_flash_data_info.h"
#include "ci_log.h"
#include "dichotomy_find.h"
#include "flash_rw_process.h"
#include "ci_nvdata_manage.h"
#include "ci112x_spiflash.h"
#include "command_file_reader.h"

#include "sdk_default_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "command_info.h"
#include "firmware_updater.h"


static partition_table_t partition_table = {0};

/**
 * @brief Get the fw version object
 * 
 * @param product_version  包含硬件版本号， 软件版本号
 * @return uint32_t RETURN_ERR 获取失败， RETURN_OK 获取成功
 */
int32_t get_fw_version(product_version_t *product_version)
{
    if(product_version == NULL)
    {
        return RETURN_ERR;
    }
    product_version->hard_ware_version = partition_table.hard_ware_version;
    product_version->soft_ware_version = partition_table.soft_ware_version;
    return RETURN_OK;
}

uint32_t get_group_addr(uint32_t partition_addr, uint16_t group_id)
{
    uint32_t ret = (uint32_t)-1;
    uint16_t group_number;

    //读取分组数量
    post_read_flash((char *)&group_number,partition_addr,sizeof(group_number));

    uint32_t table_size = 2 + sizeof(group_header_t)*group_number;
    group_table_t *p_group_table = pvPortMalloc(table_size);
    if (p_group_table)
    {
        //读取分组列表
        post_read_flash((char *)p_group_table,partition_addr,table_size);

        for(int i = 0;i < group_number;i++)
        {
            if (p_group_table->group_header[i].group_id == group_id)
            {
                ret = p_group_table->group_header[i].group_addr;
                ret += partition_addr;
            }
        }
        vPortFree(p_group_table);
    }
    else
    {
        ci_logerr(LOG_FLASH_CTL, "not enough memory\n");
    }
    
    return ret;
}


file_table_t * get_file_table(uint32_t group_addr)
{
    uint16_t file_number;

    //读取指定分组中文件的数量
    post_read_flash((char *)&file_number,group_addr,sizeof(file_number));

    uint32_t table_size = 2 + sizeof(file_header_t)*file_number;
    file_table_t *p_file_table = pvPortMalloc(table_size);
    if (p_file_table)
    {
        //读取分组列表
        post_read_flash((char *)p_file_table,group_addr,table_size);
        return p_file_table;
    }    
    else
    {
        ci_logerr(LOG_FLASH_CTL, "not enough memory\n");
    }

    return NULL;
}

void release_file_table(file_table_t * p_file_table)
{
    vPortFree(p_file_table);
}

uint32_t get_file_addr(uint32_t group_addr, uint16_t file_id, uint32_t *p_file_addr, uint32_t *p_file_size)
{
    uint32_t ret = 0;
    uint16_t file_number;

    //读取指定分组中文件的数量
    post_read_flash((char *)&file_number,group_addr,sizeof(file_number));

    uint32_t table_size = 2 + sizeof(file_header_t)*file_number;
    file_table_t *p_file_table = get_file_table(group_addr);
    if (p_file_table)
    {
        //读取分组列表
        post_read_flash((char *)p_file_table,group_addr,table_size);

        for(int i = 0;i < file_number;i++)
        {
            if (p_file_table->file_header[i].file_id == file_id)
            {
                if (p_file_addr)
                {
                    *p_file_addr = p_file_table->file_header[i].file_addr;
                }
                if (p_file_size)
                {
                    *p_file_size = p_file_table->file_header[i].file_size;
                }
                ret = 1;
            }
        }
        vPortFree(p_file_table);
    }

    return ret;
}


#if (COPYRIGHT_VERIFICATION && (ENCRYPT_ALGORITHM == ENCRYPT_USER_DEFINE))
static int user_encrypt(char * in_buffer, int in_len, char *out_buffer, int out_len)
{
    //以下为实例代码，加密强度很低，用户可以根据自己的加密算法修改
    for (int i = 0;i < in_len;i++)
    {
        out_buffer[i] = in_buffer[i] ^ 0x5A;
    }
}
#endif


uint16_t get_partition_list_checksum(partition_table_t *file_config)
{
    int len = sizeof(partition_table_t) - 2;
    uint16_t sum = 0;

    file_config->user_code1_status = 0xF0;
    file_config->user_code2_status = 0xFF;
    file_config->asr_cmd_model_status = 0;
    file_config->dnn_model_status = 0;
    file_config->voice_status = 0;
    file_config->user_file_status = 0;
    
    for(int i = 0; i< len; i++)
    {
        sum += ((uint8_t*)file_config)[i];
    }

    return sum;
}


uint32_t ci_flash_data_info_init(uint8_t default_model_group_id)
{
    #if COPYRIGHT_VERIFICATION
    {
        #if (ENCRYPT_ALGORITHM == ENCRYPT_DEFAULT)
        char *password = "mynameispassword";
        if (!copyright_verification2(password, strlen(password)))
        {
            //请修改以下校验失败处理，以下方式很容易被破解，仅供参考
            while(1)
            {
                mprintf("encrypt check failed\n");
            }
        }
        #elif (ENCRYPT_ALGORITHM == ENCRYPT_USER_DEFINE)
        if (!copyright_verification1(user_encrypt, 16))
        {
            //请修改以下校验失败处理，以下方式很容易被破解，仅供参考
            while(1)
            {
                mprintf("encrypt check failed\n");
            }
        }
        #endif
    }
    #endif

    //读取分区信息表
    post_read_flash((char *)&partition_table,FILECONFIG_SPIFLASH_START_ADDR,sizeof(partition_table_t));
    if(partition_table.patitiontablechecksum != get_partition_list_checksum(&partition_table))
    {
        //CI_ASSERT(0,"patition table err\n"); //该语句会导致异常复位，所以更换下列方式处理
		mprintf("patition table err\n");
		while(1);
    }
    
    //检查分区有效性，如果有无效分区，返回错误，应用程序应该进入升级模式
    if (0 != partition_table.dnn_model_status ||
        0 != partition_table.asr_cmd_model_status ||
        0 != partition_table.voice_status ||
        0 != partition_table.user_file_status)
    {
#if OTA_ENABLE
        // 进入升级模式，升级完成后会重启系统，所以以下调用不会返回。
        firmware_update_main(OTA_UART_BAUDRATE);
#else
        return 2;
#endif
    }

    cinv_init(partition_table.nv_data_offset, partition_table.nv_data_size);
    ci_loginfo(LOG_NVDATA,"nv_data_offset = %08x\n",partition_table.nv_data_offset);
    ci_loginfo(LOG_NVDATA,"nv_data_size = %08x\n",partition_table.nv_data_size);

    uint32_t file_addr;
    if (get_file_addr(partition_table.user_file_offset, COMMAND_INFO_FILE_ID, &file_addr, NULL))
    {
        file_addr += partition_table.user_file_offset;
        return cmd_info_init(file_addr, partition_table.voice_offset, default_model_group_id);
    }
    return 1;
}


uint32_t get_dnn_addr_by_id(uint16_t dnn_file_id, uint32_t *p_dnn_addr, uint32_t *p_dnn_size)
{
    if (get_file_addr(partition_table.dnn_model_offset, dnn_file_id, p_dnn_addr, p_dnn_size))
    {
        *p_dnn_addr += partition_table.dnn_model_offset;
        return 1;
    }
    return 0;
}

uint32_t get_asr_addr_by_id(int asr_id, uint32_t *p_asr_addr, uint32_t *p_asr_size)
{
    if(((asr_id+1)>cmd_file_get_model_number())||(asr_id<0))
    {
        mprintf("asr_mode id err\n");
        return 0;
    }
    
    if (get_file_addr(partition_table.asr_cmd_model_offset, asr_id, p_asr_addr, p_asr_size))
    {
        mprintf("partition_table.asr_cmd_model_offset : 0x%x  asr_id is %d\n",partition_table.asr_cmd_model_offset,asr_id);
        *p_asr_addr += partition_table.asr_cmd_model_offset;
    }
    else
    {
        mprintf("get_file_addr:asr err\n");
        return 0;
    }
    
    return 1;
}



uint32_t get_current_model_addr(uint32_t *p_dnn_addr, uint32_t *p_dnn_size, uint32_t *p_asr_addr, uint32_t *p_addr_size)
{
    uint32_t dnn_file_id, asr_file_id;
    cmd_info_get_cur_model_id(&dnn_file_id, &asr_file_id, NULL);      //读取模型ID
 
    uint32_t failed = 0;

    //Get DNN address and size.
    if (get_file_addr(partition_table.dnn_model_offset, dnn_file_id, p_dnn_addr, p_dnn_size))
    {
        *p_dnn_addr += partition_table.dnn_model_offset;
    }
    else
    {
        failed = 1;
    }
    
    if (get_file_addr(partition_table.asr_cmd_model_offset, asr_file_id, p_asr_addr, p_addr_size))
    {
        *p_asr_addr += partition_table.asr_cmd_model_offset;
    }
    else
    {
        failed = 1;
    }
    
    if (failed)
    {
        while(1)mprintf("Error data in flash\n");
    }
    else
    {
        return 0;
    }
}

uint32_t get_userfile_addr(uint16_t file_id, uint32_t *p_file_addr, uint32_t *p_file_size)
{
    if ((NULL == p_file_addr) ||(NULL == p_file_size))
    {
        return 1;
    }

    if (get_file_addr(partition_table.user_file_offset, file_id, p_file_addr, p_file_size))
    {
        (*p_file_addr) += partition_table.user_file_offset;
        return 0;
    }

    return 1;
}

#define CFR_CATCH_SIZE          (1024*4)

typedef struct cached_flash_reader_info{
    int32_t start_addr_in_flash;
    uint8_t *cached_reader_buffer;
    int32_t cached_start_offset;
    int32_t cached_end_offset;
}cached_flash_reader_info_t;

static cached_flash_reader_info_t cached_flash_reader_info = {-CFR_CATCH_SIZE, NULL, 0,0};


uint32_t cached_flash_reader_init(uint32_t start_addr_in_flash)
{
    if (cached_flash_reader_info.cached_reader_buffer)
    {
        free(cached_flash_reader_info.cached_reader_buffer);
    }
    cached_flash_reader_info.cached_reader_buffer = malloc(CFR_CATCH_SIZE);
    if (cached_flash_reader_info.cached_reader_buffer)
    {
        cached_flash_reader_info.start_addr_in_flash = start_addr_in_flash;
        return 1;
    }
    return 0;
}


uint32_t cached_flash_reader_read(int32_t read_offset, uint8_t *read_buffer, uint32_t read_length)
{
    uint32_t first_read_length = 0;
    uint32_t second_read_length = 0;
	
    if (read_offset >= cached_flash_reader_info.cached_end_offset || read_offset < cached_flash_reader_info.cached_start_offset)
    {
        int32_t start_offset = cached_flash_reader_info.start_addr_in_flash + read_offset;
        //int32_t mod = start_offset % CFR_CATCH_SIZE;
        //start_offset = start_offset - mod;
        cached_flash_reader_info.cached_start_offset = start_offset - cached_flash_reader_info.start_addr_in_flash;
        cached_flash_reader_info.cached_end_offset = cached_flash_reader_info.cached_start_offset + CFR_CATCH_SIZE;
        post_read_flash((char *)cached_flash_reader_info.cached_reader_buffer, start_offset, CFR_CATCH_SIZE); 
    }
    
    uint32_t left_length = cached_flash_reader_info.cached_end_offset - read_offset;
    if (read_length < left_length)
    {
        first_read_length = read_length;
        second_read_length = 0;
    }
    else
    {
        first_read_length = left_length;
        second_read_length = read_length - left_length;
    }
    
    memcpy(read_buffer, cached_flash_reader_info.cached_reader_buffer+read_offset-cached_flash_reader_info.cached_start_offset, first_read_length);

    if (second_read_length)
    {
        int32_t start_offset = cached_flash_reader_info.start_addr_in_flash + read_offset;
        //int32_t mod = start_offset % CFR_CATCH_SIZE;
        //start_offset = start_offset - mod + CFR_CATCH_SIZE;
        cached_flash_reader_info.cached_start_offset = start_offset - cached_flash_reader_info.start_addr_in_flash;
        cached_flash_reader_info.cached_end_offset = cached_flash_reader_info.cached_start_offset + CFR_CATCH_SIZE;
        post_read_flash((char *)cached_flash_reader_info.cached_reader_buffer, start_offset, CFR_CATCH_SIZE);

        memcpy(read_buffer + first_read_length, cached_flash_reader_info.cached_reader_buffer+first_read_length, second_read_length);
		
        return read_length;
    }
    return first_read_length;
}

uint32_t cached_flash_reader_destroy(void)
{
    if (cached_flash_reader_info.cached_reader_buffer)
    {
        free(cached_flash_reader_info.cached_reader_buffer);
    }
    cached_flash_reader_info.cached_reader_buffer = NULL;
    cached_flash_reader_info.cached_start_offset = -1;
    cached_flash_reader_info.cached_end_offset = -1;
	return 0;
}

