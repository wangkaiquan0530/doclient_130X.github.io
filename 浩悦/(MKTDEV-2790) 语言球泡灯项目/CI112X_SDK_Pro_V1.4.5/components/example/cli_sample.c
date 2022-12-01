/**
 * @file cli_sample.c
 * @brief CLI示例
 * @example cli_sample.c
 *       这个文件是 assist/cli 命令行组件使用示例代码
 * 
 * @version 2.0
 * @date 2019-03-03
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#include "sdk_default_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS_CLI.h"
#include "audio_play_api.h"
#include "prompt_player.h"
//#include "ff.h"
#include "ci112x_codec.h"
#if defined(__ICCARM__)
#include <iar_dlmalloc.h>
#endif
#if defined(__GNUC__)
#include <malloc.h>
#endif

/********************************* uname ******************************************/
static BaseType_t prvUnameCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "FreeRTOS" );

	return pdFALSE;
}
static const CLI_Command_Definition_t xUname =
{
	"uname",
	"uname:\t\t\t\tEchos uname in turn\r\n",
	NULL,
	prvUnameCommand,
	0
};


/********************************* echo ******************************************/
static BaseType_t prvEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,	1, &xParameterStringLength);

    configASSERT( pcParameter );

    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strncat( pcWriteBuffer, pcParameter, ( size_t ) xParameterStringLength );

	return pdFALSE;
}
static const CLI_Command_Definition_t xEcho =
{
	"echo",
	"echo:\t\t\t\tEchos each in turn\r\n",
	"echo\r\n\
    <param>:\t\techos string\r\n",
	prvEchoCommand,
	1
};


/********************************* ps ************************************************/
static BaseType_t prvPsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *const pcHeader = "\tState\tPRI\tStack\t#\r\n************************************************\r\n";
BaseType_t xSpacePadding;

	strcpy( pcWriteBuffer, "Task" );
	pcWriteBuffer += strlen( pcWriteBuffer );

	configASSERT( configMAX_TASK_NAME_LEN > 3 );
	for( xSpacePadding = strlen( "Task" ); xSpacePadding < ( configMAX_TASK_NAME_LEN - 3 ); xSpacePadding++ )
	{
		*pcWriteBuffer = ' ';
		pcWriteBuffer++;

		*pcWriteBuffer = 0x00;
	}
	strcpy( pcWriteBuffer, pcHeader );
	vTaskList( pcWriteBuffer + strlen( pcHeader ) );
    pcWriteBuffer[strlen(pcWriteBuffer)-2] = '\0';
    
	return pdFALSE;
}
static const CLI_Command_Definition_t xPs =
{
  	"ps",
	"ps:\t\t\t\t查看任务状态\r\n",
	NULL,
	prvPsCommand,
	0
};


/********************************* free **********************************************/
static BaseType_t prvFreeCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    #if defined(__ICCARM__)
    struct mallinfo heap_meminfo = __iar_dlmallinfo();/*可以查看动态分配内存的总大小*/
    sprintf(pcWriteBuffer,"HEAP \t\talloc: %d free:%d\r\n",heap_meminfo.uordblks,heap_meminfo.fordblks);
    pcWriteBuffer += strlen(pcWriteBuffer);
    #endif
	#if defined(__GNUC__)
	extern int get_heap_bytes_remaining_size(void);
	struct mallinfo mi = mallinfo();/*可以查看动态分配内存的总大小*/
	sprintf(pcWriteBuffer,"HEAP \t\talloc: %d free:%d\r\n",mi.uordblks, mi.fordblks + get_heap_bytes_remaining_size());
	pcWriteBuffer += strlen(pcWriteBuffer);
	#endif
    size_t fheap_meminfo = xPortGetFreeHeapSize();
    sprintf(pcWriteBuffer,"freeRTOS HEAP \talloc: %d free:%d",configTOTAL_HEAP_SIZE-fheap_meminfo,fheap_meminfo);

	return pdFALSE;
}
static const CLI_Command_Definition_t xFree =
{
	"free",
    "free:\t\t\t\t查看系统内存使用\r\n",
    NULL,
	prvFreeCommand,
	0
};


/********************************* version *******************************************/
static BaseType_t prvVersionCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    extern const char * const pcWelcomeMessage;
    strcpy( pcWriteBuffer, pcWelcomeMessage);
	return pdFALSE;
}
static const CLI_Command_Definition_t xVersion =
{
	"version",
    "version:\t\t\t查看系统版本\r\n",
    NULL,
	prvVersionCommand,
	0
};

/********************************* set_volume ******************************************/
static BaseType_t prvSetVolumeCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    int volume = 0;
    BaseType_t xParameterStringLength;

    memset( pcWriteBuffer, 0x00, xWriteBufferLen );

    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,	1, &xParameterStringLength);
    if(xParameterStringLength == 1)
    {
        volume = (pcParameter[0] - '0')*10;
    }
    else
    {
        volume = 60;
    }
    
    if(volume > 100) volume = 100;
    if(volume < 0) volume = 0;
    audio_play_set_vol_gain(volume);
    
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xSetVolume =
{
	"set_volume",
    "set_volume:\t\t\t设置音量\r\n",
    NULL,
	prvSetVolumeCommand,
	1
};


/********************************* set_play_speed ********************************/
static BaseType_t prvSetPlaySpeedCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *pcParameter;
    int volume = 0;
    float fvolume = 1.0f;
    BaseType_t xParameterStringLength;

    memset( pcWriteBuffer, 0x00, xWriteBufferLen );

    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,	1, &xParameterStringLength);
    if(xParameterStringLength == 1)
    {
        volume = (pcParameter[0] - '0');
    }
    else
    {
        volume = 5;
    }
    
    if(volume > 9) volume = 9;
    if(volume < 0) volume = 0;
    switch(volume)
    {
        case 0:
            fvolume = 0.5f;
            break;
        case 1:
            fvolume = 0.6f;
            break;
        case 2:
            fvolume = 0.7f;
            break;
        case 3:
            fvolume = 0.8f;
            break;
        case 4:
            fvolume = 0.9f;
            break;
        case 5:
            fvolume = 1.0f;
            break;
        case 6:
            fvolume = 1.25f;
            break;
        case 7:
            fvolume = 1.5f;
            break;
        case 8:
            fvolume = 1.75f;
            break;
        case 9:
            fvolume = 2.0f;
            break;
    }
    set_play_speed(fvolume);
    
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xSetPlaySpeed =
{
	"set_play_speed",
    "set_play_speed:\t\t\t设置播放速度\r\n",
    NULL,
	prvSetPlaySpeedCommand,
	1
};


/********************************* play_aac ******************************************/
static BaseType_t prvPlayAACCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    play_audio("/", "test128.aac", 0, "aac", NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayAAC =
{
	"play_aac",
    "play_aac:\t\t\t播放test128.aac\r\n",
    NULL,
	prvPlayAACCommand,
	0
};


/********************************* play_m4a ******************************************/
static BaseType_t prvPlayM4ACommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    play_audio("/", "music_demo3_stardard.m4a", 0, "m4a", NULL);//0xd055
    //play_audio("/", "music_demo3_high.m4a", 0, "m4a", NULL);//0xd056
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayM4A =
{
	"play_m4a",
    "play_m4a:\t\t\t播放music_demo3_stardard.m4a\r\n",
    NULL,
	prvPlayM4ACommand,
	0
};


/********************************* play_ms_wav ******************************************/
static BaseType_t prvPlayMSWavCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    play_audio("/", "tts_stream1.wav", 0, "ms_wav",NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayMSWav =
{
	"play_ms_wav",
    "play_ms_wav:\t\t\t播放tts_stream1.wav\r\n",
    NULL,
	prvPlayMSWavCommand,
	0
};


/********************************* play_flac ******************************************/
static BaseType_t prvPlayFlacCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    play_audio("/", "music_demo3_pretect.flac", 0, "flac",NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayFlac =
{
	"play_flac",
    "play_flac:\t\t\t播放music_demo3_pretect.flac\r\n",
    NULL,
	prvPlayFlacCommand,
	0
};


/********************************* play_mp3 ******************************************/
static BaseType_t prvPlayMP3Command( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    play_audio("/", "test128.mp3", 0, "mp3",NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayMP3 =
{
	"play_mp3",
    "play_mp3:\t\t\t播放test128.mp3\r\n",
    NULL,
	prvPlayMP3Command,
	0
};


/********************************* play_adpcm ******************************************/
static BaseType_t prvPlayADPCMCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    prompt_play_by_cmd_id((rand()%79+1),-1,NULL,true);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayADPCM =
{
	"play_adpcm",
    "play_adpcm:\t\t\t随机播放adpcm播报词\r\n",
    NULL,
	prvPlayADPCMCommand,
	0
};


/********************************* stop_play ******************************************/
static BaseType_t prvPlayStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    stop_play(NULL,NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayStop =
{
	"stop_play",
    "stop_play:\t\t\t停止播放\r\n",
    NULL,
	prvPlayStopCommand,
	0
};


/********************************* pause_play ******************************************/
static BaseType_t prvPlayPauseCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    pause_play(NULL,NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayPause =
{
	"pause_play",
    "pause_play:\t\t\t暂停播放\r\n",
    NULL,
	prvPlayPauseCommand,
	0
};


/********************************* resume_play ******************************************/
static BaseType_t prvPlayResumeCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    continue_history_play(NULL);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xPlayResume =
{
	"resume_play",
    "resume_play:\t\t\t恢复播放\r\n",
    NULL,
	prvPlayResumeCommand,
	0
};


/********************************* insert_play_prompt ******************************************/
static void cplay_continue(cmd_handle_t cmd_handle)
{
    continue_history_play(NULL);
}
static BaseType_t prvInsertPlayCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    prompt_play_by_cmd_id((rand()%79+1),-1,cplay_continue,true);
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( pcWriteBuffer, "Success!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xInsertPlay =
{
	"insert_play_prompt",
    "insert_play_prompt:\t\t暂停当前播放，插播adpcm播报词\r\n",
    NULL,
	prvInsertPlayCommand,
	0
};


/********************************* audio ******************************************/
static BaseType_t prvTestAudioCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *pcParameter[3];
	BaseType_t xParameterStringLength[3];
	static char file[30];

	for(int i = 0; i < 3; i++)
	{
		pcParameter[i] = FreeRTOS_CLIGetParameter(pcCommandString,	i+1, &xParameterStringLength[i]);
	}
    
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    
    if(!strncmp(pcParameter[0],"flash",xParameterStringLength[0]))
	{
		int ID;

		if(xParameterStringLength[1] >= 2 )
		{
			ID = pcParameter[1][1]-48+10*(pcParameter[1][0]-48);
		}
		else
		{
			ID = pcParameter[1][0]-48;
		}

		if(!strncmp(pcParameter[2],"yes",xParameterStringLength[2]))
		{	
            prompt_play_by_cmd_id(ID,-1,cplay_continue,true);
		}
		else
		{
            prompt_play_by_cmd_id(ID,-1,NULL,true);
		}
        strcpy( pcWriteBuffer, "Success!" );
	}
    #if 0
	else if(!strncmp(pcParameter[0],"sdc",xParameterStringLength[0]))
	{
        memcpy(file,pcParameter[1],xParameterStringLength[1]);
        file[xParameterStringLength[1]] = '\0';
		for(int i = 0; i < xParameterStringLength[1]; i++)
		{
			if(pcParameter[1][i] == '.')
			{
				if(pcParameter[1][i+2] == 'p')
				{
					play_audio("/", file, 0, "mp3",NULL);
				}
				else if(pcParameter[1][i+1] == 'a')
				{
					play_audio("/", file, 0, "aac",NULL);
				}
                else if(pcParameter[1][i+2] == '4')
				{
					play_audio("/", file, 0, "m4a",NULL);
				}
                else if(pcParameter[1][i+1] == 'w')
				{
					play_audio("/", file, 0, "ms_wav",NULL);
				}
                else if(pcParameter[1][i+1] == 'f')
				{
					play_audio("/", file, 0, "flac",NULL);
				}
				break;
			}
		}
        strcpy( pcWriteBuffer, "Success!" );
	}
    #endif
    #if 0
	else if(!strncmp(pcParameter[0],"net",xParameterStringLength[0]))
	{
        /* 来自网络的数据让播放器自动识别解码器 */
		play_audio(pcParameter[1], NULL, 0, NULL,NULL);  
        strcpy( pcWriteBuffer, "Success!" );
	}
    #endif
    else
    {
        strcpy( pcWriteBuffer, "Fail!" );
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t xTestAudio =
{
	"audio",
	"audio:\t\t\t\t音频播放\r\n",
    "audio\r\n\
    [device]:\t\tsdc/flash/net\r\n\
    [file]:\t\tfilename/AsrIndexID/url\r\n\
    [pause]:\t\tyes/no\r\n",
	prvTestAudioCommand,
	-1
};

#if 0
/********************************* test_sdc ******************************************/
static char * scan_files(char *path, char *pcWriteBuffer)
{
    FRESULT res; //部分在递归过程被修改的变量，不用全局变量
    FILINFO fno;
    DIR dir;
    int i;
    char *fn; // 文件名
    char *outBuffer = pcWriteBuffer;

#if _USE_LFN
    /* 长文件名支持 */
    /* 简体中文需要2个字节保存一个“字”*/
    static char lfn[_MAX_LFN * 2 + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif
    //打开目录
    res = f_opendir(&dir, path);
    if (res == FR_OK)
    {
        i = strlen(path);
        (void)i;
        for (;;)
        {
            //读取目录下的内容，再读会自动读下一个文件
            res = f_readdir(&dir, &fno);
            //为空时表示所有项目读取完毕，跳出
            if (res != FR_OK || fno.fname[0] == 0)
            {
                break;
            }
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            //点表示当前目录，跳过
            if (*fn == '.')
                continue;
            //目录，递归读取
            if (fno.fattrib & AM_DIR)
            {
                #if 0
                //合成完整目录名
                sprintf(&path[i], "/%s", fn);
                //递归遍历
                res = scan_files(path,outBuffer);
                path[i] = 0;
                //打开失败，跳出循环
                if (res != FR_OK)
                {
                    break;
                }
                #endif
            }
            else
            {
                //strcpy( outBuffer, path );
                //outBuffer += strlen(path);
                strcpy( outBuffer, fn );
                outBuffer += strlen(fn);
                strcpy( outBuffer, "\r\n" );
                outBuffer += strlen("\r\n");
            }                           
        }                             
    }
    return outBuffer;
}
static BaseType_t prvxLsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    strcpy( scan_files("0:/",pcWriteBuffer), "\r\nScan end!" );
	return pdFALSE;
}
static const CLI_Command_Definition_t xLs =
{
	"ls",
	"ls:\t\t\t\t列出sd卡根目录文件\r\n",
    NULL,
	prvxLsCommand,
	0
};
#endif

#if 0//USE_SDIO
#include "lightduer_dcs.h"
extern void send_record_data(void);
extern bool duer_connet_ok(void);
/********************************* test_dueros ******************************************/
static BaseType_t prvTestDuerOSSendCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    if(duer_connet_ok())
    {
        vTaskDelay(2);
        /* 发送一个压缩好的测试音频 */
        send_record_data();
        strcpy( pcWriteBuffer, "Success!" );
    }
    else
    {
        strcpy( pcWriteBuffer, "dueros not started!" );
    }

	return pdFALSE;
}
static const CLI_Command_Definition_t xTestDuerOSSend =
{
	"dueros_send",
	"dueros_send:\t\t\t发送一段音频到DuerOS服务器\r\n",
    NULL,
	prvTestDuerOSSendCommand,
	0
};


/********************************* test_dueros ******************************************/
static BaseType_t prvTestDuerOSStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    memset( pcWriteBuffer, 0x00, xWriteBufferLen );
    if(duer_connet_ok())
    {
        duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
        strcpy( pcWriteBuffer, "Success!" );
    }
    else
    {
        strcpy( pcWriteBuffer, "dueros not started!" );
    }

	return pdFALSE;
}
static const CLI_Command_Definition_t xTestDuerOSStop =
{
	"dueros_stop",
	"dueros_stop:\t\t\t通知DuerOS停止播放\r\n",
    NULL,
	prvTestDuerOSStopCommand,
	0
};
#endif


void vRegisterCLICommands( void )
{
	FreeRTOS_CLIRegisterCommand( &xUname );	
    FreeRTOS_CLIRegisterCommand( &xVersion );	
	FreeRTOS_CLIRegisterCommand( &xEcho );
    FreeRTOS_CLIRegisterCommand( &xPs );
    FreeRTOS_CLIRegisterCommand( &xFree );
    
    FreeRTOS_CLIRegisterCommand( &xPlayADPCM );
    FreeRTOS_CLIRegisterCommand( &xPlayStop );
    FreeRTOS_CLIRegisterCommand( &xPlayPause );
    FreeRTOS_CLIRegisterCommand( &xPlayResume );
    
    FreeRTOS_CLIRegisterCommand( &xInsertPlay );

    FreeRTOS_CLIRegisterCommand( &xSetVolume );
    FreeRTOS_CLIRegisterCommand( &xSetPlaySpeed );
    
    FreeRTOS_CLIRegisterCommand( &xTestAudio );
    #if 0//USE_SDC
    FreeRTOS_CLIRegisterCommand( &xPlayMP3 );
    FreeRTOS_CLIRegisterCommand( &xPlayAAC );
    FreeRTOS_CLIRegisterCommand( &xPlayM4A );
    FreeRTOS_CLIRegisterCommand( &xPlayMSWav );
    FreeRTOS_CLIRegisterCommand( &xPlayFlac );
    FreeRTOS_CLIRegisterCommand( &xLs );
    //FreeRTOS_CLIRegisterCommand( &xTestSDC );
    #endif

    #if 0//USE_SDIO
    FreeRTOS_CLIRegisterCommand( &xTestDuerOSSend );
    FreeRTOS_CLIRegisterCommand( &xTestDuerOSStop );
    #endif
    
}
