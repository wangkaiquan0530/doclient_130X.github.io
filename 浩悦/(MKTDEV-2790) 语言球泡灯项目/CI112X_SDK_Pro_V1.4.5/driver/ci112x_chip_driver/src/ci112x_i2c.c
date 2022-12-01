/**
 * @file ci112x_i2c.c
 * @brief CI112X芯片IIC模块的驱动程序
 * @version 0.1
 * @date 2020-07-20
 *
 * @copyright Copyright (c) 2020  Chipintelli Technology Co., Ltd.
 *
 */
#include <stddef.h>
#include <string.h>
#include "ci112x_i2c.h"
#include "ci_log.h"
#include "ci112x_core_eclic.h"
#include "ci112x_scu.h"
#include "platform_config.h"

#define i2c_outb(adap, reg, val) adap->write_byte(adap->data, reg, val)
#define i2c_inb(adap, reg) adap->read_byte(adap->data, reg)

#define i2c_set_clk(adap, val) i2c_outb(adap, IIC_SCLDIV, val)
#define i2c_status(adap) i2c_inb(adap, IIC_STATUS)
#define i2c_set_cmd(adap,val)  i2c_outb(adap,IIC_CMD,val)
#define i2c_intclr_set(adap,val)  i2c_outb(adap,IIC_INTCLR,val)
#define i2c_wait(adap) adap->wait_for_completion(adap->data)
#define i2c_rx_byte(adap) i2c_inb(adap, IIC_RXDR)
#define i2c_resetd(adap) adap->reset(adap->data)
#define i2c_tx_byte(adap, val) i2c_outb(adap, IIC_TXDR, val)

#define _REG_R(reg) *(volatile uint32_t*)(reg)
#define _REG_W(val,reg) *(volatile uint32_t*)(reg)=(val)

struct i2c_data i2c0_master;           /* 寻址IICx */
uint32_t i2c0_is_master;               /* 0为SLAVE 1为MASTER */
uint32_t i2c0_send_count;              /* IICx作为master时，发送数据个数 */
uint32_t i2c0_recv_count;              /* IICx作为salve 时，接收数据个数 */
uint8_t i2c0_reg = 0xFF;               /* IICx作为salve 时，通信协议寄存器地址 */

#define IIC_WAITEEVENT_TIMEOUT  50    //Normally 3 is ready
static volatile uint32_t i2c_timeout = 0;


/**
 * @brief 延迟函数
 *
 * @param time : 延迟毫秒
 */
static void delay_time(int32_t time)
{
    volatile int32_t loop;
    while(time--)
    {
        loop = 100;
        while(loop--);
    }
}


#if DRIVER_OS_API
#include "FreeRTOS.h"
#include "event_groups.h"
static EventGroupHandle_t iic_int_event_group = NULL;


/**
 * @brief i2c事件组初始化
 *
 * @param void : 无
 */
void iic_int_event_group_init(void)
{
    iic_int_event_group = xEventGroupCreate();
    if(iic_int_event_group == NULL)
    {
        ci_logerr(LOG_IIC_DRIVER,"=== ERROR\n");
    }
}


/**
 * @brief 等待中断信号量
 *
 * @param i2c : 传入指针，用来寻址寄存器
 * @return int32_t : 0 成功，-1 失败
 */
int32_t wait_event_interruptible(struct i2c_data *i2c)
{
    int32_t wait_bit = 0;
    if(&i2c0_master == i2c)
    {
        wait_bit = 0x1 << 0;
    }

    EventBits_t uxBits;
    UBaseType_t currPri = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL,configMAX_PRIORITIES-1);
    uxBits = xEventGroupWaitBits(iic_int_event_group,
                                wait_bit,
                                pdTRUE,
                                pdFALSE,
                                pdMS_TO_TICKS(IIC_WAITEEVENT_TIMEOUT));
    vTaskPrioritySet(NULL,currPri);
    if(0 != (uxBits & wait_bit))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}


/**
 * @brief 设置中断标志
 *
 * @param i2c : 传入指针，用来寻址寄存器
 */
void iic_with_os_int(struct i2c_data* i2c)
{
    static portBASE_TYPE iic_xHigherPriorityTaskWoken = pdFALSE;
    int32_t reg = 0;
    if(&i2c0_master == i2c)
    {
        reg = 0x1 << 0;
    }

    xEventGroupSetBitsFromISR(iic_int_event_group,reg,&iic_xHigherPriorityTaskWoken);
}


#else


/**
 * @brief 等待中断信号量
 *
 * @param i2c : 传入指针，用来寻址寄存器
 * @return int32_t : 0 成功，-1 失败
 */
int32_t wait_event_interruptible(struct i2c_data *i2c)
{
    int32_t i = 0;
    i2c_timeout = 0;
    while(i2c->wait != WAKE_UP)
    {
        delay_time(3);
        i++;
        if(i >= IIC_WAITEEVENT_TIMEOUT)
        {
            i2c_timeout = -1;
            break;
        }
    }
    i2c->wait = SLEEP;
    return i2c_timeout;
}


/**
 * @brief 设置中断标志
 *
 * @param i2c : 传入指针，用来寻址寄存器
 */
void iic_with_os_int(struct i2c_data* i2c)
{
    i2c->wait = WAKE_UP;
}


#endif


/**
 * @brief 读I2C寄存器值
 *
 * @param pd : 传入指针，用来寻址寄存器
 * @param reg : 寄存器偏移
 */
static int32_t i2c_readbyte(void *pd, int32_t reg)
{
    struct i2c_data *i2c = (struct i2c_data *)pd;
    return _REG_R(i2c->io_base + reg);    //ioread8(i2c->io_base + reg);
}


/**
 * @brief 写I2C寄存器值
 *
 * @param pd : 传入指针，用来寻址寄存器
 * @param reg : 寄存器偏移
 * @param val : 写入寄存器的值
 */
static void i2c_writebyte(void *pd, int32_t reg, int32_t val)
{
    struct i2c_data *i2c = (struct i2c_data *)pd;
    _REG_W(val, i2c->io_base + reg);    //iowrite8(val, i2c->io_base + reg);
}


/**
 * @brief 等待I2C总线传输完成
 *
 * @param pd : 传入指针，用来寻址寄存器
 * @return int32_t : 0 成功，-1 失败
 */
static int32_t i2c_waitforcompletion(void *pd)
{
    struct i2c_data *i2c = (struct i2c_data*)pd;
    int32_t ret = 0;

    if (i2c->irq)
    {
      ret = wait_event_interruptible(i2c);
    }
    else
    {
        /*
        * Do polling...
        * XXX: Could get stuck in extreme cases!
        *      Maybe add timeout, but using irqs is preferred anyhow.
        */
        while ((i2c->algo_data.read_byte(i2c, IIC_STATUS)
        & SR_IRQ_FLAG) == 0)
            delay_time(200);

        i2c->algo_data.i2c_errors = i2c->algo_data.read_byte(i2c, IIC_STATUS);
        if(i2c->algo_data.i2c_over == I2C_TRANS_OVER)
        {
            i2c->algo_data.i2c_errors &= ~SR_RXACK;
            i2c->algo_data.i2c_over = I2C_TRANS_ON;
        }
    }

    i2c->algo_data.i2c_errors &= (~SR_IRQ_FLAG);
    return ret;
}


/**
 * @brief 复位I2C总线
 *
 * @param pd : 传入指针，用来寻址寄存器
 */
static void i2c_reset(void *pd)
{
    struct i2c_data *i2c = (struct i2c_data*)pd;
    i2c->algo_data.write_byte(i2c, IIC_CMD, CR_STOP);        //:TODO: send stop
}


/**
 * @brief I2C总线发送start位
 *
 * @param adap : 传入指针，用来寻址寄存器
 */
static void i2c_start(struct i2c_algo_data *adap)
{
    uint32_t value;
    ci_loginfo(LOG_IIC_DRIVER,"=== START\n");
    value = (0x1 << 4);
    i2c_intclr_set(adap,value);
    value = CR_START | CR_TB;
    i2c_set_cmd(adap,value);
    i2c_wait(adap);
}


/**
 * @brief I2C总线发送stop位
 *
 * @param adap : 传入指针，用来寻址寄存器
 */
static void i2c_stop(struct i2c_algo_data *adap)
{
    uint32_t value;
    ci_loginfo(LOG_IIC_DRIVER,"=== STOP\n");
    adap->i2c_over = I2C_TRANS_OVER;
    value = CR_STOP | CR_TB;
    i2c_set_cmd(adap,value);
    value = CR_IRQ_CLEAR;
    i2c_intclr_set(adap,value);
    i2c_wait(adap);
}


/**
 * @brief 配置发送数据地址
 *
 * @param adap : 传入指针，用来寻址寄存器
 * @param msg : I2C传递地址等消息指针
 */
static void i2c_address(struct i2c_algo_data * adap, struct i2c_msg *msg)
{
    uint16_t flags = msg->flags;
    uint8_t addr;

    addr = (uint8_t) ( (0x7f & msg->addr) << 1 );

    if (flags & I2C_M_READ )
    {
      addr |= 1;
    }

    i2c_outb(adap, IIC_TXDR, addr);
}


/**
 * @brief I2C发送数据
 *
 * @param adap : 传入指针，用来寻址寄存器
 * @param lastbyte : 1 是最后一个字节，0 不是最后一个字节
 * @param receive : 1 发送数据，0 接收数据，
 * @param midbyte : 1 是中间数据，0 不是中间数据
 */
static void i2c_transfer( struct i2c_algo_data *adap, int32_t lastbyte, int32_t receive, int32_t midbyte)
{
    uint8_t value;

    if(lastbyte)
    {
        adap->i2c_over = I2C_TRANS_OVER;
        if( receive==I2C_RECEIVE)
        {
            //value = CR_READ | CR_IRQ_CLEAR | CR_NACK | CR_STOP ;
            value = CR_NACK | CR_TB;
            i2c_set_cmd(adap,value);
            value = CR_IRQ_CLEAR;
            i2c_intclr_set(adap,value);
        }
        else
        {
            //value = CR_WRITE | CR_IRQ_CLEAR  | CR_NACK | CR_STOP ;
            value = CR_WRITE | CR_TB;
            i2c_set_cmd(adap,value);
            value = CR_IRQ_CLEAR;
            i2c_intclr_set(adap,value);
        }
        ci_loginfo(LOG_IIC_DRIVER,"=== LASTBYTE\n");
    }
    else if(midbyte)
    {
        if(receive==I2C_RECEIVE)
        {
            //value = CR_READ | CR_IRQ_CLEAR  ;
            value = CR_READ | CR_TB;
            i2c_set_cmd(adap,value);
            value = CR_IRQ_CLEAR;
            i2c_intclr_set(adap,value);
        }
        else
        {
            //value = CR_WRITE | CR_IRQ_CLEAR | CR_NACK ;
            value = CR_WRITE | CR_NACK | CR_TB;
            i2c_set_cmd(adap,value);
            value = CR_IRQ_CLEAR;
            i2c_intclr_set(adap,value);
        }
        ci_loginfo(LOG_IIC_DRIVER,"=== MIDBYTE\n");
    }

    i2c_wait(adap);
}



/**
 * @brief I2C发送多个字节数据
 *
 * @param adap : 传入指针，用来寻址寄存器
 * @param buf : 发送数据缓冲指针
 * @param count : 发送字节个数
 * @param last : 当前发送计数
 *
 * @return int32_t : 发送字节个数
 */
static int32_t i2c_sendbytes(struct i2c_algo_data *adap, const int8_t *buf, int32_t count, int32_t last)
{
    int32_t wrcount;
    for (wrcount=0; wrcount<count; ++wrcount)
    {
        i2c_tx_byte(adap,buf[wrcount]);
        if ((wrcount==(count-1)) && last)
        {
            i2c_transfer( adap, last, I2C_TRANSMIT, 0);
            i2c_stop(adap);
        }
        else
        {
            i2c_transfer( adap, 0, I2C_TRANSMIT, 1);
        }

        if(adap->i2c_errors & SR_ABITOR_LOST)
        {
            i2c_stop(adap);
            break;
        }

        if(adap->i2c_errors & SR_TRANS_ERR)
        {
            i2c_stop(adap);
            break;
        }

        if(adap->i2c_errors & SR_RXACK)
        {
            ci_loginfo(LOG_IIC_DRIVER,"=== NOT ACK received after SLA+R\n");
            i2c_stop(adap);
            break;
        }
    }

    return (wrcount);
}


/**
 * @brief I2C读取多个字节数据
 *
 * @param adap : 传入指针，用来寻址寄存器
 * @param buf : 读取数据缓冲指针
 * @param count : 读取字节个数
 * @param last : 当前读取计数
 *
 * @return uint32_t: 读取字节个数
 */
static uint32_t  i2c_readbytes(struct i2c_algo_data *adap, int8_t *buf, int32_t count, int32_t last)
{
    int32_t i;

    /* increment number of bytes to read by one -- read dummy byte */
    for (i = 0; i <= count; i++)
    {
        if (i!=0)
        {
            /* set ACK to NAK for last received byte ICR[ACKNAK] = 1
            only if not a repeated start */

            if ((i == count) && last)
            {
                i2c_transfer( adap, last, I2C_RECEIVE, 0);
            }
            else
            {
                i2c_transfer( adap,0, I2C_RECEIVE, 1);
            }

            if(adap->i2c_errors & SR_ABITOR_LOST)
            {
                i2c_stop(adap);
                break;
            }

            if(adap->i2c_errors & SR_TRANS_ERR)
            {
                i2c_stop(adap);
                break;
            }

            if(adap->i2c_errors & SR_RXACK)
            {
                i2c_stop(adap);
                break;
            }

        }

        if (i)
        {
            buf[i - 1] = i2c_rx_byte(adap);
        }
        else
        {
            i2c_rx_byte(adap); /*! dummy read */
        }
    }
    i2c_stop(adap);
    return (i - 1);
}


/**
 * @brief I2C master模式 初始化
 *
 * @param I2Cx : I2C基地址（IIC0）
 * @param speed : 传输速率（100 标准、 400 快速），单位kbit/s
 * @param timeout : 超时时间（参考枚举类型 I2C_TimeOut 选择）
 *
 * @return int32_t : 0
 */
int32_t i2c_master_init(I2C_TypeDef *I2Cx, uint32_t speed, I2C_TimeOut timeout)
{
    struct i2c_data *i2c;
    uint32_t clock;
    uint32_t scldiv_l;
    uint32_t scldiv_h;
    uint32_t scldiv;
    uint32_t tmp;
    IRQn_Type irq;
    
    Scu_SetDeviceGate((uint32_t)I2Cx,ENABLE);
    Scu_Setdevice_Reset((uint32_t)I2Cx);
    Scu_Setdevice_ResetRelease((uint32_t)I2Cx);

    i2c0_is_master = 1;
    i2c0_send_count = 0;
    i2c = &i2c0_master;
    irq = IIC0_IRQn;
    I2Cx->SLAVDR = IIC0_SLAVEADDR;
    
    eclic_irq_enable(irq);
    
    i2c->irq = irq;
    i2c->io_base = (uint32_t)I2Cx;
    i2c->wait =SLEEP;
    i2c->timeout = timeout;

    clock = get_apb_clk();
    scldiv_l = (clock / (speed *1000)) / 2 - 15;
    scldiv_h = (scldiv_l << 16);
    scldiv = (scldiv_l | scldiv_h);

    i2c->algo_data.data = i2c;
    i2c->algo_data.write_byte = i2c_writebyte;
    i2c->algo_data.read_byte = i2c_readbyte;
    i2c->algo_data.wait_for_completion = i2c_waitforcompletion;
    i2c->algo_data.reset = i2c_reset;
    i2c->algo_data.i2c_errors = 0;
    i2c->algo_data.i2c_over = 0;

    I2Cx->INTCLR = 0xffffffff;
    I2Cx->GLBCTRL = 0x00040080;
    I2Cx->SCLDIV = scldiv;
    I2Cx->SRHLD = scldiv;
    tmp = scldiv_l / 2;
    tmp = (tmp << 16) | (scldiv_l / 2);
    if(tmp)
    {
        I2Cx->DTHLD = tmp;
    }
    else
    {
        I2Cx->DTHLD = 0x00040004;
    }
    I2Cx->INTCLR = 0x7f;
    I2Cx->INTEN = (RXDUFLIE | TXDEPTIE | SSTOPIE | SLVADIE);
    I2Cx->GLBCTRL |= 0x3;

    return 0;
}


/**
 * @brief I2C slave模式 初始化
 *
 * @param I2Cx : I2C基地址（IIC0）
 * @param speed : 传输速率（100 标准、 400 快速），单位kbit/s
 * @param timeout : 超时时间（参考枚举类型 I2C_TimeOut 选择）
 *
 * @return int32_t : 0
 */
int32_t i2c_slave_init(I2C_TypeDef *I2Cx, uint32_t speed, I2C_TimeOut timeout)
{
    struct i2c_data *i2c;
    uint32_t clock;
    uint32_t scldiv_l;
    uint32_t scldiv_h;
    uint32_t scldiv;
    uint32_t tmp;
    IRQn_Type irq;
    
    Scu_SetDeviceGate((uint32_t)I2Cx,ENABLE);
    Scu_Setdevice_Reset((uint32_t)I2Cx);
    Scu_Setdevice_ResetRelease((uint32_t)I2Cx);
    
    i2c0_is_master = 0;
    i2c0_recv_count = 0;
    i2c = &i2c0_master;
    irq = IIC0_IRQn;
    I2Cx->SLAVDR = IIC0_SLAVEADDR;
    
    eclic_irq_enable(irq);
    
    i2c->irq = irq;
    i2c->io_base = (uint32_t)I2Cx;
    i2c->wait =SLEEP;
    i2c->timeout = timeout;
    
    clock = get_apb_clk();
    scldiv_l = (clock / (speed *1000)) / 2 - 15;
    scldiv_h = (scldiv_l << 16);
    scldiv = (scldiv_l | scldiv_h);
    
    i2c->algo_data.data = i2c;
    i2c->algo_data.write_byte = i2c_writebyte;
    i2c->algo_data.read_byte = i2c_readbyte;
    i2c->algo_data.wait_for_completion = i2c_waitforcompletion;
    i2c->algo_data.reset = i2c_reset;
    i2c->algo_data.i2c_errors = 0;
    i2c->algo_data.i2c_over = 0;

    I2Cx->INTCLR = 0xffffffff;
    I2Cx->GLBCTRL = 0x00040084;
    I2Cx->SCLDIV = scldiv;
    I2Cx->SRHLD = scldiv;
    tmp = scldiv_l / 2;
    tmp = (tmp << 16) | (scldiv_l / 2);
    if(tmp)
    {
        I2Cx->DTHLD = tmp;
    }
    else
    {
        I2Cx->DTHLD = 0x00040004;
    }
    I2Cx->INTCLR = 0x7f;
    I2Cx->INTEN = (RXDUFLIE | TXDEPTIE | SSTOPIE | SLVADIE);
    I2Cx->GLBCTRL |= 0x2;

    return 0;
}


/**
 * @brief I2C使用消息传输
 *
 * @param adap : 传入指针，用来寻址寄存器
 * @param msgs : 消息指针
 * @param num : 传输消息个数
 *
 * @return int32_t : 1或2 传输消息个数，-1超时，-2 错误
 */
int32_t i2c_master_transfer(struct i2c_data * i2c_adap, struct i2c_msg *msgs, int32_t num)
{
    struct i2c_algo_data *adap = &i2c_adap->algo_data;
    struct i2c_msg *msg = NULL;
    int32_t curmsg;
    int32_t state;
    int32_t ret,retval;
    int32_t last;
    int32_t timeout = i2c_adap->timeout;

    while (((state = i2c_status(adap)) & SR_BUSY) && timeout--)
    {
        delay_time(10);
    }
    if (state & SR_BUSY)
    {
        return -ERET;
    }
    ret = -ERET;
    for (curmsg = 0;curmsg < num; curmsg++)
    {
        last = curmsg + 1 == num;
        msg = &msgs[curmsg];
        i2c_address(adap,msg);
        i2c_start(adap);
        if(adap->i2c_errors & SR_ABITOR_LOST)
        {
            i2c_stop(adap);
            goto out;
        }

        if(adap->i2c_errors & SR_TRANS_ERR)
        {
            i2c_stop(adap);
            goto out;
        }

        if(adap->i2c_errors & SR_RXACK)
        {
            i2c_stop(adap);
            goto out;
        }
        /* Read */
        if (msg->flags & I2C_M_READ)
        {
            /*read bytes into buffer*/
            retval = i2c_readbytes(adap, (int8_t *)msg->buf, msg->len, last);
            if (retval != msg->len)
            {
                goto out;
            }
            else
            {
            }
        }
        else /* Write */
        {
            retval = i2c_sendbytes(adap, (int8_t *)msg->buf, msg->len, last);

            if (retval != msg->len)
            {
                goto out;
            }
            else
            {
            }
        }
    }
    ret = curmsg;
out:
    return ret;
}


/**
 * @brief I2C 只发送数据。应用于只写数据的需求
 *
 * @param slave_ic_address : SLAVE设备IIC芯片地址，不带读写位。如IIC设备地址0x20左移一位或上读写位后为0x40，0x41
 * @param buf : 发送数据缓存指针
 * @param count : 发送数据字节个数
 *
 * @return int32_t : count 成功发送数据字节个数，-1 等待超时，-2 错误或中断超时
 */
int32_t i2c_master_only_send(char slave_ic_address,const char *buf,int32_t count)
{
    int32_t ret = 1;
    struct i2c_msg msg;
    struct i2c_data *adap = &i2c0_master;
    
    msg.addr = slave_ic_address;
    msg.flags = I2C_M_WRITE;
    msg.len = count;
    msg.buf = (uint8_t *)buf;

    ret = i2c_master_transfer(adap, &msg, 1);
    if(i2c_timeout == 0)
    {
        ret = count;
    }
    else if(i2c_timeout == 1)
    {
        ret = RETURN_ERR;
    }

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    return ret;
}


/**
 * @brief I2C 先发送数据，再接收数据。应用于发送数据请求，再读取回复数据的需求
 *
 * @param slave_ic_address : SLAVE设备IIC芯片地址，不带读写位。如IIC设备地址0x20左移一位或上读写位后为0x40，0x41
 * @param buf : 发送/接收数据缓存指针
 * @param send_len : 发送数据字节个数
 * @param rev_len : 接收数据字节个数
 *
 * @return int32_t : rev_len 接收数据个数，-1  等待超时，-2 错误或中断超时
 */
int32_t i2c_master_send_recv(char slave_ic_address,char *buf,int32_t send_len,int32_t rev_len)
{
    int32_t ret32 = 0;
    struct i2c_msg msgs[2];
	struct i2c_data *adap = &i2c0_master;
    
    msgs[0].addr    = slave_ic_address;
    msgs[0].flags   = I2C_M_WRITE;
    msgs[0].len     = send_len;
    msgs[0].buf     = (uint8_t*)buf;
    
    msgs[1].addr    = slave_ic_address;
    msgs[1].flags   = I2C_M_READ;
    msgs[1].len     = rev_len;
    msgs[1].buf     = (uint8_t*)buf;

    ret32 = i2c_master_transfer(adap,msgs,2);

    return (ret32 == 2) ? rev_len : ret32;
}


/**
 * @brief I2C 只接收数据。应用于只读数据的
 *
 * @param slave_ic_address : SLAVE设备IIC芯片地址，不带读写位。如IIC设备地址0x20左移一位或上读写位后为0x40，0x41
 * @param buf : 接收数据缓存指针
 * @param rev_len : 接收数据字节个数
 *
 * @return int32_t : rev_len 接收数据个数，-1 等待超时，-2 错误或中断超时
 */
int32_t i2c_master_only_recv(char slave_ic_address,char *buf,int32_t rev_len)
{
    int32_t ret32 = 0;
    struct i2c_msg msgs;
	struct i2c_data *adap = &i2c0_master;

    msgs.addr    = slave_ic_address;
    msgs.flags   = I2C_M_READ;
    msgs.len     = rev_len;
    msgs.buf     = (uint8_t*)buf;

    ret32 = i2c_master_transfer(adap,&msgs,1);

    return (ret32 == 1) ? rev_len : ret32;
}


/********************************** sample **********************************/
/**
 * @brief I2C 使用示例
 */
#define IIC_TEST_SLAVE_ADDR  0x20  /*不带读写位，IIC设备地址左移一位，或上读写位后为0x40，0x41*/

void i2c_test()
{
    /*I/O 引脚初始化*/
    i2c_io_init();

    /*master模式初始化，IIC0总线，100K速率，超时等待时间*/
    i2c_master_init(IIC0,100,LONG_TIME_OUT);

	/*往IIC设备地址为0x20的设备，只写入5个字节*/
	char only_send_buf[5] = {0x01,0x02,0x03,0x04,0x05};
	i2c_master_only_send(IIC_TEST_SLAVE_ADDR,only_send_buf,5);

	/*往IIC设备地址为0x20的设备，先写入1个字节，再读出5个字节*/
	char send_recv_buf [10] = {0} ;
	send_recv_buf [0] = 0x01;
	i2c_master_send_recv(IIC_TEST_SLAVE_ADDR,send_recv_buf,1,10);

	/*往IIC设备地址为0x20的设备，只读出5个字节*/
	char only_recv_buf [10] = {0} ;
	i2c_master_only_recv(IIC_TEST_SLAVE_ADDR,only_recv_buf,10);
}

 /***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
