/**
 * @file sdk_default_config.h
 * @brief sdk配置文件
 * @version 1.0
 * @date 2019-08-08
 * 
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 * 
 */
#ifndef _SDK_DEFAULT_CONFIG_H_
#define _SDK_DEFAULT_CONFIG_H_

#if defined(CI_CONFIG_FILE)
#include CI_CONFIG_FILE
#endif


/******************************** sdk config *********************************/
// SDK VER
#define SDK_VERSION                    1
#define SDK_SUBVERSION                 4
#define SDK_REVISION                   5
#define SDK_TYPE                       "Pro"

// 退出唤醒时间
#ifndef EXIT_WAKEUP_TIME
#define EXIT_WAKEUP_TIME                10000   //default exit wakeup time,unit ms
#endif

// volume set
#ifndef VOLUME_MAX
#define VOLUME_MAX                      7       //voice play max volume level
#endif
#ifndef VOLUME_MID
#define VOLUME_MID                      5       //voice play middle volume level
#endif
#ifndef VOLUME_MIN
#define VOLUME_MIN                      1       //voice play min volume level
#endif

#ifndef VOLUME_DEFAULT
#define VOLUME_DEFAULT                  5       //voice play default volume level
#endif

// customer Ver (user define)
#ifndef USER_VERSION_MAIN_NO
#define USER_VERSION_MAIN_NO            1
#endif
#ifndef USER_VERSION_SUB_NO
#define USER_VERSION_SUB_NO             0
#endif
#ifndef USER_TYPE
#define USER_TYPE                       "CustomerAA"
#endif

/*for save SRAM memory used, if function define with this macro,
must be carefull,function will cover by RTOS heap,
also can used #pragma location = ".init_call"  */
//#define ININT_CALL_FUNC @ ".init_call"

#ifndef ININT_CALL_FUNC
#define ININT_CALL_FUNC
#endif

//#define ININT_CALL_DATA @ ".init_one_data"
#define ININT_CALL_DATA

/* 串口调试LOG输出 */
#ifndef CONFIG_CI_LOG_UART
#define CONFIG_CI_LOG_UART                  HAL_UART0_BASE
#endif

/* 驱动中断采用事件标准组配置为1，采用全局变量配置为0 */
#ifndef DRIVER_OS_API
#define DRIVER_OS_API                       0
#endif

#ifndef CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
#define CONFIG_DIRVER_BUF_USED_FREEHEAP_EN  1
#endif

/* 在播放中是否暂停语音输入，一般来讲开启aec则不开启暂停语音输入否则开启 */
#ifndef PAUSE_VOICE_IN_WITH_PLAYING
#define PAUSE_VOICE_IN_WITH_PLAYING         1
#endif

/*是否允许关闭提示播报音 */
#ifndef PROMPT_CLOSE_EN
#define PROMPT_CLOSE_EN                     1
#endif

/*异常复位跳过上电播报 */
#ifndef EXCEPTION_RST_SKIP_BOOT_PROMPT
#define EXCEPTION_RST_SKIP_BOOT_PROMPT      1
#endif

/*默认模型分组ID，开机后第一次运行的识别环境 */
#ifndef DEFAULT_MODEL_GROUP_ID
#define DEFAULT_MODEL_GROUP_ID              0
#endif

/*VAD 指示灯开关 */
#ifndef VAD_LED_EN
#define VAD_LED_EN                          1
#if VAD_LED_EN
#define VAD_LIGHT_IO_PIN_BASE HAL_GPIO3_BASE
#define VAD_LIGHT_IO_PIN_NAME I2C0_SDA_PAD
#define VAD_LIGHT_IO_PIN_IOREUSE FIRST_FUNCTION
#define VAD_LIGHT_IO_PIN_PORT GPIO3
#define VAD_LIGHT_IO_PIN_NUMBER gpio_pin_1
#endif
#endif

/* 启用非唤醒状态切换模型 */
#ifndef USE_SEPARATE_WAKEUP_EN
#define USE_SEPARATE_WAKEUP_EN              0
#endif 

/* 启用降频模式 */
#ifndef USE_LOWPOWER_DOWN_FREQUENCY
#define USE_LOWPOWER_DOWN_FREQUENCY         0
#endif

/* 启用低功耗模式 */
#ifndef USE_LOWPOWER_OSC_FREQUENCY
#define USE_LOWPOWER_OSC_FREQUENCY          0
#endif


/* 使能IIS1相关引脚功能 */
#ifndef IIS1_ENABLE
#define IIS1_ENABLE                         1
#endif

/*************************** 语音前端算法 config *****************************/

/*单MIC DENOISE*/
#ifndef USE_SINGLE_MIC_DENOISE
#define USE_SINGLE_MIC_DENOISE             0
#endif

/*ALC AUTO SWITCH*/
#ifndef USE_ALC_AUTO_SWITCH
#define USE_ALC_AUTO_SWITCH                1
#endif


/*使用多个CODEC同时录音,需根据算法选择配合使用*/
#ifndef AUDIO_CAPTURE_USE_MULTI_CODEC
#define AUDIO_CAPTURE_USE_MULTI_CODEC      0
#endif

/*是否codec录音只支持单个声道,需根据算法选择配合使用*/
#ifndef AUDIO_CAPTURE_USE_SINGLE_CHANNEL
#define AUDIO_CAPTURE_USE_SINGLE_CHANNEL   1
#endif

/*是否有工频干扰*/
#ifndef HAS_POWER_FREQUENCY_DISTURB
#define HAS_POWER_FREQUENCY_DISTURB        1
#endif

#ifndef USE_ESTVAD
#define USE_ESTVAD                         0
#endif

/*根据codec使用的声道数来配置使用内存大小*/
#if !CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
#if AUDIO_CAPTURE_USE_SINGLE_CHANNEL
    #define AUDUI_CAPTURE_NO_USE_MALLOC_BLOCK_SIZE (512U)
#else
    #define AUDUI_CAPTURE_NO_USE_MALLOC_BLOCK_SIZE (1024U)
#endif
#endif

/******************************** VP config **********************************/

//1:开启VP计算；0：关闭VP计算//目前的声纹需打开PSRAM
#ifndef USE_VP
#define USE_VP                              0
#endif

#ifndef VP_TEMPLATE_NUM
#define VP_TEMPLATE_NUM                     10
#endif

/******************************** command word self-learning config **********************************/
#ifndef USE_CWSL
#define USE_CWSL                             0 //命令词自学习(command word self-learning)
#endif

#ifndef CICWSL_TOTAL_TEMPLATE
#define CICWSL_TOTAL_TEMPLATE                6 //可存储模板数量
#endif


#if 0
#ifndef INNER_CODEC_AUDIO_IN_USE_RESAMPLE
#define INNER_CODEC_AUDIO_IN_USE_RESAMPLE   0//语音输入是否使用重采样，默认关闭（开了也没用）
#endif
#endif 

#ifndef AUDIO_CAPTURE_USE_CONST_PARA
#define AUDIO_CAPTURE_USE_CONST_PARA 1
#endif

#ifndef  AUDIO_CAPTURE_USE_ONLY_INNER_CODEC
#define AUDIO_CAPTURE_USE_ONLY_INNER_CODEC 1
#endif

/**************************** audio player config ****************************/
/* 注册prompt解码器 */
#ifndef USE_PROMPT_DECODER
#define USE_PROMPT_DECODER                   1
#endif

/* 注册mp3解码器 */
#ifndef USE_MP3_DECODER
#define USE_MP3_DECODER                     0
#endif

/* 注册m4a(aac)解码器 */
#ifndef USE_AAC_DECODER
#define USE_AAC_DECODER                     0
#endif

/* 注册ms wav解码器 */
#ifndef USE_MS_WAV_DECODER
#define USE_MS_WAV_DECODER                  0
#endif

/* 注册flac解码器 */
#ifndef USE_FLAC_DECODER
#define USE_FLAC_DECODER                    0
#endif

/* 命令词支持MP3格式音频，注意目前仅支持具有CI特殊头格式的MP3文件 */
#ifndef AUDIO_PLAY_SUPPT_MP3_PROMPT
#define AUDIO_PLAY_SUPPT_MP3_PROMPT         0
#endif
#if AUDIO_PLAY_SUPPT_MP3_PROMPT
#if !USE_MP3_DECODER
#error "if AUDIO_PLAY_SUPPT_MP3_PROMPT enabled please config USE_MP3_DECODER enable in your user_config.h"
#endif
#endif

/* 命令词支持FLAC格式音频，注意目前仅支持具有CI特殊头格式的FLAC文件 */
#ifndef AUDIO_PLAY_SUPPT_FLAC_PROMPT
#define AUDIO_PLAY_SUPPT_FLAC_PROMPT        0
#endif
#if AUDIO_PLAY_SUPPT_FLAC_PROMPT
#if !USE_FLAC_DECODER
#error "if AUDIO_PLAY_SUPPT_FLAC_PROMPT enabled please config USE_FLAC_DECODER enable in your user_config.h"
#endif
#endif

/* 命令词支持标准IMA ADPCM格式音频 */
#ifndef AUDIO_PLAY_SUPPT_IMAADPCM_PROMPT
#define AUDIO_PLAY_SUPPT_IMAADPCM_PROMPT    1
#endif

/* 播放器底层缓冲区个数 */
#ifndef AUDIO_PLAY_BLOCK_CONT
    #define AUDIO_PLAY_BLOCK_CONT          (4)
#endif

/* 解码器内存使用动态分配 */
#ifndef AUDIO_PLAY_USE_DYNAMIC_DECODER_MEM
#define AUDIO_PLAY_USE_DYNAMIC_DECODER_MEM  0
#endif

/* 数据组合播报 */
#ifndef AUDIO_PLAY_USE_QSPI_FLASH_LIST
#define AUDIO_PLAY_USE_QSPI_FLASH_LIST      0
#endif

/* 启用网络播放（需要lwip支持） */
#ifndef AUDIO_PLAY_USE_NET
#define AUDIO_PLAY_USE_NET                  0
#endif

/* 启用自定义外部数据源播放 */
#ifndef AUDIO_PLAY_USE_OUTSIDE
#define AUDIO_PLAY_USE_OUTSIDE              0
#endif

/* 启用自定义外部数据源播放 */
#ifndef AUDIO_PLAY_USE_OUTSIDE_V2
#define AUDIO_PLAY_USE_OUTSIDE_V2           0
#endif

/* 启用文件系统播放 */
#ifndef AUDIO_PLAY_USE_SD_CARD
#define AUDIO_PLAY_USE_SD_CARD              0
#endif

/*! 启用播放器双声道混音到右声道功能 */
#ifndef AUDIO_PLAY_USE_MIX_2_CHANS
#define AUDIO_PLAY_USE_MIX_2_CHANS                     0
#endif

/*! 自动音频识别文件头(消耗额外内存,播放m4a、flac、非单声道16Kwav音频格式时必须打开) */
#ifndef AUDIO_PLAYER_CONFIG_AUTO_PARSE_AUDIO_FILE
#define AUDIO_PLAYER_CONFIG_AUTO_PARSE_AUDIO_FILE      0
#endif

/*! 针对TTS音频倍速播放功能，注意仅支持加/减速播放TTS人声，不支持音乐 */
#ifndef AUDIO_PLAY_USE_SPEEDING_SPEECH
#define AUDIO_PLAY_USE_SPEEDING_SPEECH                 0
#endif


/*! 唤醒词播放词常驻内存 */
#ifndef AUDIO_PLAY_SUPPT_WAKEUP_VOICE_IN_RAM
#define AUDIO_PLAY_SUPPT_WAKEUP_VOICE_IN_RAM           0
#endif

/*! 用于解决应用程序可能存在的偏移不对齐问题 */
#ifndef AUDIO_PLAYER_FIX_OFFSET_ISSUE
#define AUDIO_PLAYER_FIX_OFFSET_ISSUE                  0
#endif


/****************************** 功放控制 config *******************************/

/*Whether the PA switch is controled by audio player*/
#ifndef PLAYER_CONTROL_PA
#define PLAYER_CONTROL_PA                              0
#endif

/*Power amplifier control level */
// 已修改为自动检测配置，不需要此宏
// #ifndef PA_CONTROL_LEVEL
// #define PA_CONTROL_LEVEL                               0    //1:高电平使能，0：低电平使能
// #endif

#ifndef PA_CONTROL_GPIO_BASE
#define PA_CONTROL_GPIO_BASE GPIO3
#endif
#ifndef PA_CONTROL_GPIO_PIN
#define PA_CONTROL_GPIO_PIN gpio_pin_2
#endif
#ifndef PA_CONTROL_PINPAD_NAME
#define PA_CONTROL_PINPAD_NAME GPIO3_2_PAD
#endif
#ifndef PA_CONTROL_FUN_CHOICE
#define PA_CONTROL_FUN_CHOICE FIRST_FUNCTION
#endif


/****************************** 串口协议 config *******************************/
#ifndef MSG_COM_USE_UART_EN
#define MSG_COM_USE_UART_EN                1
#endif


#ifndef UART_PROTOCOL_NUMBER
#define UART_PROTOCOL_NUMBER	          (HAL_UART1_BASE)// HAL_UART0_BASE ~ HAL_UART1_BASE
#endif

#ifndef UART_PROTOCOL_BAUDRATE
#define UART_PROTOCOL_BAUDRATE            (UART_BaudRate9600)
#endif

#ifndef UART_PROTOCOL_VER
#define UART_PROTOCOL_VER                 2   //串口协议版本号，1：一代协议，2：二代协议，255：平台生成协议(只有发送没有接收)
#endif

/***************************** flash加密 config ******************************/
/*flash防拷贝加密 */
#ifndef COPYRIGHT_VERIFICATION
#define COPYRIGHT_VERIFICATION          0 //1：使能flash加密校验，用于防止flash拷贝。需要在lds文件增加ROM段大小。0：禁止flash加密校验
#endif

#if (COPYRIGHT_VERIFICATION == 1)
#define ENCRYPT_DEFAULT                 0 //默认加密算法
#define ENCRYPT_USER_DEFINE             1 //用户自定义加密算法

#define ENCRYPT_ALGORITHM               ENCRYPT_DEFAULT //设置加密方式
#endif


/*********************** 使用外部IIS输入语音进行识别 *************************/
#ifndef USE_OUTSIDE_IIS_TO_ASR
#define USE_OUTSIDE_IIS_TO_ASR 0
#endif

#if USE_OUTSIDE_IIS_TO_ASR
    #if 0//INNER_CODEC_AUDIO_IN_USE_RESAMPLE
    #error "use outside codec, don't resample\n"
    #endif
#endif

#ifndef USE_OUTSIDE_IIS_TO_ASR_THIS_BORD_SLAVE//使用外部IIS进行ASR的时候，这块板子做slave
#define USE_OUTSIDE_IIS_TO_ASR_THIS_BORD_SLAVE 0
#endif


#ifndef USE_OUTSIDE_CODEC_PLAY
#define USE_OUTSIDE_CODEC_PLAY      0//使用外部codec进行播放
#endif

/******************************OTA config************************************/
#ifndef OTA_ENABLE
#define OTA_ENABLE              0           //使能OTA功能
#endif

#ifndef OTA_UPDATE_PORT
#define OTA_UPDATE_PORT         UART1       //配置OTA功能使用的串口
#endif

#ifndef OTA_UART_BAUDRATE
#define OTA_UART_BAUDRATE       921600      //配置OTA串口的波特率
#endif


/****************************** i2c communicate config *******************************/
#ifndef MSG_USE_I2C_EN
#define MSG_USE_I2C_EN                0                  /*使能IID通信协议*/
#endif

#ifndef I2C_PROTOCOL_SPEED
#define I2C_PROTOCOL_SPEED            (100)              /*IIC传输速度*/
#endif

/******************************I2S interface config************************************/
#ifndef USE_I2S_INTERFACE_SCK_LRCK_32
#define USE_I2S_INTERFACE_SCK_LRCK_32              0//I2S 传输语音数据到WIFI时，SCK与LRCK的比值是否为32，否则为64
#endif

// 播报音配置
#ifndef PLAY_WELCOME_EN
#define PLAY_WELCOME_EN                     1 //欢迎词播报          =1是 =0否
#endif
#ifndef PLAY_ENTER_WAKEUP_EN
#define PLAY_ENTER_WAKEUP_EN                1 //唤醒词播报          =1是 =0否
#endif
#ifndef PLAY_EXIT_WAKEUP_EN
#define PLAY_EXIT_WAKEUP_EN                 1 //退出唤醒播报        =1是 =0否
#endif
#ifndef PLAY_OTHER_CMD_EN
#define PLAY_OTHER_CMD_EN                   1 //命令词播报          =1是 =0否
#endif

/******************************ASR config************************************/
#ifndef ADAPTIVE_THRESHOLD
#define ADAPTIVE_THRESHOLD                  1   //ASR 自适应阈值    =1 开启  =0 关闭
#endif

#ifndef ASR_SKIP_FRAME_CONFIG
#define ASR_SKIP_FRAME_CONFIG               0   //跳帧模式  0:关闭跳帧，1:只有命令词网络跳帧，2:所有模型都跳帧
#endif
#ifndef AUDIO_PLAYER_ENABLE
#define AUDIO_PLAYER_ENABLE                 1 //用于屏蔽播放器任务相关代码      0：屏蔽，1：开启
#endif

#ifndef OPEN_ASR_IN_ADVANCE
#define OPEN_ASR_IN_ADVANCE                 0 //提示音播报时，是否提前开启语音识别。 0：不提前开启， 1：提前开启
#endif

#ifndef TEXT_INIT_IN_FLASH
#define TEXT_INIT_IN_FLASH                  0 //DEBUG再置1
#endif                     

#endif /*_SDK_DEFAULT_CONFIG_H_*/
