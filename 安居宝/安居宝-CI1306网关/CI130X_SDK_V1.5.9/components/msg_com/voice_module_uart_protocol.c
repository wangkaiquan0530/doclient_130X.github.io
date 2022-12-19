


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


#define VMUP_PACKET_MIN_SIZE (8)
#define VMUP_PACKET_MAX_SIZE (VMUP_MSG_DATA_MAX_SIZE + VMUP_PACKET_MIN_SIZE)


extern void  resume_voice_in(void);
extern void  pause_voice_in(void);


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
        vmup_receive_packet(UART_RXDATA((UART_TypeDef*)UART_PROTOCOL_NUMBER));
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
                     receive function
********************************************************************/
/*receive use state machine method, so two char can arrive different time*/
typedef enum 
{
    REV_STATE_HEAD0   = 0x00,
    REV_STATE_ONE     = 0x01,
    REV_STATE_TWO     = 0x02,
    REV_STATE_THREE   = 0x03,
    REV_STATE_MSG     = 0x04,
    REV_STATE_FOUR    = 0x05,
    REV_STATE_FIVE    = 0x06,
    REV_STATE_SIX     = 0x07,        
}vmup_receive_state_t;

sys_msg_com_data_t recever_packet;
static uint8_t rev_state = REV_STATE_HEAD0;


void vmup_receive_packet(uint8_t receive_char)
{
    if(true == vmup_port_timeout_one_packet())
    {
        rev_state = REV_STATE_HEAD0;
    }

    switch(rev_state)
    {
        case REV_STATE_HEAD0:
            if(VMUP_MSG_HEAD == receive_char)
            {
                rev_state = REV_STATE_ONE;
                recever_packet.header = VMUP_MSG_HEAD;
            }
            else
            {
                rev_state = REV_STATE_HEAD0;

            }
            break;

        case REV_STATE_ONE:
            if(VMUP_DATA_ONE == receive_char)
            {
                rev_state = REV_STATE_TWO;
                recever_packet.data_one = VMUP_DATA_ONE;
            }
            else
            {
                rev_state = REV_STATE_HEAD0;

            }
            break;

        case REV_STATE_TWO:
            if(VMUP_DATA_TWO == receive_char)
            {
                rev_state = REV_STATE_THREE;
                recever_packet.data_two = REV_STATE_TWO;
            }
            else
            {
                rev_state = REV_STATE_HEAD0;

            }
            break;

        case REV_STATE_THREE:
            if(VMUP_DATA_THREE == receive_char)
            {
                rev_state = REV_STATE_MSG;
                recever_packet.data_three = VMUP_DATA_THREE;
            }
            else
            {
                rev_state = REV_STATE_HEAD0;

            }
            break;

         case REV_STATE_MSG:
                rev_state = REV_STATE_FOUR;
                recever_packet.data_msg = receive_char;
            break;  

        case REV_STATE_FOUR:
            if(VMUP_DATA_FOUR == receive_char)
            {
                rev_state = REV_STATE_FIVE;
                recever_packet.data_four = VMUP_DATA_FOUR;
            }
            else
            {
                rev_state = REV_STATE_HEAD0;

            }
            break;

        case REV_STATE_FIVE:
            if(VMUP_DATA_FIVE == receive_char)
            {
                rev_state = VMUP_DATA_SIX;
                recever_packet.data_five = VMUP_DATA_FIVE;
            }
            else
            {
                rev_state = REV_STATE_HEAD0;

            }
            break;

        case VMUP_DATA_SIX:
            if(VMUP_DATA_SIX == receive_char)
            {
                rev_state = REV_STATE_HEAD0;
                recever_packet.data_six = VMUP_DATA_SIX;
                vmup_port_send_packet_rev_msg(&recever_packet);
            }
            break;

        default:
            rev_state = REV_STATE_HEAD0;
            break;
    }
    
}


/********************************************************************
                     send function
********************************************************************/
static int vmup_send_packet(sys_msg_com_data_t * msg)
{
    uint8_t *buf = (uint8_t*)msg;
    int i;
    
    if(msg == NULL)
    {
        return RETURN_ERR;
    }        

    vmup_port_mutex_take();
        
    /*header and data*/    
    for(i = 0;i < 8; i++)
    {
        vmup_port_send_char( buf[i] );
    }

    vmup_port_mutex_give();

    return RETURN_OK;
}




void vmup_communicate_init(void)
{
    vmup_port_hw_init();
    vmup_port_mutex_creat();

}


     
/********************************************************************
                     user add function
********************************************************************/

void play_voice_callback(cmd_handle_t cmd_handle)
{
    resume_voice_in();
}

void vmup_send_asr_result_cmd(cmd_handle_t cmd_handle)
{
    sys_msg_com_data_t msg;

    msg.header = VMUP_MSG_HEAD;
    msg.data_one = VMUP_DATA_ONE;
    msg.data_two = VMUP_DATA_TWO;
    msg.data_three = VMUP_DATA_THREE;
    msg.data_msg = cmd_info_get_command_id(cmd_handle);
    msg.data_four = VMUP_DATA_FOUR;
    msg.data_five = VMUP_DATA_FIVE;
    msg.data_six = VMUP_DATA_SIX;

    vmup_send_packet(&msg);
}

void vmup_send_notify(uint8_t notify_event)
{
    sys_msg_com_data_t msg;

    msg.header = VMUP_MSG_HEAD;
    msg.data_one = VMUP_DATA_ONE;
    msg.data_two = VMUP_DATA_TWO;
    msg.data_three = VMUP_DATA_THREE;
    msg.data_msg = notify_event;
    msg.data_four = VMUP_DATA_FOUR;
    msg.data_five = VMUP_DATA_FIVE;
    msg.data_six = VMUP_DATA_SIX;

    vmup_send_packet(&msg);
}

void userapp_deal_cmd(sys_msg_com_data_t *msg)
{


}


void userapp_deal_com_msg(sys_msg_com_data_t *msg)
{    
    int select_index = -1;
    uint8_t vol;
    switch(msg->data_msg)
    {
        case	0x6B:
        case	0x6C:
        case	0x6D:
        case	0x6E:
        case	0x6F:
        case	0x70:
        case	0x71:
        case	0x72:
        case	0x73:
        case	0x74:
        case	0x75:
        case	0x76:
        case	0x77:
        case	0x78:
        case	0x79:
        case	0x7A:
        case	0x7B:
        case	0x7C:
        case	0x7D:
        case	0x7E:
        case	0x7F:
        case	0x80:
        case	0x81:
        case	0x82:
        case	0x83:
        case	0x84:
        case	0x85:
        case	0x86:
        case	0x87:
        case	0x88:
        case	0x89:
        case	0x8A:
        case	0x8B:
        case	0x8C:
        case	0x8D:
        case	0x8E:
        case	0x8F:
        case	0x90:
        case	0x91:
        case	0x92:
        case	0x93:
        case	0x94:
        case	0x95:
        case	0x96:
        case	0x97:
        case	0x98:
        case	0x99:
        case	0x9A:
        case	0x9B:
        case	0x9C:
        case	0x9D:
        case	0x9E:
        case	0x9F:
        case	0xA0:
        case	0xA1:
        case	0xA2:
        case	0xA3:
        case	0xA4:
        case	0xA5:
        case	0xA6:
        case	0xA7:
        case	0xA8:
        case	0xA9:
        case	0xAA:
        case	0xAB:
        case	0xAC:
        case	0xAD:
        case	0xAE:
        case	0xAF:
        case	0xB0:
        case	0xB1:
        case	0xB2:
        case	0xB3:
        case	0xB4:
        case	0xB5:
        case	0xB6:
        case	0xB7:
        case	0xB8:
        case	0xB9:
        case	0xBA:
        case	0xBB:
        case	0xBC:
        case	0xBD:
        case	0xBE:

        case	0xC2:
        case	0xC3:
        case	0xC4:
        case	0xC5:
        case	0xC6:
        case	0xC7:
        case	0xC8:
        case	0xC9:
        case	0xCA:
        case	0xCB:
        case	0xCC:
        case	0xCD:
        case	0xCE:
        case	0xCF:
        case	0xD0:
        case	0xD1:
  
            pause_voice_in();
            prompt_play_by_cmd_id(msg->data_msg,-1,play_voice_callback,true);  

        break;
        
        case	0xBF://增大音量   
            vol = vol_set(vol_get() + 1);
            select_index = (vol == VOLUME_MAX) ? 1:0;
            pause_voice_in();
            prompt_play_by_cmd_id(msg->data_msg,select_index,play_voice_callback,true); 
        break;  

        case	0xC0://减小音量
            vol = vol_set(vol_get() - 1);
            select_index = (vol == VOLUME_MIN) ? 1:0;
            pause_voice_in();
            prompt_play_by_cmd_id(msg->data_msg,select_index,play_voice_callback,true); 
        break;

        case	0xC1://打开播报音
            prompt_player_enable(ENABLE);
            pause_voice_in();
            prompt_play_by_cmd_id(msg->data_msg,-1,play_voice_callback,true); 
        break;

        case	0xD2://关闭语音播报
            prompt_play_by_cmd_id(msg->data_msg,-1,play_voice_callback,true); 
            pause_voice_in();
            prompt_player_enable(DISABLE);
        break;

        default:
            break;

        
    }
}


#endif //(UART_PROTOCOL_VER == 2)
