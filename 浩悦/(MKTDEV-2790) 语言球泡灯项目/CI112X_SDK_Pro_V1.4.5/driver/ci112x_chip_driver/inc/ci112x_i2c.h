/**
 * @file ci112x_i2c.h
 * @brief CI112x芯片IIC模块的驱动程序头文件
 * @version 0.1
 * @date 2020-7-20
 *
 * @copyright Copyright (c) 2020  Chipintelli Technology Co., Ltd.
 *
 */
#ifndef __CI112X_I2C_H_
#define __CI112X_I2C_H_

#include "ci112x_system.h"
#include "ci112x_scu.h"

#ifdef __cplusplus
    extern "C"{
#endif


#define I2C_NAME_SIZE  20


#ifndef NULL
#define NULL  0
#endif

/*用于i2c传输数据总线忙时，等待的时间*/
typedef enum
{
	LONG_TIME_OUT  = 0X5FFFFF,
	SHORT_TIME_OUT = 0XFFFF,
}I2C_TimeOut;

struct i2c_algo_data
{
    void    *data;    /* private low level data */
    void (*write_byte)    (void *data, int32_t reg, int32_t val);
    int32_t (*read_byte)    (void *data, int32_t reg);
    int32_t (*wait_for_completion)    (void *data);
    void (*reset)    (void *data);
    /* i2c_errors values -- error types */
    uint32_t    i2c_errors;
    uint32_t    i2c_over;
};


struct i2c_data 
{
    volatile int32_t    irq;    /* if 0, use polling */
    volatile int32_t    wait;
    struct i2c_algo_data    algo_data;
    volatile uint32_t    io_base;
    volatile int32_t    timeout;
};

/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_i2c ci112x_i2c
 * @brief CI112X芯片IIC驱动，需要自定义协议时可配置i2c_masg、i2c_client结构体，再调用
 * i2c_master_transfer传输接口；仅读写数据就配置i2c_client结构体，再调用读写接口。
 * @{
 */
/**
 * @brief IIC传输协议初始化结构体
 * 
 */
struct i2c_msg 
{
    volatile uint16_t addr;    /* slave address */
    volatile uint16_t flags;
#define I2C_M_WRITE  0x0    /* write reg address of salave chip,or write instructions */
#define I2C_M_READ  0x1    /* read data, from slave to master */
    volatile uint16_t len;    /* msg length */
    volatile uint8_t *buf;    /* pointer to msg data */
};

/**
 * @brief IIC传输数据的SLAVE设备属性
 * 
 */
struct i2c_client
{
    uint16_t addr;                  /*!< SLAVE设备IIC总线地址，一般存于高7位 */
    char name[I2C_NAME_SIZE];     /*!< SLAVE设备名称 */
};
/**
 * @}
 */

#define I2C_TRANSMIT  0x1
#define I2C_RECEIVE  0x0
#define I2C_TRANS_OVER  0x1
#define I2C_TRANS_ON  0x0

#define IIC_SCLDIV  0x0
#define IIC_CMD  0x10
#define IIC_INTCLR  0x18
#define IIC_TXDR  0x20
#define IIC_RXDR  0x24
#define IIC_STATUS  0x2c

#define STATUS_TRANSMITTER (0x1 << 11)
#define STATUS_SEND_BYTE (0x1 << 2)
#define STATUS_NOACK (0x1 << 14)
#define STATUS_SLAVE_SELECTED (0x1 << 6)
#define STATUS_WRITE (0x1 << 13)
#define STATUS_RXDR_FULL (0x1 << 1)
#define STATUS_STOP (0x1 << 4)

#define CR_STOP  (0x1 << 3)
#define CR_START  (0x1 << 4)
#define CR_IRQ_CLEAR  (0x7f)
#define SR_BUSY  (0x1 << 15)
#define SR_ABITOR_LOST  (0x1 << 5)
#define SR_TRANS_ERR  (0x1 << 3)
#define SR_RXACK  (0x1 << 14)
#define SR_IRQ_FLAG  (0x1 << 12)
#define CR_NACK  (0x1 << 2)
#define CR_TB  (0x1 << 0)

#define CR_WRITE  0
#define CR_READ  0

#define WAKE_UP  0xacced
#define SLEEP  0x0

#define ERET  0x2

#define RXDUFLIE  (0x1 << 1)
#define TXDEPTIE  (0x1 << 2)
#define SSTOPIE  (0x1 << 4)
#define SLVADIE  (0x1 << 6)

/*之前均为0x20。现已更改*/
#define IIC0_SLAVEADDR  0x64    

/**
 * @ingroup ci112x_chip_driver
 * @defgroup ci112x_i2c ci112x_i2c
 * @brief CI112X芯片I2C驱动
 * @{
 */
void iic_with_os_int(struct i2c_data* iic_master);
void iic_int_event_group_init(void);
int32_t wait_event_interruptible(struct i2c_data *i2c);


int32_t i2c_master_init(I2C_TypeDef *I2Cx, uint32_t speed, I2C_TimeOut timeout);
int32_t i2c_slave_init(I2C_TypeDef *I2Cx, uint32_t speed, I2C_TimeOut timeout);
int32_t i2c_master_only_send(char slave_ic_address,const char *buf,int32_t count);
int32_t i2c_master_send_recv(char slave_ic_address,char *buf,int32_t send_len,int32_t rev_len);
int32_t i2c_master_transfer(struct i2c_data *adap, struct i2c_msg *msgs, int32_t num);
int32_t i2c_master_only_recv(char slave_ic_address,char *buf,int32_t rev_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /*__CI112X_I2C_H_*/
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/

