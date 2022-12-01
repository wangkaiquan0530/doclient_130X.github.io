#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

// customer Ver (user define)
#define USER_VERSION_MAIN_NO                1
#define USER_VERSION_SUB_NO                 0
#define USER_TYPE                           "CustomerAA"
#define CI_CHIP_TYPE                        1122

#define USE_PROMPT_DECODER                  1   //播放器默认开启prompt解码器
#define USE_MP3_DECODER                     1   //为1时加入mp3解码器，需要调整lds配置：ROM_SIZE+28KB,FHEAP_SIZE-9KB,ASR_USED_SIZE-19KB
#define AUDIO_PLAY_SUPPT_MP3_PROMPT         1   //播放器默认开启mp3播报音
#define USE_FLAC_DECODER                    0   //为1时加入flac解码器，需要调整lds配置：ROM_SIZE+6KB,ASR_USED_SIZE-6KB
#define AUDIO_PLAY_SUPPT_FLAC_PROMPT        0   //播放器默认不开启flac播报音
#define AUDIO_PLAY_USE_DYNAMIC_DECODER_MEM  1

#define USE_SINGLE_MIC_DENOISE              0   //默认不开启denoise功能

#if USE_SINGLE_MIC_DENOISE
#define SEND_POSTALG_VOICE_TO_ASR           0   //降噪语音asr，默认为0
#endif

#define USE_LOWPOWER_DOWN_FREQUENCY         0   //默认不启用降频功能
#define USE_LOWPOWER_OSC_FREQUENCY          0   //默认不启用低功耗功能
#define USE_SEPARATE_WAKEUP_EN              1   //默认启用双网络切换（识别/唤醒网络）

#define MSG_USE_I2C_EN                      0   //默认不使能IIC通信协议
#define I2C_PROTOCOL_SPEED                (100) //默认IIC协议传输速度
#define UART_PROTOCOL_VER                 2   //默认使用二代协议，1：一代协议，2：二代协议，255：平台生成协议(只有发送没有接收)

// 播报音配置
#define PLAY_WELCOME_EN                     0 //欢迎词播报          =1是 =0否
#define PLAY_ENTER_WAKEUP_EN                0 //唤醒词播报          =1是 =0否
#define PLAY_EXIT_WAKEUP_EN                 0 //退出唤醒播报        =1是 =0否
#define PLAY_OTHER_CMD_EN                   0 //命令词播报          =1是 =0否
#define PLAY_CWSL_PROMPT_EN                 0 //自学习播报          =1是 =0否
// ASR配置
#define ADAPTIVE_THRESHOLD                  0 //ASR 自适应阈值    =1 开启  =0 关闭
#define ASR_SKIP_FRAME_CONFIG               0 //跳帧模式  0:关闭跳帧，1:只有命令词网络跳帧，2:所有模型都跳帧
#define AUDIO_PLAYER_ENABLE                 1 //用于屏蔽播放器任务相关代码      0：屏蔽，1：开启
#define OPEN_ASR_IN_ADVANCE                 0 //提示音播报时，是否提前开启语音识别。 0：不提前开启， 1：提前开启
#define ASR_DETAIL_RESULT                   0 //中间结果打印

#endif /* _USER_CONFIG_H_ */ 
