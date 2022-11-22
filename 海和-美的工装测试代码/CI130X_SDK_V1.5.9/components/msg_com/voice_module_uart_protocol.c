


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "sdk_default_config.h"

#include "voice_module_uart_protocol.h"

#include "system_msg_deal.h"
#include "command_info.h"
#include "ci130x_spiflash.h"
#include "ci130x_dpmu.h"
#include "prompt_player.h"
#include "audio_play_api.h"
// #include "flash_rw_process.h"
#include "asr_api.h"
#include "firmware_updater.h"
#include <string.h>
#include <stdlib.h>
#include "romlib_runtime.h"
#include "flash_manage_outside_port.h"
#include "baudrate_calibrate.h"

#if (UART_PROTOCOL_VER == 2)

#define TIMEOUT_ONE_PACKET_INTERVAL (1000)/*ms, in this code, it should be bigger than portTICK_PERIOD_MS */
#define MAX_DATA_RECEIVE_PER_PACKET (80)/*???*/



extern void  resume_voice_in(void);
extern void  pause_voice_in(void);
extern void set_state_enter_wakeup(uint32_t exit_wakup_ms);


/********************************************************************
                     port function
********************************************************************/
static SemaphoreHandle_t send_packet_mutex;

#define log_debug(fmt, args...) do{}while(0)
#if USE_STD_PRINTF
#define log_info printf
#define log_error printf
#else
#define log_info _printf
#define log_error _printf
#endif
#define cinv_assert(x) do{}while(0)

static uint32_t baud_sync_req;

static void vmup_port_mutex_creat(void)
{
    send_packet_mutex = xSemaphoreCreateMutex();

    if (NULL == send_packet_mutex) 
    {
        log_error("%s, error\n",__func__);
    }
    log_debug("%s\n",__func__);
}


static void vmup_port_mutex_take(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) 
    {
        if (xSemaphoreTake(send_packet_mutex, portMAX_DELAY) == pdFALSE)
        {
            log_error("%s, error\n",__func__);
        }
        log_debug("%s\n",__func__);
    }
}


static void vmup_port_mutex_give(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) 
    {
        if (xSemaphoreGive(send_packet_mutex) == pdFALSE) 
        {
            log_error("%s, error\n",__func__);
        }
        log_debug("%s\n",__func__);
    }
}


int vmup_port_send_packet_rev_msg(sys_msg_com_data_t *msg)
{
    sys_msg_t send_msg;
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    send_msg.msg_type = SYS_MSG_TYPE_COM;
    if(sizeof(sys_msg_com_data_t) > sizeof(send_msg.msg_data))
    {
        CI_ASSERT(0,"\n");
    }                             
    MASK_ROM_LIB_FUNC->newlibcfunc.memcpy_p((uint8_t *)(&send_msg.msg_data), msg, sizeof(sys_msg_com_data_t));
    
    xResult = send_msg_to_sys_task(&send_msg,&xHigherPriorityTaskWoken);

    if((xResult != pdFAIL)&&(pdTRUE == xHigherPriorityTaskWoken))
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    return xHigherPriorityTaskWoken;
}


TickType_t last_time;
static bool vmup_port_timeout_one_packet(void)
{
    TickType_t now_time;
    TickType_t timeout;
    
    now_time = xTaskGetTickCountFromISR();

    timeout = (now_time - last_time);/*uint type, so overflow just used - */

    last_time = now_time;

    if(timeout > TIMEOUT_ONE_PACKET_INTERVAL/portTICK_PERIOD_MS) /*also as timeout = timeout*portTICK_PERIOD_MS;*/
    {
        return true;
    }
    else
    {
        return false;
    }
}


static uint16_t vmup_port_checksum(uint16_t init_val, uint8_t * data, uint16_t length)
{
    uint32_t i;
    uint16_t chk_sum = init_val;
    for(i=0; i<length; i++)
    {
        chk_sum += data[i];
    }

    return chk_sum;
}


static int vmup_port_send_char(uint8_t ch)
{
    UartPollingSenddata((UART_TypeDef *)UART_PROTOCOL_NUMBER, ch);
    return RETURN_OK;
}

static void vump_uart_irq_handler(void)
{
    /*发送数据*/
    if (((UART_TypeDef*)UART_PROTOCOL_NUMBER)->UARTMIS & (1UL << UART_TXInt))
    {
        UART_IntClear((UART_TypeDef*)UART_PROTOCOL_NUMBER,UART_TXInt);
    }
    /*接受数据*/
    if (((UART_TypeDef*)UART_PROTOCOL_NUMBER)->UARTMIS & (1UL << UART_RXInt))
    {
        //here FIFO DATA must be read out
        //vmup_receive_packet(UART_RXDATA((UART_TypeDef*)UART_PROTOCOL_NUMBER));
        UART_IntClear((UART_TypeDef*)UART_PROTOCOL_NUMBER,UART_RXInt);
    }

    UART_IntClear((UART_TypeDef*)UART_PROTOCOL_NUMBER,UART_AllInt);
}


static int vmup_port_hw_init(void)
{
#if HAL_UART0_BASE == UART_PROTOCOL_NUMBER
    __eclic_irq_set_vector(UART0_IRQn, (int32_t)vump_uart_irq_handler);
#elif (HAL_UART1_BASE == UART_PROTOCOL_NUMBER)
    __eclic_irq_set_vector(UART1_IRQn, (int32_t)vump_uart_irq_handler);
#else
    __eclic_irq_set_vector(UART2_IRQn, (int32_t)vump_uart_irq_handler);
#endif
    UARTInterruptConfig((UART_TypeDef *)UART_PROTOCOL_NUMBER, UART_PROTOCOL_BAUDRATE);
    return RETURN_OK;
}


/********************************************************************
                     send function
********************************************************************/
static int vmup_send_packet(sys_msg_com_data_t * msg)
{
    uint16_t chk_sum;
    uint8_t *buf = (uint8_t*)msg;
    int i;
    
    if(msg == NULL)
    {
        return RETURN_ERR;
    }        

    vmup_port_mutex_take();
        
    /*header and data*/    
    for(i = 0;i < sizeof(sys_msg_com_data_t); i++)
    {
        vmup_port_send_char( buf[i] );
    }

    /*check sum*/
    chk_sum = vmup_port_checksum(0, (uint8_t*)&msg->data_length, 4);
    vmup_port_send_char( chk_sum&0xff );

    vmup_port_mutex_give();

    return RETURN_OK;
}

#if UART_BAUDRATE_CALIBRATE
void send_baudrate_sync_req(void)
{
    sys_msg_com_data_t msg;
    uint32_t *sendid = (uint32_t *)msg.msg_data;

    memset(msg.msg_data,0,8);
    msg.header = VMUP_MSG_HEAD;
    msg.data_length = 7;
    msg.msg_type = VMUP_MSG_TYPE_CMD_UP;
    msg.msg_cmd = VMUP_MSG_CMD_ASR_RESULT;
    msg.msg_seq = glb_send_sequence++;
    
    *sendid = baud_sync_req;
    msg.msg_data[6] = 0;

    vmup_send_packet(&msg);
}
#endif


void vmup_communicate_init(void)
{
    vmup_port_hw_init();
    vmup_port_mutex_creat();

    #if UART_BAUDRATE_CALIBRATE
    cmd_handle_t cmd_handle = cmd_info_find_command_by_string("<baud_sync_req>");
    baud_sync_req = cmd_info_get_command_id(cmd_handle);
    ci_loginfo(CI_LOG_INFO, "baud_sync_req:%08x\n", baud_sync_req);

    baudrate_calibrate_init(UART_PROTOCOL_NUMBER, UART_PROTOCOL_BAUDRATE, send_baudrate_sync_req);          // 初始化波特率校准
    baudrate_calibrate_start();         // 启动一次波特率校准
    // baudrate_calibrate_wait_finish();   // 等待波特率校准完成
    #endif
}





void vmup_send_asr_result_cmd(cmd_handle_t cmd_handle)
{
    sys_msg_com_data_t msg;
    uint8_t cmd_id;

    msg.header = VMUP_MSG_HEAD;
    msg.data_length = 0x1;
    msg.msg_type = 0x0;
    
    cmd_id = cmd_info_get_command_id(cmd_handle);
    switch (cmd_id)
    {

    case 2:   //验证浴霸产品
        msg.msg_cmd = 0x65;
        msg.msg_data = 0x01;
        vmup_send_packet(&msg);
        break;

    case 3:   //验证风扇灯产品
        msg.msg_cmd = 0xC9;
        msg.msg_data = 0x01;
        vmup_send_packet(&msg);
        break;
    case 4:   //验证厨卫灯产品
        msg.msg_cmd = 0xFF;
        msg.msg_data = 0x00;
        vmup_send_packet(&msg);
        break;
    
    default:
        break;
    }


}


void vmup_send_key_cmd(uint8_t cmd_id)
{
    sys_msg_com_data_t msg;

    msg.header = VMUP_MSG_HEAD;
    msg.data_length = 0x1;
    msg.msg_type = 0x0;
    
     switch (cmd_id)
    {

    case 2:   //验证浴霸产品
        msg.msg_cmd = 0x65;
        msg.msg_data = 0x01;
        vmup_send_packet(&msg);
        break;

    case 3:   //验证风扇灯产品
        msg.msg_cmd = 0xC9;
        msg.msg_data = 0x01;
        vmup_send_packet(&msg);
        break;
    case 4:   //验证厨卫灯产品
        msg.msg_cmd = 0xFF;
        msg.msg_data = 0x00;
        vmup_send_packet(&msg);
        break;
    
    default:
        break;
    }




}



/********************************************************************
                     user add function
********************************************************************/

void play_voice_callback(cmd_handle_t cmd_handle)
{
    resume_voice_in();
}



#endif //(UART_PROTOCOL_VER == 2)
