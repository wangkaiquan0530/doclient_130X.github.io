


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

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


#define TIMEOUT_ONE_PACKET_INTERVAL (1000)/*ms, in this code, it should be bigger than portTICK_PERIOD_MS */
#define MAX_DATA_RECEIVE_PER_PACKET (80)/*???*/

static uint8_t co_gas_alarm_value=0;//co 值
static uint8_t ch4_gas_alarm_value=0;//ch4 值
/*报警器间隔播放时间*/
xTimerHandle  gas_alarm_time = NULL;
static uint8_t g_gas_alarm_type = 0;//1  2 3 播放类型


uint8_t glb_send_sequence;

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

//lme add
static uint8_t tc_state2 = 0;
static uint8_t tc_cnt2=0;
static uint8_t rx_buf2[sizeof(sys_com_msg_data_t)];

void gas_alarm_play_voice_callback(cmd_handle_t cmd_handle)
{
    resume_voice_in();
    
}

/*****************************************************************************
 Prototype    : gas_alarm_time_callback
 Description  : 报警器间隔播放时间
*****************************************************************************/
void gas_alarm_time_callback(TimerHandle_t xTimer)
{
    xTimerStop(gas_alarm_time,0);
    pause_voice_in();
    if(1 == g_gas_alarm_type)
    {
        prompt_play_by_cmd_string("<coconcentration>", -1, gas_alarm_play_voice_callback,true);
    }
    else if(2 == g_gas_alarm_type)
    {
        prompt_play_by_cmd_string("<ch4concentration>", -1, gas_alarm_play_voice_callback,true);
    }
    else if(3 == g_gas_alarm_type)
    {
        prompt_play_by_cmd_string("<coch4concentration>", -1, gas_alarm_play_voice_callback,true);
    }

	xTimerStart(gas_alarm_time,0);

}

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
        lme_uart_recv_char(UART_RXDATA((UART_TypeDef*)UART_PROTOCOL_NUMBER));
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

    //vmup_send_packet(&msg);
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

        //播报报警器语音
    gas_alarm_time = xTimerCreate("gas_alarm_time", (pdMS_TO_TICKS(GAS_ALARM_PLAY_TIME)),\
      pdFALSE, (void *)0, gas_alarm_time_callback);    
    if(NULL == gas_alarm_time)
    {
        ci_logerr(LOG_USER,"user task create gas_alarm_time error\n");
    }  

}


/********************************************************************
                     user add function
********************************************************************/

void play_voice_callback(cmd_handle_t cmd_handle)
{
    resume_voice_in();
}


/*****************************************************************************
 Prototype    : lme_asr_cmd_com_send
 Description  : 语音识别结果，使用串口进行处理
 Input        : uint16_t cmd_id,识别到的命令词ID
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2020/07/07
    Author       : yaoj
    Modification : Created function
*****************************************************************************/
void lme_asr_cmd_com_send(uint8_t header, uint16_t cmd_id,uint32_t semantic_id)
{
    sys_com_msg_data_t com_data;
    unsigned int pathValue;
    uint8_t index = 0,i = 0,num = 0;
    uint8_t *buf = (uint8_t*)(&com_data);
    
    ci_loginfo(LOG_USER,"lme_asr_semanticid_com_send %x \n", semantic_id);
    
    index = semantic_id;//cmd_id;  //edit by yxp 20220423
    memset((void *)(&com_data),0,sizeof(sys_com_msg_data_t));
    
    com_data.header0 = header;
    com_data.code = UART_CODE_ASR;
    com_data.id_H= (UART_DATA_ID& 0xFF00) >> 8;
    com_data.id_L= UART_DATA_ID & 0xFF;
    
    //ci_loginfo(LOG_USER,"asr cmd_id:%x \n", cmd_id);
   
    if(index == AWAKEN_INDEX_CMD )//唤醒词
    {
    	 pathValue = PATH_AWAKEN;
    }
    else if((index >=ON_CONTROL_INDEX_START) && (index <= OFF_CONTROL_INDEX_END))//开关机
    {
        pathValue = PATH_OPEN_CLOSE;
    }
    else if((index >=INCREASE_TEMP_INDEX_START) && (index <= WATER_48_DEGREES_INDEX_END))//温度控制
    {
        pathValue = PATH_TEMP_CONTROL;
    }
    else if(((index >=OPEN_BOOKING_INDEX_CMD) && (index <= CLOSE_SUPER_CHARGE_INDEX_CMD)) || ((index >=OPEN_TWO_SUPER_CHARGE_INDEX_CMD) && (index <= CLOSE_DISINFECT_INDEX_CMD)))//模式控制
    {
        pathValue = PATH_MODE_CONTROL;    //34
    }
    else if((index >=INCREASE_VOICE_INDEX_CMD) && (index <= MIN_VOICE_INDEX_CMD))//音量调节
    {
        pathValue = PATH_VOLUME_CONTROL;  //35
    }
    else if((index >=EMERGENCY_CALL_INDEX_CMD && index <= CLOSE_COOL_WATER_INDEX_CMD) || (index >=OPEN_SIG_COOL_WATER_INDEX_CMD && index <= CLOSE_SIG_COOL_WATER_INDEX_CMD))//
    {
        pathValue = PATH_URGENT_CONTROL;  //36
    }
    else if(index >=OPEN_KITCHEN_CLEAN_INDEX_CMD && index <= CLOSE_COMFORT_CLEAN_INDEX_CMD)//
    {
        pathValue = PATH_SMART_CONTROL;   //37
    }
    com_data.path_3 = (pathValue >> 24) &0xFF;
    com_data.path_2 = (pathValue >> 16) &0xFF;
    com_data.path_1 = (pathValue >> 8) &0xFF;
    com_data.path_0 = pathValue &0xFF;
    com_data.token_H = (UART_DATA_TOKEN& 0xFF00) >> 8;
    com_data.token_L = UART_DATA_TOKEN & 0xFF;
    com_data.payload= UART_DATA_PAYLOAD;
    com_data.Cmd_H= (semantic_id  &0xFF00) >> 8;  
    com_data.Cmd_L=  semantic_id  & 0xFF;  
    com_data.end= UART_DATA_END;
   
    //串口发送出去
    if(buf != NULL)
    {
        vmup_port_mutex_take();
        num = sizeof(sys_com_msg_data_t);
        ci_loginfo(LOG_USER,"uart send \r\n");
        for(i = 0;i < num; i++)
        {
            vmup_port_send_char( buf[i] );
        }
        vmup_port_mutex_give();
    }        

   
}

/*****************************************************************************
 Prototype    : lme_gas_alarm_com_send
 Description  : 发送CO\CH4，使用串口进行处理
 Input        : (1)coch4_value  co或者ch4值  (2)type 0 1 2 分别是co、ch4、双气状态
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2022/12/12
    Author       : wkq
    Modification : Created function
*****************************************************************************/
void gas_alarm_com_send(uint8_t header,uint32_t coch4_value,uint8_t type)
{
    sys_com_msg_data_t com_data;
    unsigned int pathValue;
    uint8_t i = 0,num = 0;
    uint8_t *buf = (uint8_t*)(&com_data);
    
    ci_loginfo(LOG_USER,"gas_alarm_com_send %x \n", coch4_value);
    
    memset((void *)(&com_data),0,sizeof(sys_com_msg_data_t));
    
    com_data.header0 = header;
    com_data.code = UART_CODE_ASR;
    com_data.id_H= (UART_DATA_ID& 0xFF00) >> 8;
    com_data.id_L= UART_DATA_ID & 0xFF;
    
    if(type == 0)
    {
        pathValue = PATH_CO_ALARM_CONTROL;
    }
    else if(type == 1)
    {
        pathValue = PATH_CH4_ALARM_CONTROL;
    }
    // else if(type == 2)
    // {
    //     pathValue = PATH_DOUBLE_ALARM_STATUS;
    // }


    com_data.path_3 = (pathValue >> 24) &0xFF;
    com_data.path_2 = (pathValue >> 16) &0xFF;
    com_data.path_1 = (pathValue >> 8) &0xFF;
    com_data.path_0 = pathValue &0xFF;
    com_data.token_H = (UART_DATA_TOKEN& 0xFF00) >> 8;
    com_data.token_L = UART_DATA_TOKEN & 0xFF;
    com_data.payload= UART_DATA_PAYLOAD;
    com_data.Cmd_H= (coch4_value  &0xFF00) >> 8;  
    com_data.Cmd_L=  coch4_value  & 0xFF;  
    com_data.end= UART_DATA_END;
   
    //串口发送出去
    num = sizeof(sys_com_msg_data_t);
    if(buf != NULL)
    {
        vmup_port_mutex_take();
        ci_loginfo(LOG_USER,"uart send gas alarm\r\n");
        for(i = 0;i < num; i++)
        {
            vmup_port_send_char( buf[i] );
        }
        vmup_port_mutex_give();
    }
    vTaskDelay(pdMS_TO_TICKS(UART_RESEND_TIME));
    //间隔1.5秒重复 发送
    if(buf != NULL)
    {
        vmup_port_mutex_take();
        ci_loginfo(LOG_USER,"uart send gas second alarm\r\n");
        for(i = 0;i < num; i++)
        {
            vmup_port_send_char( buf[i] );
        }
        vmup_port_mutex_give();
    }    

   
}
/*****************************************************************************
 Prototype    : hex_to_decimal_val
 Description  : 16进制转10进制
 Input        : uint8_t hex,16进制值
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2020/07/08
    Author       : yaoj
    Modification : Created function
*****************************************************************************/
uint8_t hex_to_decimal_val(uint8_t hex)
{
	if ((hex >= '0') && (hex <= '9'))
		return (hex - '0');
	else if ((hex >= 'a') && (hex <= 'f'))
		return (hex - 'a' + 10);
	else if ((hex >= 'A') && (hex <= 'F'))
		return (hex - 'A' + 10);

	return 0;
}



/*****************************************************************************
 Prototype    : lme_uart_recv_char
 Description  : 串口接收字符
 Input        : unsigned char rxdata,16进制值
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2020/07/08
    Author       : yaoj
    Modification : Created function
*****************************************************************************/
void lme_uart_recv_char(unsigned char rxdata)
{
    mprintf("%02x ",rxdata);
    int tmp_data;
    switch(tc_state2)
    {
        case SOP_STATE1:
            if(UART_HEADER_CON == rxdata || UART_HEADER_NON == rxdata)
            {
                rx_buf2[tc_cnt2++] = rxdata;
                tc_state2 = DATA_STATE;
            }            
        break;
        case DATA_STATE:
            if(tc_cnt2 < sizeof(sys_com_msg_data_t))
            {
                rx_buf2[tc_cnt2] = rxdata; 

                if(tc_cnt2 == (sizeof(sys_com_msg_data_t)-1))
                {
                    if(rx_buf2[tc_cnt2] == UART_DATA_END)
                    {
                      
                        sys_com_msg_data_t *com_data = (sys_com_msg_data_t *)rx_buf2;
                        unsigned short cmd_id;
                        unsigned short token;

                        //检查参数有效性
                        cmd_id = (com_data->id_H << 8) | com_data->id_L;
                        token = (com_data->token_H << 8) | com_data->token_L;
                        tmp_data = (com_data->Cmd_H << 8) | com_data->Cmd_L;
                        if(tmp_data == 0x0000){
                               mprintf("software reset \r\n");
                               //Scu_SoftwareRst_System_From_ISR();
                               //Scu_SoftwareRst_System();//wkq
                               dpmu_software_reset_system_config();
                        }else if(tmp_data == 0x99){
                          
                            //nvdata_save.test_flag = 1;
                            mprintf("write test_flag \r\n");
                        }
                        mprintf("\nUART1_rx_handle %02x %04x %04x %02x %02x\n",com_data->payload,cmd_id,token,com_data->token_H,rx_buf2[4]);
                        if(com_data->payload != UART_DATA_PAYLOAD || cmd_id != UART_DATA_ID || token != UART_DATA_TOKEN)
                        {
                            mprintf("com2 too many data,reset recive\r\n");
                            tc_state2 = SOP_STATE1;
                            tc_cnt2 = 0;
	                    break;
			}
                        mprintf("recv uart data \r\n");
                        lme_vmup_port_send_packet_rev_msg(com_data);
                    }
                    else
                    {
                        mprintf("get com data err_end \r\n");
                    }
                    memset(rx_buf2,0,sizeof(sys_com_msg_data_t));
                    tc_state2 = SOP_STATE1;
                    tc_cnt2 = 0;
                }
                else
                {
                    tc_cnt2++;                
                }               
            }
            else
            {
                mprintf("com2 too many data,reset recive\r\n");
                tc_state2 = SOP_STATE1;
                tc_cnt2 = 0;
            }
       break;        
    }
}
/*****************************************************************************
 Prototype    : lme_vmup_port_send_packet_rev_msg
 Description  : 串口接收事件传递
 Input        : unsigned char rxdata,16进制值
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2020/07/08
    Author       : yaoj
    Modification : Created function
*****************************************************************************/
int lme_vmup_port_send_packet_rev_msg(sys_com_msg_data_t *msg)
{
    sys_msg_t send_msg;
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    send_msg.msg_type = SYS_MSG_TYPE_COM;                             
    memcpy((uint8_t *)(&send_msg.msg_data), msg, sizeof(sys_com_msg_data_t));
    //memcpy((uint8_t *)(&send_msg.msg_data), msg, sizeof(sys_msg_t));
    
    xResult = send_msg_to_sys_task(&send_msg,&xHigherPriorityTaskWoken);

    if((xResult != pdFAIL)&&(pdTRUE == xHigherPriorityTaskWoken))
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    return xHigherPriorityTaskWoken;
}
/*****************************************************************************
 Prototype    : uart2_send_Play_FeedBack
 Description  : 串口接收事件传递
 Input        : unsigned int index,播报语音
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2020/07/08
    Author       : yaoj
    Modification : Created function
*****************************************************************************/
void uart2_send_Play_FeedBack(sys_com_msg_data_t *com_data)
{

    unsigned int pathValue;
    uint8_t *buf = (uint8_t*)(com_data);
    uint8_t  num = 0;

    com_data->header0 = UART_HEADER_NON;
    com_data->code = UART_CODE_PLAY_FEEDBACK;
  
    if(buf != NULL)
    {
        vmup_port_mutex_take();
        num = sizeof(sys_com_msg_data_t);
        ci_loginfo(LOG_USER,"uart 1 send \r\n");
        for(int i = 0;i < num; i++)
        {
            vmup_port_send_char( buf[i] );
        }
        vmup_port_mutex_give();
    } 
} 
/*****************************************************************************
 Prototype    : userapp_deal_com_msg
 Description  : 处理串口消息
 Input        : unsigned int index,播报语音
 Output       : None
 Return Value : None
 Calls        : 
 Called By    : 
  History        :
  1.Date         : 2020/07/08
    Author       : yaoj
    Modification : Created function
*****************************************************************************/
void userapp_deal_com_msg(sys_com_msg_data_t *com_data)
{    
    uint32_t tmp_data;
    int funcIdx;
    unsigned short cmd_id;
    unsigned short token;
    unsigned int pathValue;
    unsigned short play_flag=0;

    mprintf("\n########## 11111 \n");

    //电控主动发送播报
    if(com_data->code == UART_CODE_PLAY && com_data->header0 == UART_HEADER_CON)
    {   
      #if 0
        //Reset命令
        if( com_data->path_0 == 0x38 && ((com_data->Cmd_H << 8) | com_data->Cmd_L) == 0x28)
        {
		Scu_SoftwareRst_System();
		uart2_send_Play_FeedBack(com_data);
		return;
        }   
	  #endif
	
        //检查参数有效性
        //检查参数有效性
        cmd_id = (com_data->id_H << 8) | com_data->id_L;
        token = (com_data->token_H << 8) | com_data->token_L;

        if(com_data->payload != UART_DATA_PAYLOAD 
		|| cmd_id != UART_DATA_ID 
		|| token != UART_DATA_TOKEN)
        {    
                mprintf("参数错误 \r\n");
        	return;//参数错误
        }
        tmp_data = (com_data->Cmd_H << 8) | com_data->Cmd_L;
        mprintf("tmp_data0=%04x\r\n",tmp_data);

        pathValue=(com_data->path_3 <<24) | (com_data->path_2 <<16) | (com_data->path_1 <<8) | (com_data->path_0) ;
        mprintf("pathValue=%08x\r\n",pathValue);

        if( PATH_CO_ALARM_CONTROL == pathValue)//CO 浓度报警功能
        {
            //暂时不做处理
            co_gas_alarm_value=tmp_data;
            uart2_send_Play_FeedBack(com_data);
        }
        else if( PATH_CH4_ALARM_CONTROL == pathValue)//甲烷浓度报警功能
        {
            //暂时不做处理
            ch4_gas_alarm_value=tmp_data;
            uart2_send_Play_FeedBack(com_data);
        }
        else if( PATH_DOUBLE_ALARM_STATUS == pathValue)//双气报警器状态
        {
            switch (tmp_data)
            {
            case 0:
            case 1:
                xTimerStop(gas_alarm_time,0); 
                break;

            case 2://报警
                mprintf("o_gas_alarm_value==%x\n",co_gas_alarm_value);
                mprintf("ch4_gas_alarm_value==%x\n",ch4_gas_alarm_value);

                if((co_gas_alarm_value>= 0x1E) &&(ch4_gas_alarm_value>= 0xF))
                {
                    
                    g_gas_alarm_type=3;
                    xTimerStop(gas_alarm_time,0);
                    pause_voice_in();
                    prompt_play_by_cmd_string("<coch4concentration>", -1, gas_alarm_play_voice_callback,true);
                    xTimerStart(gas_alarm_time,0);
                }
                else if((co_gas_alarm_value>= 0x1E) &&(ch4_gas_alarm_value< 0xF))
                {
                    g_gas_alarm_type=1;
                    xTimerStop(gas_alarm_time,0);
                    pause_voice_in();
                    prompt_play_by_cmd_string("<coconcentration>", -1, gas_alarm_play_voice_callback,true);
                    xTimerStart(gas_alarm_time,0);
                }
                else if((co_gas_alarm_value < 0x1E) &&(ch4_gas_alarm_value>= 0xF))
                {
                    g_gas_alarm_type=2;
                    xTimerStop(gas_alarm_time,0);
                    pause_voice_in();
                    prompt_play_by_cmd_string("<ch4concentration>", -1, gas_alarm_play_voice_callback,true);
                    xTimerStart(gas_alarm_time,0);
                }
                break;

            case 3://自身故障
            case 7:
                break;
            
            default:
                break;
            }

        }
        else
        {
            if((tmp_data >= PLAY_INDEX_MIN && tmp_data  <= PLAY_INDEX_MAX))
            {
                //判断pathValue是否有效
                if((tmp_data == AWAKEN_INDEX_CMD) && (pathValue == PATH_AWAKEN))//唤醒词
                {
                    play_flag=1;
                }
                else if((tmp_data >=ON_CONTROL_INDEX_START && tmp_data <= OFF_CONTROL_INDEX_END)&& (pathValue == PATH_OPEN_CLOSE))//开关机
                {
                    play_flag=1;
                }
                else if((tmp_data >=INCREASE_TEMP_INDEX_START && tmp_data <= WATER_48_DEGREES_INDEX_END) && (pathValue == PATH_TEMP_CONTROL))//温度控制
                {
                    play_flag=1;
                }
                else if(((tmp_data >=OPEN_BOOKING_INDEX_CMD && tmp_data <= CLOSE_SUPER_CHARGE_INDEX_CMD) || (tmp_data >=OPEN_TWO_SUPER_CHARGE_INDEX_CMD && tmp_data <= CLOSE_DISINFECT_INDEX_CMD)) && (pathValue == PATH_MODE_CONTROL))//模式控制
                {
                    play_flag=1;
                }
                else if((tmp_data >=INCREASE_VOICE_INDEX_CMD && tmp_data <= MIN_VOICE_INDEX_CMD) && (pathValue == PATH_VOLUME_CONTROL)) //音量调节
                {
                    play_flag=1;
                }
                else if(((tmp_data >=EMERGENCY_CALL_INDEX_CMD && tmp_data <= CLOSE_COOL_WATER_INDEX_CMD) || (tmp_data >=OPEN_SIG_COOL_WATER_INDEX_CMD && tmp_data <= CLOSE_SIG_COOL_WATER_INDEX_CMD))  && (pathValue == PATH_URGENT_CONTROL))//
                {
                    play_flag=1;
                }
                else if((tmp_data >=OPEN_KITCHEN_CLEAN_INDEX_CMD && tmp_data <= CLOSE_COMFORT_CLEAN_INDEX_CMD) && (pathValue == PATH_SMART_CONTROL))//
                {
                    play_flag=1;
                }

                //pathValue有效后进行播放
                if(play_flag==1)
                {
                pause_voice_in();
                prompt_play_by_semantic_id(tmp_data,-1,play_voice_callback,true);
                uart2_send_Play_FeedBack(com_data);
                }
                /*
                if(voice_type_get()==0)
                {
                    prompt_play_by_semantic_id(tmp_data,-1,play_voice_callback,false);
                }
                else
                {
                    prompt_play_by_semantic_id(50+tmp_data,-1,play_voice_callback,false);
                }
                */
                
            return;
            }
            
            if(tmp_data == TEST_QUERY_INDEX){  //测试完成写入标志
                test_flag_set();
                uart2_send_Play_FeedBack(com_data);
            }
            else if(tmp_data == TEST_QUERY_INDEX){  //测试查询标志
                tmp_data = test_flag_get();
                com_data->Cmd_H=((tmp_data)&0xFF00) >> 8;  
                com_data->Cmd_L=(tmp_data) & 0xFF;  
                uart2_send_Play_FeedBack(com_data);
            }
            else if(tmp_data == VERSION_QUERY_INDEX)//版本查询
            {
                tmp_data = Prog_Version_Info - 1;
                com_data->Cmd_H=((tmp_data)&0xFF00) >> 8;  
                com_data->Cmd_L=(tmp_data) & 0xFF; 
                com_data->path_3 = (PATH_VERSION_QUERY >> 24) &0xFF;
                com_data->path_2 = (PATH_VERSION_QUERY >> 16) &0xFF;
                com_data->path_1 = (PATH_VERSION_QUERY >> 8) &0xFF;
                uart2_send_Play_FeedBack(com_data);
            }
        }
    }
    else if(com_data->code == UART_CODE_PLAY_FEEDBACK 
		&& com_data->header0 == UART_HEADER_NON)//电控收到命令词播报
    {
        //检查参数有效性
        cmd_id = (com_data->id_H << 8) | com_data->id_L;
	    token = (com_data->token_H << 8) | com_data->token_L;

        if(com_data->payload != UART_DATA_PAYLOAD 
		|| cmd_id != UART_DATA_ID 
		|| token != UART_DATA_TOKEN)
        {    
                mprintf("UART_CODE_PLAY_FEEDBACK 参数错误 \r\n");
        	return;//参数错误
        }

        pathValue=(com_data->path_3 <<24) | (com_data->path_2 <<16) | (com_data->path_1 <<8) | (com_data->path_0) ;
        mprintf("pathValue=%08x\r\n",pathValue);

        tmp_data = (com_data->Cmd_H << 8) | com_data->Cmd_L;
        mprintf("tmp_data1=%04x\r\n",tmp_data);

        if( PATH_CO_ALARM_CONTROL == pathValue)//CO 浓度报警功能
        {
            //暂时不做处理
            co_gas_alarm_value=tmp_data;
        }
        else if( PATH_CH4_ALARM_CONTROL == pathValue)//甲烷浓度报警功能
        {
            //暂时不做处理
            ch4_gas_alarm_value=tmp_data;
        }
        else if( PATH_DOUBLE_ALARM_STATUS == pathValue)//双气报警器状态
        {
            switch (tmp_data)
            {
            case 0:
            case 1:
                xTimerStop(gas_alarm_time,0); 
                break;

            case 2://报警
                mprintf("o_gas_alarm_value==%x\n",co_gas_alarm_value);
                mprintf("ch4_gas_alarm_value==%x\n",ch4_gas_alarm_value);

                if((co_gas_alarm_value>= 0x1E) &&(ch4_gas_alarm_value>= 0xF))
                {
                    
                    g_gas_alarm_type=3;
                    xTimerStop(gas_alarm_time,0);
                    pause_voice_in();
                    prompt_play_by_cmd_string("<coch4concentration>", -1, gas_alarm_play_voice_callback,true);
                    xTimerStart(gas_alarm_time,0);
                }
                else if((co_gas_alarm_value>= 0x1E) &&(ch4_gas_alarm_value< 0xF))
                {
                    g_gas_alarm_type=1;
                    xTimerStop(gas_alarm_time,0);
                    pause_voice_in();
                    prompt_play_by_cmd_string("<coconcentration>", -1, gas_alarm_play_voice_callback,true);
                    xTimerStart(gas_alarm_time,0);
                }
                else if((co_gas_alarm_value < 0x1E) &&(ch4_gas_alarm_value>= 0xF))
                {
                    g_gas_alarm_type=2;
                    xTimerStop(gas_alarm_time,0);
                    pause_voice_in();
                    prompt_play_by_cmd_string("<ch4concentration>", -1, gas_alarm_play_voice_callback,true);
                    xTimerStart(gas_alarm_time,0);
                }
                break;

            case 3://自身故障
            case 7:
                break;
            
            default:
                break;
            }

        }
        else
        {


            if((tmp_data < PLAY_INDEX_MIN || tmp_data > PLAY_INDEX_MAX))
            {
                return;//播报index超界
            }
                
            //正确播报，取消当前命令播报超时的计时
            if(userapp_get_current_function_index() == tmp_data)
            {
                userapp_set_current_function_index(INVALID_FUNCTION_INDEX);
                exit_uart_resend_timer();
            }
            else
            {
                mprintf("Play index is wrong!!!\r\n");
                return;
            }
            
            pause_voice_in();
            prompt_play_by_semantic_id(tmp_data,-1,play_voice_callback,false);  
        }

    }
    else
    {
        mprintf("com2 may recived an undefined cmd\r\n",com_data->code);
    }
}


