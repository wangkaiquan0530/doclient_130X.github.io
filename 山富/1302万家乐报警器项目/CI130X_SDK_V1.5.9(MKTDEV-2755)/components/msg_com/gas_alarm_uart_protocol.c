


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "sdk_default_config.h"

#include "gas_alarm_uart_protocol.h"

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
#include "user_config.h"
#include "voice_module_uart_protocol.h"



#define TIMEOUT_ONE_PACKET_INTERVAL (1000)/*ms, in this code, it should be bigger than portTICK_PERIOD_MS */

extern void  resume_voice_in(void);
extern void  pause_voice_in(void);
extern void set_state_enter_wakeup(uint32_t exit_wakup_ms);


/********************************************************************
                     port function
********************************************************************/
static SemaphoreHandle_t send_gas_packet_mutex;

#define log_debug(fmt, args...) do{}while(0)
#if USE_STD_PRINTF
#define log_info printf
#define log_error printf
#else
#define log_info _printf
#define log_error _printf
#endif
#define cinv_assert(x) do{}while(0)


static void gas_alarm_mutex_creat(void)
{
    send_gas_packet_mutex = xSemaphoreCreateMutex();

    if (NULL == send_gas_packet_mutex) 
    {
        log_error("%s, error\n",__func__);
    }
    log_debug("%s\n",__func__);
}


static void gas_alarm_mutex_take(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) 
    {
        if (xSemaphoreTake(send_gas_packet_mutex, portMAX_DELAY) == pdFALSE)
        {
            log_error("%s, error\n",__func__);
        }
        log_debug("%s\n",__func__);
    }
}


static void gas_alarm_mutex_give(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) 
    {
        if (xSemaphoreGive(send_gas_packet_mutex) == pdFALSE) 
        {
            log_error("%s, error\n",__func__);
        }
        log_debug("%s\n",__func__);
    }
}


int gas_alarm_send_packet_rev_msg(sys_gas_alarm_com_data_t *msg)
{
    sys_msg_t send_msg;
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    send_msg.msg_type = SYS_MSG_TYPE_GAS_ALARM_COM;
    if(sizeof(sys_gas_alarm_com_data_t) > sizeof(send_msg.msg_data))
    {
        CI_ASSERT(0,"\n");
    }                             
    MASK_ROM_LIB_FUNC->newlibcfunc.memcpy_p((uint8_t *)(&send_msg.msg_data), msg, sizeof(sys_gas_alarm_com_data_t));
    
    xResult = send_msg_to_sys_task(&send_msg,&xHigherPriorityTaskWoken);

    if((xResult != pdFAIL)&&(pdTRUE == xHigherPriorityTaskWoken))
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    return xHigherPriorityTaskWoken;
}


TickType_t gas_last_time;
static bool gas_alarm_timeout_one_packet(void)
{
    TickType_t now_time;
    TickType_t timeout;
    
    now_time = xTaskGetTickCountFromISR();

    timeout = (now_time - gas_last_time);/*uint type, so overflow just used - */

    gas_last_time = now_time;

    if(timeout > TIMEOUT_ONE_PACKET_INTERVAL/portTICK_PERIOD_MS) /*also as timeout = timeout*portTICK_PERIOD_MS;*/
    {
        return true;
    }
    else
    {
        return false;
    }
}


static uint16_t gas_alarm_checksum(uint16_t init_val, uint8_t * data, uint16_t length)
{
    uint32_t i;
    uint16_t chk_sum = init_val;
    for(i=0; i<length; i++)
    {
        chk_sum += data[i];
    }

    return chk_sum;
}


static int gas_alarm_send_char(uint8_t ch)
{
    UartPollingSenddata((UART_TypeDef *)UART_GAS_ALARM_PROTOCOL_NUMBER, ch);
    return RETURN_OK;
}

static void gas_alarmuart_irq_handler(void)
{
    /*发送数据*/
    if (((UART_TypeDef*)UART_GAS_ALARM_PROTOCOL_NUMBER)->UARTMIS & (1UL << UART_TXInt))
    {
        UART_IntClear((UART_TypeDef*)UART_GAS_ALARM_PROTOCOL_NUMBER,UART_TXInt);
    }
    /*接受数据*/
    if (((UART_TypeDef*)UART_GAS_ALARM_PROTOCOL_NUMBER)->UARTMIS & (1UL << UART_RXInt))
    {
        //here FIFO DATA must be read out
        gas_alarm_receive_packet(UART_RXDATA((UART_TypeDef*)UART_GAS_ALARM_PROTOCOL_NUMBER));
        UART_IntClear((UART_TypeDef*)UART_GAS_ALARM_PROTOCOL_NUMBER,UART_RXInt);
    }

    UART_IntClear((UART_TypeDef*)UART_GAS_ALARM_PROTOCOL_NUMBER,UART_AllInt);
}


static int gas_alarm_hw_init(void)
{
    __eclic_irq_set_vector(UART2_IRQn, (int32_t)gas_alarmuart_irq_handler);
    UARTInterruptConfig((UART_TypeDef *)UART_GAS_ALARM_PROTOCOL_NUMBER, UART_PROTOCOL_BAUDRATE);
    return RETURN_OK;
}

/********************************************************************
                     receive function
********************************************************************/
/*receive use state machine method, so two char can arrive different time*/
typedef enum 
{
    REV_GAS_HEAD0   = 0x00,
    REV_GAS_LENGTH0 = 0x01,
    REV_GAS_LENGTH1 = 0x02,
    REV_GAS_VER     = 0x03,
    REV_GAS_CMD     = 0x04,
    REV_GAS_PAYLOAD = 0x05,
    REV_GAS_CKSUM   = 0x06,
}gas_alarm_receive_state_t;

sys_gas_alarm_com_data_t gas_recv_packet;
static uint8_t rev_state = REV_GAS_HEAD0;
static uint16_t length0 = 0, length1 = 0;
static uint8_t chk_sum=0;


void gas_alarm_receive_packet(uint8_t receive_char)
{
    if(true == gas_alarm_timeout_one_packet())
    {
        rev_state = REV_GAS_HEAD0;
    }

    switch(rev_state)
    {
        case REV_GAS_HEAD0:
            if(GAS_ALARMMSG_HEAD == receive_char)
            {
                gas_recv_packet.header = receive_char;
                rev_state = REV_GAS_LENGTH0;
            }
            else
            {
                rev_state = REV_GAS_HEAD0;
            }
            break;
        case REV_GAS_LENGTH0:
            length0 = receive_char;
            rev_state = REV_GAS_LENGTH1;
            break;
        case REV_GAS_LENGTH1:
            length1 = receive_char;
            length0 <<= 8;
            length0 += length1;
            gas_recv_packet.data_length = length0;
            rev_state = REV_GAS_VER;
            break;
        case REV_GAS_VER:
            gas_recv_packet.msg_ver = receive_char;
            rev_state = REV_GAS_CMD;
            break;
        case REV_GAS_CMD:
            gas_recv_packet.msg_cmd = receive_char;
            rev_state = REV_GAS_PAYLOAD;
            break;
        case REV_GAS_PAYLOAD:
            gas_recv_packet.msg_payload = receive_char;
            rev_state = REV_GAS_CKSUM;
            break;
        case REV_GAS_CKSUM:
        {
            uint16_t packet_chk_sum;
            uint8_t temp_chk_sum=0;
            chk_sum = receive_char;
            packet_chk_sum = gas_alarm_checksum(0, (uint8_t *)&gas_recv_packet.header, 6);   
            temp_chk_sum= packet_chk_sum & 0xFF;

            if(chk_sum == temp_chk_sum)
            {
                gas_recv_packet.chksum = receive_char;
                gas_alarm_send_packet_rev_msg(&gas_recv_packet);
            }
            else
            {
                rev_state = REV_GAS_HEAD0;
            }
            break;
        }

        default:
            rev_state = REV_GAS_HEAD0;
            break;
    }
    
}



void gas_alarm_communicate_init(void)
{
    gas_alarm_hw_init();
    gas_alarm_mutex_creat();

}


/*****************************************************************************
 Prototype    : uart_send_Play_FeedBack
 Description  : 串口接收事件回传到外系统
*****************************************************************************/
void uart_send_Play_FeedBack(sys_gas_alarm_com_data_t *com_data)
{

    uint8_t *buf = (uint8_t*)(com_data);
    uint8_t  num = 0;

    if(buf != NULL)
    {
        gas_alarm_mutex_take();
        num = sizeof(sys_gas_alarm_com_data_t);
        ci_loginfo(LOG_USER,"uart  2 send \r\n");
        for(int i = 0;i < num; i++)
        {
            if(1 == i)
            {
                gas_alarm_send_char( buf[2] );
            }
            else if(2 == i)
            {
                gas_alarm_send_char( buf[1] );
            }
            else
            {
                gas_alarm_send_char( buf[i] );
            }

        }
        gas_alarm_mutex_give();
    } 
} 

void userapp_gas_alarm_deal_com_msg(sys_gas_alarm_com_data_t *msg)
{    
    gas_alarm_com_send(UART_HEADER_CON,msg->msg_payload,0);
    uart_send_Play_FeedBack(msg);//原样数据返回
}


