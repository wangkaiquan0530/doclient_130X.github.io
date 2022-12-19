

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#include "ci130x_uart.h"
#include "command_info.h"
#include "sdk_default_config.h"
#include "user_config.h"

#ifndef _VOICE_MODULE_UART_PROTOCOL_
#define _VOICE_MODULE_UART_PROTOCOL_

#ifdef __cplusplus
extern "C"
{
#endif


#define VMUP_PROTOCOL_VERSION       1
#define VMUP_PROTOCOL_SUBVERSION    0
#define VMUP_PROTOCOL_REVISION      0

//版本9 芯片迭代升级
#define Prog_Version_Info 			9
//WJL GAS protocol content  
#define PLAY_INDEX_MIN	          (0)
#define PLAY_INDEX_MAX	          (84)

#define PLAY_INDEX_ADD_CMD_0	  (58)//紧急呼叫
#define PLAY_INDEX_ADD_CMD_1	  (59)//语音导航
  
#define VOLUME_MAX_COMMAND_INDEX  (56)
#define VOLUME_MIN_COMMAND_INDEX  (57)
#define PLAY_VOLMAX_VOICE_INDEX   (56)
#define PLAY_VOLMIN_VOICE_INDEX   (57)
 
#define UART_HEADER_CON	          (0x42)
#define UART_HEADER_NON		      (0x52)

#define UART_CODE_ASR             (0x03)
#define UART_CODE_PLAY		      (0x03)
#define UART_CODE_PLAY_FEEDBACK	  (0x44)

#define UART_DATA_ID		      (0x0001)
#define UART_DATA_TOKEN	          (0xBC90)
#define UART_DATA_PAYLOAD	      (0xFF)

#define PATH_AWAKEN		 	      (0xb3312f31)
#define PATH_OPEN_CLOSE 		  (0xb3312f32)
#define PATH_TEMP_CONTROL		  (0xb3312f33)
#define PATH_MODE_CONTROL 	      (0xb3312f34)
#define PATH_VOLUME_CONTROL	      (0xb3312f35)
#define PATH_URGENT_CONTROL	      (0xb3312f36)
#define PATH_CO_ALARM_CONTROL	  (0xb3312f38)//CO 浓度报警功能
#define PATH_CH4_ALARM_CONTROL	  (0xb3312f39)//甲烷浓度报警功能
#define PATH_DOUBLE_ALARM_STATUS  (0xb3312f40)//双气报警器状态


#define PATH_SMART_CONTROL	      (0xb3312f37) //add 20220731 智慧浴功能

#define PATH_VERSION_QUERY        (0xb3312f38)
#define UART_DATA_END             0xFB
#define NOT_CATCH_CMD_WORD_ID     (59)
#define VERSION_QUERY_INDEX       0xAA
#define TEST_QUERY_INDEX          0x99
  

#define VMUP_MSG_DATA_MAX_SIZE    (8)

#pragma pack(1)

typedef struct
{
    unsigned char header0;//header
    unsigned char code;
    unsigned char id_H;
    unsigned char id_L;
    unsigned char token_H;
    unsigned char token_L;
    unsigned char path_3;
    unsigned char path_2;
    unsigned char path_1;
    unsigned char path_0;
    unsigned char payload;
    unsigned char Cmd_H;
    unsigned char Cmd_L;
    unsigned char end;	
}sys_com_msg_data_t;

typedef enum{
    AWAKEN_INDEX_CMD 				= 0x02,//0x02=0,
                                                 
    ON_CONTROL_INDEX_START 			= 0x03,//0x03=1,
    ON_CONTROL_INDEX_END 			= 0x03,//0x03=3,
                                                 
    OFF_CONTROL_INDEX_START 		= 0x04,//0x04=4,
    OFF_CONTROL_INDEX_END 			= 0x04,//0x04=5,
                                                 
    INCREASE_TEMP_INDEX_START 		= 0x05,//0x05=6,
    INCREASE_TEMP_INDEX_END   		= 0x05,//0x05=7,
    DECREASE_TEMP_INDEX_START  		= 0x06,//0x06=8,
    DECREASE_TEMP_INDEX_END    		= 0x06,//0x06=9,
                                                 
    WATER_36_DEGREES_INDEX_START  	= 0x07,//0x07=10,
    WATER_48_DEGREES_INDEX_END   	= 0x13,//0x13=35,
                                                 
    OPEN_BOOKING_INDEX_CMD	 		= 0x18,//0x18=36,
    CLOSE_BOOKING_INDEX_CMD 		= 0x19,//0x19=37,
                                                 
    OPEN_KEEP_WARM_INDEX_CMD 		= 0x1A,//0x1A=38,
    CLOSE_KEEP_WARM_INDEX_CMD 		= 0x1B,//0x1B=39,
                                                 
    OPEN_THE_HOT_INDEX_CMD 			= 0x1C,//0x1C=40,
    CLOSE_THE_HOT_INDEX_CMD 		= 0x1D,//0x1D=41,
                                                 
    OPEN_TEMP_SENSE_INDEX_CMD 		= 0x1F,//0x1F=42,
    CLOSE_TEMP_SENSE_INDEX_CMD 		= 0x20,//0x20=43,
                                                 
    OPEN_SUPER_CHARGE_INDEX_CMD 	= 0x21,//0x21=44,
    CLOSE_SUPER_CHARGE_INDEX_CMD 	= 0x22,//0x22=45,
    //                                           
    INCREASE_VOICE_INDEX_CMD 		= 0x23,//0x23=49,
    DECREASE_VOCE_INDEX_CMD 		= 0x24,//0x24=53,
                                                 
    MAX_VOICE_INDEX_CMD 			= 0x25,//0x25=56,
    MIN_VOICE_INDEX_CMD 			= 0x26,//0x26=57,
    //                                           
    EMERGENCY_CALL_INDEX_CMD 		= 0x27,//0x27=58,
    AUDIO_GUIDE_INDEX_CMD           = 0x28,//0x28=58,
                                                 
	OPEN_COOL_WATER_INDEX_CMD	    = 0x2E,//0x2E=
	CLOSE_COOL_WATER_INDEX_CMD      = 0x2F,//0x2F=

	OPEN_TWO_SUPER_CHARGE_INDEX_CMD    =0x30, //=0x30双增压
	CLOSE_TWO_SUPER_CHARGE_INDEX_CMD   =0x31, //=0x31双增压

	//OPEN_SIG_COOL_WATER_INDEX_CMD	    = 0x30,//= 0x30//开启单次零冷水  //add 20220731 
	//CLOSE_SIG_COOL_WATER_INDEX_CMD      = 0x31,//= 0x31//关闭单次零冷水
                                                       
	OPEN_SIG_THE_HOT_INDEX_CMD	        = 0x32,//= 0x32//开启单次即热模式/单次/打开单次
	CLOSE_SIG_THE_HOT_INDEX_CMD         = 0x33,//= 0x33//关闭单次即热模式/关闭单次
                                                       
	OPEN_DISINFECT_INDEX_CMD	        = 0x34,//= 0x34//打开杀菌
	CLOSE_DISINFECT_INDEX_CMD           = 0x35,//= 0x35//关闭杀菌
                                                       
    OPEN_KITCHEN_CLEAN_INDEX_CMD        = 0X36,//= 0X36//开启厨房洗
    CLOSE_KITCHEN_CLEAN_INDEX_CMD       = 0X37,//= 0X37//关闭厨房洗
                                                       
    OPEN_CHILDREN_CLEAN_INDEX_CMD       = 0X38,//= 0X38//开启儿童浴
    CLOSE_CHILDREN_CLEAN_INDEX_CMD      = 0X39,//= 0X39//关闭儿童浴
                                                       
    OPEN_THEOLD_CLEAN_INDEX_CMD         = 0X3A,//= 0X3A//开启老人浴
    CLOSE_THEOLD_CLEAN_INDEX_CMD        = 0X3B,//= 0X3B//关闭老人浴
                                                       
    OPEN_COMFORT_CLEAN_INDEX_CMD        = 0X3C,//= 0X3C//开启舒适浴
    CLOSE_COMFORT_CLEAN_INDEX_CMD       = 0X3D,//= 0X3D//关闭舒适浴    //add 20220731

	OPEN_SIG_COOL_WATER_INDEX_CMD	    = 0x3E,//= 0x30//开启单次零冷水  //add 20220810
	CLOSE_SIG_COOL_WATER_INDEX_CMD      = 0x3F //= 0x31//关闭单次零冷水

}INDEX_CMD;

//LME 命令词定义
typedef enum{
    WAKEUP_ID = 0X02,
  
    POWER_ON_ID,
    POWER_OFF_ID,
    
    INCREASE_TEMP_ID,
    DOWN_TEMP_ID,
	
    /*temp regulation*/
    WATER_36_DEGREES_ID,
    WATER_37_DEGREES_ID,
    WATER_38_DEGREES_ID,
    WATER_39_DEGREES_ID,
    WATER_40_DEGREES_ID,
    WATER_41_DEGREES_ID,
    WATER_42_DEGREES_ID,
    WATER_43_DEGREES_ID,
    WATER_44_DEGREES_ID,
    WATER_45_DEGREES_ID,
    WATER_46_DEGREES_ID,
    WATER_47_DEGREES_ID,
    WATER_48_DEGREES_ID 	= 0x13,
    
    OPEN_BOOKING_ID 		= 0x18,
    CLOSE_BOOKING_ID,
    
    OPEN_KEEP_WARM_ID,
    CLOSE_KEEP_WARM_ID,
    
    OPEN_THE_HOT_ID,
    CLOSE_THE_HOT_ID,
    					    //无0X1e,
    OPEN_TEMP_SENSE_ID      = 0x1f,
    CLOSE_TEMP_SENSE_ID,
    
    OPEN_SUPER_CHARGE_ID,
    CLOSE_SUPER_CHARGE_ID,
    
    INCREASE_VOICE_ID,
    DECREASE_VOCE_ID,
    MAX_VOICE_ID,
    MIN_VOICE_ID,

    EMERGENCY_CALL_ID,
    AUDIO_GUIDE_ID     
}ASR_CMD;
#pragma pack()


//void vmup_receive_packet(uint8_t receive_char);
void vmup_communicate_init(void);
//void userapp_deal_com_msg(sys_msg_com_data_t *msg);
void userapp_send_com_test(void);
uint8_t hex_to_decimal_val(uint8_t hex);
//lme add 
void lme_asr_cmd_com_send(uint8_t header, uint16_t cmd_id,uint32_t semantic_id);


void gas_alarm_com_send(uint8_t header,uint32_t coch4_value,uint8_t type);

//void lme_recv_char(unsigned char rxdata);
void lme_uart_recv_char(unsigned char rxdata);
int lme_vmup_port_send_packet_rev_msg(sys_com_msg_data_t *msg);

void userapp_deal_com_msg(sys_com_msg_data_t *com_data);

void uart2_send_Play_FeedBack(sys_com_msg_data_t *com_data);
#define SOP_STATE1      0x00
#define SOP_STATE2      0x01
#define DATA_STATE      0x02


#ifdef __cplusplus
}
#endif


#endif


