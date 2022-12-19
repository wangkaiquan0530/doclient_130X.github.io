

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
    uint8_t header;
    uint8_t data_one;
    uint8_t data_two;
    uint8_t data_three;
    uint8_t data_msg;
    uint8_t data_four;
    uint8_t data_five;
    uint8_t data_six;
}sys_msg_com_data_t;


#pragma pack()

/*header*/
#define VMUP_MSG_HEAD   (0xA5)

/*data*/
#define VMUP_DATA_ONE   (0xFA)
#define VMUP_DATA_TWO   (0x00)
#define VMUP_DATA_THREE (0x03)
#define VMUP_DATA_FOUR  (0x00)
#define VMUP_DATA_FIVE  (0x00)
#define VMUP_DATA_SIX   (0xFB)

#define VMUP_DATA_POWERON   (0xC2)
#define VMUP_DATA_SLEEP     (0xC3)
#define VMUP_DATA_WAKEUP    (0x6B)


void vmup_receive_packet(uint8_t receive_char);
void vmup_communicate_init(void);
void userapp_deal_cmd(sys_msg_com_data_t *msg);
void userapp_deal_com_msg(sys_msg_com_data_t *msg);
void vmup_send_asr_result_cmd(cmd_handle_t cmd_handle);
void vmup_send_notify(uint8_t notify_event);


#ifdef __cplusplus
}
#endif

#endif //#if (UART_PROTOCOL_VER == 2)

#endif

