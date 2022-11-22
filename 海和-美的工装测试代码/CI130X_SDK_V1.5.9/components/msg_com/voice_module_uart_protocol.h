

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#include "ci130x_uart.h"
#include "command_info.h"
#include "sdk_default_config.h"


#ifndef _VOICE_MODULE_UART_PROTOCOL_
#define _VOICE_MODULE_UART_PROTOCOL_

#if (UART_PROTOCOL_VER == 2)

#ifdef __cplusplus
extern "C"
{
#endif


#pragma pack(1)
typedef struct
{
    uint16_t header;
    uint8_t data_length;
    uint8_t msg_type;
    uint8_t msg_cmd;
    uint8_t msg_data;
    /*uint16_t chksum; send add auto*/
    /*uint8_t tail; send add auto*/
}sys_msg_com_data_t;


#pragma pack()

/*header*/
#define VMUP_MSG_HEAD_LOW  (0xA5)
#define VMUP_MSG_HEAD_HIGH (0xFA)
#define VMUP_MSG_HEAD   ((VMUP_MSG_HEAD_HIGH<<8)|VMUP_MSG_HEAD_LOW)


//void vmup_receive_packet(uint8_t receive_char);
void vmup_communicate_init(void);
void vmup_send_notify(uint8_t notify_event);
void userapp_deal_cmd(sys_msg_com_data_t *msg);
void userapp_deal_com_msg(sys_msg_com_data_t *msg);
//void userapp_send_com_test(void);
void vmup_send_asr_result_cmd(cmd_handle_t cmd_handle);
void vmup_send_key_cmd(uint8_t cmd_id);



#ifdef __cplusplus
}
#endif

#endif //#if (UART_PROTOCOL_VER == 2)

#endif

