/**/
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define CI_CHIP_TYPE  1302//1301 没有1301的配置,使用1302的配置
#define BOARD_PORT_FILE                "CI-D02GS02S.c"//1301 没有1301的配置,使用1302的配置

#define GPIO_SET_IO_PULL_UP             1//GPIO 是否上拉

#define AUDIO_PLAYER_ENABLE 1 //用于屏蔽播放器任务相关代码      0：屏蔽，1：开启

#if AUDIO_PLAYER_ENABLE
#define USE_PROMPT_DECODER 1          //播放器是否支持prompt解码器
#define USE_MP3_DECODER 1             //为1时加入mp3解码器
#define AUDIO_PLAY_SUPPT_MP3_PROMPT 1 //播放器默认开启mp3播报音

#define AUDIO_PLAY_BLOCK_CONT 4 //播放器底层缓冲区个数
#endif

#define USE_ALC_AUTO_SWITCH_MODULE 1 //使用动态alc模块:1开启，0关闭
#define USE_DENOISE_MODULE 0         //使用降噪模块:1开启，0关闭
#define USE_AEC_MODULE 0             //使用回声消除模块:1开启，0关闭

#if USE_AEC_MODULE
#define PAUSE_VOICE_IN_WITH_PLAYING  0//开启aec时关闭
#endif



#define CONFIG_CI_LOG_UART HAL_UART0_BASE

#define MSG_COM_USE_UART_EN 0
#define UART_PROTOCOL_NUMBER (HAL_UART2_BASE) // HAL_UART0_BASE ~ HAL_UART2_BASE
#define UART_PROTOCOL_BAUDRATE (UART_BaudRate9600)
#define UART_PROTOCOL_VER 2 //串口协议版本号，1：一代协议，2：二代协议，255：平台生成协议(只有发送没有接收)

#define USE_CWSL                            1   //命令词自学习(command word self-learning)

#define USE_EXTERNAL_CRYSTAL_OSC             1      //使用外部晶振

#if (USE_EXTERNAL_CRYSTAL_OSC == 0)
#define UART_BAUDRATE_CALIBRATE         0           //是否使能波特率校准功能
#define BAUDRATE_SYNC_PERIOD            60000       // 波特率同步周期，单位毫秒
#define BAUD_CALIBRATE_MAX_WAIT_TIME    60          // 等待反馈包的超时时间，单位毫秒

// #define UART_CALIBRATE_FIX_BAUDRATE UART_PROTOCOL_BAUDRATE            //通信波特率
// #define UART_CALIBRATE_MAX_ERR_PROPORITION  0.08    //波特率允许的最大偏差
// #define UART_CALIBRATE_ARRAY_SIZE   16              //最大数据缓存
// #define UART_CALIBRATE_IO_BASE      PB              //校准波特率的IO口(RX引脚)
// #define UART_CALIBRATE_IO_PIN       pin_1
// #define UART_CALIBRATE_IO_PAD       PB1
// #define UART_CALIBRATE_IRQ          PB_IRQn
// #define UART_CALIBRATE_TIMER        TIMER2
// #define UART_CALIBRATE_TIMER_BASE   HAL_TIMER2_BASE
#endif

//播报音配置
#define PLAY_WELCOME_EN                     0 //欢迎词播报          =1是 =0否
#define PLAY_ENTER_WAKEUP_EN                0 //唤醒词播报          =1是 =0否
#define PLAY_EXIT_WAKEUP_EN                 0 //退出唤醒播报        =1是 =0否
#define PLAY_OTHER_CMD_EN                   0 //命令词播报          =1是 =0否
#define PLAY_CWSL_PROMPT_EN                 0 //自学习播报          =1是 =0否


#define BZZER_ONOFF  1           //-1- 开启蜂鸣器  0关闭

#endif /* _USER_CONFIG_H_ */
