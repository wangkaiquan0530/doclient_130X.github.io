/**
 * @file main.c
 * @brief 示例程序
 * @version 1.0.0
 * @date 2021-03-19
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */
#include <stdio.h> 
#include <malloc.h>
#include "FreeRTOS.h" 
#include "task.h"
#include "sdk_default_config.h"
#include "ci130x_core_eclic.h"
#include "ci130x_spiflash.h"
#include "ci130x_gpio.h"
#include "audio_play_api.h"
#include "audio_play_decoder.h"
#include "ci_flash_data_info.h"
// #include "ci130x_audio_capture.h"
#include "board.h"
#include "ci130x_uart.h"
#include "flash_manage_outside_port.h"
#include "system_msg_deal.h"
#include "ci130x_dpmu.h"
#include "ci130x_mailbox.h"
#include "ci130x_nuclear_com.h"
#include "flash_control_inner_port.h"
#include "romlib_runtime.h"
#include "audio_in_manage_inner.h"
#include "ci_log.h"
#include "status_share.h"
#include "platform_config.h"
#include "asr_api.h"
#include "alg_preprocess.h"
#include "ci130x_iwdg.h"
#if USE_CWSL 
#include "cwsl_manage.h"
#endif
/**
 * @brief 硬件初始化
 *          这个函数主要用于系统上电后初始化硬件寄存器到初始值，配置中断向量表初始化芯片io配置时钟
 *          配置完成后，系统时钟配置完毕，相关获取clk的函数可以正常调用
 */
static void hardware_default_init(void)
{
    /* 配置外设复位，硬件外设初始化 */
	extern void SystemInit(void);
    SystemInit();

	/* 设置中断优先级分组 */
	eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO0);
    
	/* 开启全局中断 */
	eclic_global_interrupt_enable();

	enable_mcycle_minstret();

	init_platform();

    /* 初始化maskrom lib */
    maskrom_lib_init();

    #if !(USE_INNER_LDO3)
    dpmu_ldo3_en(false);
    dpmu_config_update_en(DPMU_UPDATE_EN_NUM_LDO3);
    #endif

}


/**
 * @brief 用于平台初始化相关代码
 *
 * @note 在这里初始化硬件需要注意：
 *          由于部分驱动代码中使用os相关接口，在os运行前调用这些接口会导致中断被屏蔽
 *          其中涉及的驱动包括：QSPIFLASH、DMA、I2C、SPI
 *          所以这些外设的初始化需要放置在vTaskVariablesInit进行。
 *          如一定需要（非常不建议）在os运行前初始化这些驱动，请仔细确认保证：
 *              1.CONFIG_DIRVER_BUF_USED_FREEHEAP_EN  宏配置为0
 *              2.DRIVER_OS_API                     宏配置为0
 */
static int platform_init(void)
{   
    #if CONFIG_CI_LOG_UART
    ci_log_init();      //初始化日志模块
    #endif

    #if (CONFIG_CI_LOG_UART == UART_PROTOCOL_NUMBER && MSG_COM_USE_UART_EN)
	CI_ASSERT(0,"Log uart and protocol uart confict!\n");
    #endif
    
    #if CONFIG_SYSTEMVIEW_EN   
    /* 初始化SysView RTT，仅用于调试 */
	SEGGER_SYSVIEW_Conf();
	/* 使用串口方式输出sysview信息 */
	vSYSVIEWUARTInit();
	ci_logdebug(CI_LOG_DEBUG, "Segger Sysview Control Block Detection Address is 0x%x\n",&_SEGGER_RTT);
    #endif

    iwdg_init_t init;
    init.irq = iwdg_irqen_enable;
    init.res = iwdg_resen_enable;
    init.count = ((get_src_clk()/0x10)*3);/* IWDG时钟从src_clk经过16分频得到, 当前配置为2秒*/
    dpmu_set_device_gate(IWDG, ENABLE);
    dpmu_iwdg_reset_system_config();
    iwdg_init(IWDG,init);
    iwdg_open(IWDG);

	return 0;
}


/**
 * @brief sdk上电信息打印
 *
 */
static void welcome(void)
{
    ci_loginfo(LOG_USER,"\r\n");
    ci_loginfo(LOG_USER,"\r\n");
    ci_loginfo(LOG_USER,"ci130x_sdk_%s_%d.%d.%d Built-in\r\n",
               SDK_TYPE,
               SDK_VERSION,SDK_SUBVERSION,SDK_REVISION);
    ci_loginfo(LOG_USER,"\033[1;32mWelcome to CI130x_SDK.\033[0;39m\r\n");
    extern char heap_start;
    extern char heap_end;
    ci_loginfo(LOG_USER,"Heap size:%dKB\n", (&heap_end - &heap_start)/1024);
    ci_loginfo(LOG_USER,"Freq factor %d\n", (int)(get_freq_factor()*1000));
}


static void task_init(void *p_arg)
{
    #if UART_BAUDRATE_CALIBRATE
    //串口波特率自适应计算
    // extern void uart_calibrate_calc(void);
    // uart_calibrate_calc();
    #endif

    dsu_init();

    /* 注册录音codec */
    audio_in_codec_registe();
    
    //等待bnpu初始化完成后的消息，之后发送ack回应bnpu
    //nuclear_com_init需在mailboxboot_sync之前
    nuclear_com_init();

    /*各个通信组件的初始化*/
    decoder_port_inner_rpmsg_init();
    flash_control_inner_port_init();
    dnn_nuclear_com_outside_port_init();
    asr_top_nuclear_com_outside_port_init();
    vad_fe_nuclear_com_outside_port_init();
    flash_manage_nuclear_com_outside_port_init();
    codec_manage_inner_port_init();
    ciss_init();
    /*各个通信组件的初始化完成*/

	mailboxboot_sync();

    //注册语音前段信号处理模块
    extern ci_ssp_config_t ci_ssp;
    extern audio_capture_t audio_capture;
    REMOTE_CALL(set_ssp_registe(&audio_capture, (ci_ssp_st*)&ci_ssp, sizeof(ci_ssp)/sizeof(ci_ssp_st)));

    REMOTE_CALL(set_freqvad_start_para_gain(VAD_SENSITIVITY));
    
    //等待NPU flash管理初始化完成
    ciss_set(CI_SS_FLASH_HOST_STATE,CI_SS_FLASH_IDLE);
    bool is_power_off;
    is_flash_power_off(&is_power_off);
    while(is_power_off)
    {
        is_flash_power_off(&is_power_off);
        vTaskDelay(pdMS_TO_TICKS(1));
        // mprintf("wait flash power up\n");
    }
    //需等待flash驱动初始化完成
    //TODO: 这里面读flash用的memcpy
    ci_flash_data_info_init(DEFAULT_MODEL_GROUP_ID);

    /*离线命令词自学习功能信号量和任务*/
    #if USE_CWSL 
    cwsl_set_vad_alc_config(1);
    cwsl_init();
    #endif

    //audio_in_manage_inner_task需要asr创建成功之后再创建
    extern void decoder_task_init_port(void);
    decoder_task_init_port();

    //这里的栈原来的128有点不够，申请到的地址破坏了voice_table的一片区域
    xTaskCreate(audio_in_manage_inner_task,"audio_in_manage_inner_task",300,NULL,4,NULL);
    
    #if AUDIO_PLAYER_ENABLE
    /* 播放器任务 */
    audio_play_init();
    #endif

    /* 用户任务 */
    sys_msg_task_initial();
    xTaskCreate(UserTaskManageProcess,"UserTaskManageProcess",480,NULL,4,NULL);

    #if 1
    while(1) 
    {
        UBaseType_t ArraySize = 10;
        TaskStatus_t *StatusArray;
        ArraySize = uxTaskGetNumberOfTasks();
        StatusArray = pvPortMalloc(ArraySize*sizeof(TaskStatus_t));
        if (StatusArray && ArraySize)
        {
            uint32_t ulTotalRunTime;
            volatile UBaseType_t ArraySize2 = uxTaskGetSystemState(StatusArray, ArraySize, &ulTotalRunTime);
            mprintf("TaskName\t\tPriority\tTaskNumber\tMinStk\t%d\n", ArraySize2);
            for (int i = 0;i < ArraySize2;i++)
            {
                mprintf("% -16s\t%d\t\t%d\t\t%d\r\n",
                    StatusArray[i].pcTaskName,
                    (int)StatusArray[i].uxCurrentPriority,
                    (int)StatusArray[i].xTaskNumber,
                    (int)StatusArray[i].usStackHighWaterMark
                );
            }
            mprintf("\n");
            extern int get_heap_bytes_remaining_size(void);
            mprintf("Heap left: %dKB\n", get_heap_bytes_remaining_size()/1024);
        }
        if(StatusArray)
        {
            vPortFree(StatusArray);
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    #else
    vTaskDelete(NULL);
    #endif
}

/**
 * @brief 
 * 
 */
int main(void)
{
    hardware_default_init();

    /*平台相关初始化*/
    platform_init();

    /* 版本信息 */
    welcome();

    /* 创建启动任务 */
    xTaskCreate(task_init,"init task",280,NULL,4,NULL);

    /* 启动调度，开始执行任务 */
    vTaskStartScheduler();

    while(1){}
}


