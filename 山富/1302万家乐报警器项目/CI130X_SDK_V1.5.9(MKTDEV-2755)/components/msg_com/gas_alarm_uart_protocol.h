

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#include "ci130x_uart.h"
#include "command_info.h"
#include "sdk_default_config.h"


#ifndef _GAS_ALARM_UART_PROTOCOL_
#define _GAS_ALARM_UART_PROTOCOL_


#ifdef __cplusplus
extern "C"
{
#endif


#define GAS_ALARMPROTOCOL_VERSION                1
#define GAS_ALARMPROTOCOL_SUBVERSION             0
#define GAS_ALARMPROTOCOL_REVISION               0


#define GAS_ALARMMSG_DATA_MAX_SIZE (20)

#pragma pack(1)
typedef struct
{
    uint8_t header;//0xAA
    uint16_t data_length;//ver到chksum长度
    uint8_t msg_ver;//协议版本号
    uint8_t msg_cmd;//帧类型
    uint8_t msg_payload;//有效数据
    uint8_t chksum;
}sys_gas_alarm_com_data_t;


#pragma pack()

/*header*/
#define GAS_ALARMMSG_HEAD   (0xAA)

/*msg_ver*/
#define GAS_ALARMMSG_TYPE_CMD_UP   (0x10)//协议版本号

/*msg_cmd*/
#define GAS_ALARMMSG_CO_CMD_ASR_RESULT    (0x70)           //CO帧类型
#define GAS_ALARMMSG_CH4_CMD_ASR_RESULT   (0x71)           //CH4帧类型
#define GAS_ALARMMSG_FAULT_CMD_ASR_RESULT (0x72)           //FAULT帧类型


void gas_alarm_receive_packet(uint8_t receive_char);
void gas_alarm_communicate_init(void);
void userapp_gas_alarm_deal_com_msg(sys_gas_alarm_com_data_t *com_data);



#ifdef __cplusplus
}
#endif


#endif

