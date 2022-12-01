/**
 * @file ci112x_spiflash.c
 * @brief  SPIFLASH驱动文件
 * @version 0.1
 * @date 2019-03-27
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */

#include <string.h>
#include "ci112x_spiflash.h"
#include "ci112x_dma.h"
#include "ci112x_scu.h"
#include "ci112x_core_eclic.h"
#include "ci_assert.h"
#include "ci_log.h"
#include "sdk_default_config.h"
#include "platform_config.h"
#include "FreeRTOS.h"
#include "task.h"

#define DMAC_CHANNEL0_LOCK()          do{}while(0);
#define DMAC_CHANNEL0_UNLOCK()        do{}while(0);

#define SPIC_CLK                    (get_apb_clk())

#define ERASE_64K                   (64 * 1024)
#define ERASE_4K                    (4 * 1024)

#define FLASH_PAGE_SIZE             (256)
#define FLASH_SISZE_16M             (16 * 1024 * 1024)

#define CPU_MS                      (get_ipcore_clk() / 1000)
#define SPIC_MS                     (SPIC_CLK / 1000)

#define WRITE_STATUS_REGISTER_TIMR  (30UL)    /* ms */
#define PAGE_PROGRAMMING_TIMR       (3UL)     /* ms */
#define SECTOR_ERASE_4K_TIMR        (300UL)   /* ms */
#define BLOCK_ERASE_32K_TIMR        (1600UL)  /* ms */
#define BLOCK_ERASE_64K_TIMR        (2000UL)  /* ms */
#define CHIP_ERASE_TIMR             (60000UL) /* ms */
#define OTHER_TIMR                  (20UL)    /* ms */

#define STATUS_REG_DMA_BUF_SIZE (20)
#if (1)
#pragma data_alignment=8
static uint8_t spi_dma_used_buf[STATUS_REG_DMA_BUF_SIZE];/* DMA访问 */
static uint8_t spi_dma_buf[256];/* DMA访问 */
#else
static uint8_t *spi_dma_used_buf=NULL;
static uint8_t *spi_dma_buf=NULL;
#endif

/**
 * @brief SPI控制器寄存器结构体
 */
typedef struct
{
    volatile uint32_t spic_csr_00;/*!< offect:0x00; explain:status register wite time */
    volatile uint32_t spic_csr_01;/*!< offect:0x04; explain:set the spi bus mode */
    volatile uint32_t spic_csr_02;/*!< offect:0x08; explain:erase_counter_tap */
    volatile uint32_t spic_csr_03;/*!< offect:0x0C; explain:dma_en_hclk_status */
    volatile uint32_t spic_csr_04;/*!< offect:0x10; explain:type number */
    volatile uint32_t spic_csr_05;/*!< offect:0x14; explain:write/read number */
    volatile uint32_t spic_csr_06;/*!< offect:0x18; explain:flash command */
    volatile uint32_t spic_csr_07;/*!< offect:0x1C; explain:int ergister */
    volatile uint32_t spic_csr_08;/*!< offect:0x20; explain:dma request tap */
    volatile uint32_t spic_csr_09;/*!< offect:0x24; explain:flash write/read addr */
    volatile uint32_t spic_csr_10;/*!< offect:0x28; explain:program time parameter */
    volatile uint32_t spic_csr_11;/*!< offect:0x2C; explain:sector 4K erase time */
    volatile uint32_t spic_csr_12;/*!< offect:0x30; explain:block 32K erase time */
    volatile uint32_t spic_csr_13;/*!< offect:0x34; explain:block 64K erase time */
    volatile uint32_t spic_csr_14;/*!< offect:0x38; explain:chip erase time */
    volatile uint32_t spic_csr_15;/*!< offect:0x3C; explain:cs deselected time */
    volatile uint32_t spic_csr_16;/*!< offect:0x40; explain:powerdown or release time */
    volatile uint32_t spic_csr_17;/*!< offect:0x44; explain:DNN mode ctrl */
    volatile uint32_t spic_csr_18;/*!< offect:0x48; explain:M data ctrl */
    volatile uint32_t spic_csr_19;/*!< offect:0x4C; explain:Dummy wait count */
    volatile uint32_t spic_csr_20;/*!< offect:0x50; explain:first addr */
    volatile uint32_t spic_csr_21;/*!< offect:0x54; explain:end addr */
    volatile uint32_t spic_csr_22;/*!< offect:0x58; explain:start addr */
    volatile uint32_t spic_csr_23;/*!< offect:0x5C; explain:Reset count */
    volatile uint32_t spic_csr_24;/*!< offect:0x60; explain:AHB check ctrl */
    volatile uint32_t spic_csr_25;/*!< offect:0x64; explain:AHB1 mode int en */
    volatile uint32_t spic_csr_26;/*!< offect:0x68; explain:AHB1 mode int */
    volatile uint32_t spic_csr_27;/*!< offect:0x6C; explain:AHB1 mode int clear */
    volatile uint32_t spic_csr_28;/*!< offect:0x70; explain:AHB1 addr */
    volatile uint32_t spic_csr_29;/*!< offect:0x74; explain:state register */
    volatile uint32_t spic_csr_30;/*!< offect:0x78; explain:protect_en and protect first addr */
    volatile uint32_t spic_csr_31;/*!< offect:0x7C; explain:protect last addr */
    volatile uint32_t spic_csr_32;/*!< offect:0x80; explain:flash last addr */
    volatile uint32_t spic_csr_33;/*!< offect:0x84; explain:erase num */
}spic_register_t;

/**
 * @brief SPI传输模式定义
 */
typedef enum
{
    SPIC_BUSMODE_STD = 0,   /*!< 一线模式 */
    SPIC_BUSMODE_DUAL = 1,  /*!< 二线模式 */
    SPIC_BUSMODE_QUAD = 2,  /*!< 四线模式 */
}spic_busmode_t;

/**
 * @brief SPI时钟模式定义
 */
typedef enum
{
    SPIC_CLKMODE_0 = 0,     /*!< 时钟模式0 传输时为时钟低 */
    SPIC_CLKMODE_3 = 1,     /*!< 时钟模式3 传输时为时钟高*/
}spic_clkmode_t;

/**
 * @brief FASTREAD模式定义
 */
typedef enum
{
    SPIC_FASTREADIO_OUTPUT = 0,     /*!< only use io for read output */
    SPIC_FASTREADIO_ADDRDATA = 1,   /*!< use io for address and data */
}spic_fastreadio_t;

/**
 * @brief SPI控制器命令类型
 */
typedef enum
{
    SPIC_CMD_TYPE_PROGRAM                         = 0,
    SPIC_CMD_TYPE_WRSTATUSREG                     = 1,
    SPIC_CMD_TYPE_RDSTATUSREG                     = 2,
    SPIC_CMD_TYPE_SECTORERASE4K                    = 3,
    SPIC_CMD_TYPE_BLOCKERASE32K                    = 4,
    SPIC_CMD_TYPE_BLOCKERASE64K                    = 5,
    SPIC_CMD_TYPE_CHIPERASE                        = 6,
    SPIC_CMD_TYPE_POWERDOWN                        = 7,
    SPIC_CMD_TYPE_RELEASEPOWERDOWN                = 8,
    SPIC_CMD_TYPE_READSECURITYREG                 = 10,
    SPIC_CMD_TYPE_ERASESECURITYREG                = 11,
    SPIC_CMD_TYPE_WRSECURITYREG                    = 12,
    SPIC_CMD_TYPE_READ                            = 13,
    SPIC_CMD_TYPE_READMANUFACTURERID            = 14,
    SPIC_CMD_TYPE_READJEDECID                    = 15,
}spic_cmd_type_t;

/**
 * @brief SPI搬运模式定义
 */
typedef enum
{
    SPIC_MOVE_MODE_CPU = 0,/*!< CPU模式 */
    SPIC_MOVE_MODE_DMA = 1,/*!< DMA模式 */
}spic_move_mode_t;

/**
 * @brief DMA Request大小定义
 */
typedef enum
{
    DMA_REQUEST_1WORD    =0,/*!< 1个word发一次request */
    DMA_REQUEST_4WORD    =1,/*!< 4个word发一次request */
    DMA_REQUEST_8WORD    =2,/*!< 8个word发一次request */
    DMA_REQUEST_16WORD    =3,/*!< 16个word发一次request */
    DMA_REQUEST_32WORD    =4,/*!< 32个word发一次request */
    DMA_REQUEST_64WORD    =5,/*!< 64个word发一次request */
}dma_request_t;

/**
 * @brief SPI命令配置结构体
 */
typedef struct
{
    spic_busmode_t busmode;
    spic_clkmode_t clkmode;
    spic_fastreadio_t fastreadio;
    FunctionalState state;
    spic_cmd_code_t code;
    spic_cmd_type_t type;
    spic_move_mode_t move;
    dma_request_t request;
    uint32_t byte;
    uint32_t addr;
}spic_command_t;

/**
 * @brief SPI保护类型定义
 */
typedef enum{
    SPIC_SOFTWAREPROTECTION        =0,     /*!< 软件保护 */
    SPIC_HARDWAREPROTECTION        =1,     /*!< 保护由硬件决定 WP为低则保护 */
    SPIC_POWERSUPPLYLOCK_DOWN    =2,     /*!< 必须产生上电序列才能写 */
    SPIC_ONETIMEPROGRAM            =3,     /*!< 一次性编程保护，flash被永久性保护 */
    SPIC_RESV                    =-1,    /*!< 保留 */
}spic_status_protect_t;

/**
 * @brief 设置FLASH 写状态寄存器的时间
 *
 * @param spic spiflash控制器组
 * @param tw 时间
 */
static void spic_write_status_time(spic_base_t spic,uint32_t tw)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_00 = tw & 0xffffff;
}

/**
 * @brief 设置FLASH 编程一页的时间
 *
 * @param spic spiflash控制器组
 * @param tpp 时间
 */
static void spic_page_program_tpp(spic_base_t spic,uint32_t tpp)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_10 = tpp;
}

/**
 * @brief 设置FLASH 4K擦出时间
 *
 * @param spic spiflash控制器组
 * @param tce 时间
 */
static void spic_erase4k_tce(spic_base_t spic,uint32_t tce)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_11 = tce;
}

/**
 * @brief 设置FLASH 32K擦出时间
 *
 * @param spic spiflash控制器组
 * @param tce 时间
 */
static void spic_erase32k_tce(spic_base_t spic,uint32_t tce)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_12 = tce;
}

/**
 * @brief 设置FLASH 64K擦出时间
 *
 * @param spic spiflash控制器组
 * @param tce 时间
 */
static void spic_erase64k_tce(spic_base_t spic,uint32_t tce)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_13 = tce;
}

/**
 * @brief 设置FLASH 全片擦出时间
 *
 * @param spic spiflash控制器组
 * @param tce 时间
 */
static void spic_chiperase_tce(spic_base_t spic,uint32_t tce)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_14 = tce;
}

/**
 * @brief 设置FLASH cs 时间
 *
 * @param spic spiflash控制器组
 * @param tcsh1 读操作取消CS时间
 * @param tcsh2 擦除/写操作取消CS时间
 */
static void spic_cs_time(spic_base_t spic,uint32_t tcsh1,uint32_t tcsh2)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_15 = (tcsh1 << 5) | (tcsh2 << 0);
}

/**
 * @brief 设置FLASH powerdown 时间
 *
 * @param spic spiflash控制器组
 * @param tres 释放Power Down 时间
 * @param tdp cs取消到power down时间
 */
static void spic_powerdown_time(spic_base_t spic,uint32_t tres,uint32_t tdp)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_16 = (tres << 16) | (tdp << 0);
}

/**
 * @brief 设置SPI总线模式
 *
 * @param spic spiflash控制器组
 * @param busmode 总线模式
 */
static void spic_busmode(spic_base_t spic,spic_busmode_t busmode)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_01 = busmode;
}

/**
 * @brief 设置SPI协议模式
 *
 * @param spic spiflash控制器组
 * @param clkmode 协议模式
 */
static void spic_clkmode(spic_base_t spic,spic_clkmode_t clkmode)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    uint32_t temp = spic_p->spic_csr_04;
    temp &= ~(0x1 << 2);
    temp |= (clkmode << 2);
    spic_p->spic_csr_04 = temp;
}

/**
 * @brief 设置SPI fastreadio
 *
 * @param spic spiflash控制器组
 * @param fastreadio io模式
 */
static void spic_fastreadio(spic_base_t spic,spic_fastreadio_t fastreadio)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    uint32_t temp = spic_p->spic_csr_04;
    temp &= ~(0x1 << 1);
    temp |= (fastreadio << 1);
    spic_p->spic_csr_04 = temp;
}

/**
 * @brief 设置SPI fastread 使能
 *
 * @param spic spiflash控制器组
 * @param state 使能开关
 */
static void spic_fastread(spic_base_t spic,FunctionalState state)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    if(ENABLE == state)
    {
        spic_p->spic_csr_04 |= (0x1 << 0);
    }
    else if(DISABLE == state)
    {
        spic_p->spic_csr_04 &= ~(0x1 << 0);
    }
}

/**
 * @brief 设置SPI读写字节数
 *
 * @param spic spiflash控制器组
 * @param bytes 读写字节数
 */
static void spic_writeread_bytes(spic_base_t spic,uint32_t bytes)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_05 = bytes;
}

/**
 * @brief 设置SPI读写FLASH地址
 *
 * @param spic spiflash控制器组
 * @param addr 读写地址
 */
static void spic_writeread_addr(spic_base_t spic,uint32_t addr)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_09 = addr;
}

/**
 * @brief 设置SPI命令
 *
 * @param spic spiflash控制器组
 * @param code 命令
 * @param type 命令类型
 * @param move 读写方式
 */
static void spic_command_config(spic_base_t spic,spic_cmd_code_t code,
                                spic_cmd_type_t type,spic_move_mode_t move)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    uint32_t temp = spic_p->spic_csr_06;
    temp &= ~((0xFF << 6) | (0x1 << 5) | (0xF << 1));
    temp |= ((code << 6) | (move << 5) | (type << 1));
    spic_p->spic_csr_06 = temp;
}

/**
 * @brief 等待命令有效
 *
 * @param spic spiflash控制器组
 * @param timeout 超时时间
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_wait_command_vaild(spic_base_t spic,int32_t timeout)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    while((spic_p->spic_csr_06 & (0x1 << 0)) && timeout--);
    if(0 >= timeout)
    {
        return RETURN_ERR;
    }
    else
    {
        return RETURN_OK;
    }
}

/**
 * @brief 命令使能
 *
 * @param spic spiflash控制器组
 */
static void spic_command_vaild(spic_base_t spic)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_06 |= (0x1 << 0);
}

/**
 * @brief 设置SPI向DMA发出请求的字节数
 *
 * @param spic spiflash控制器组
 * @param count 字节数
 */
static void spic_dma_request(spic_base_t spic,dma_request_t count)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    spic_p->spic_csr_08 = count;
}

/**
 * @brief SPI发送命令
 *
 * @param spic spiflash控制器组
 * @param command 命令结构体指针
 */
static void spic_send_command(spic_base_t spic,spic_command_t* command)
{
    spic_busmode(spic,command->busmode);
    spic_clkmode(spic,command->clkmode);
    spic_fastreadio(spic,command->fastreadio);
    spic_fastread(spic,command->state);
    spic_writeread_bytes(spic,command->byte);
    spic_writeread_addr(spic,command->addr);
    spic_command_config(spic,command->code,command->type,command->move);
    spic_dma_request(spic,command->request);
    spic_command_vaild(spic);
}

/**
 * @brief 结构体初始化
 *
 * @param config dma结构体指针
 * @param command 命令结构体指针
 */
static void spic_struct_init(dma_config_t* config,spic_command_t* command)
{
    if(NULL != config)
    {
        config->flowctrl = P2M_DMA;
        config->busrtsize = BURSTSIZE1;
        config->transferwidth = TRANSFERWIDTH_32b;
        config->srcaddr = SPI0FIFO_BASE;
        config->destaddr = SPI0FIFO_BASE;
        config->transfersize = 1;
    }
    if(NULL != command)
    {
        command->busmode = SPIC_BUSMODE_STD;
        command->clkmode = SPIC_CLKMODE_0;
        command->fastreadio = SPIC_FASTREADIO_OUTPUT;
        command->state = DISABLE;
        command->byte = 1;
        command->addr = 0;
        command->code = SPIC_CMD_CODE_READSTATUSREG1;
        command->type = SPIC_CMD_TYPE_RDSTATUSREG;
        command->move = SPIC_MOVE_MODE_DMA;
        command->request = DMA_REQUEST_1WORD;
    }
}

#if FLASH_MSF
/**
 * @brief 读扩展地址寄存器
 *
 * @param spic spiflash控制器组
 * @param cmd 命令
 * @param reg 寄存器值(flash地址的（24 ~ 31）bit,用来访问16M以上的FLASH)
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_read_extend_addr_register(spic_base_t spic,spic_cmd_code_t cmd,uint8_t* reg)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;

    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = (uint32_t)spi_dma_used_buf;
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = cmd;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    *reg = spi_dma_used_buf[0];
    return RETURN_OK;
}

/**
 * @brief 写扩展地址寄存器
 *
 * @param spic spiflash控制器组
 * @param reg 寄存器值(flash地址的（24 ~ 31）bit,用来访问16M以上的FLASH)
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_write_extend_addr_register(spic_base_t spic,uint8_t reg)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;

    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = M2P_DMA;
    spi_dma_used_buf[0] = reg;
    config.destaddr = (uint32_t)spi_dma_used_buf;
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_WRITE_EXTENDED_ADDR_REG;
    command.type = SPIC_CMD_TYPE_WRSTATUSREG;
    command.byte = 1;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    return RETURN_OK;
}
#endif

/**
 * @brief 读取状态寄存器
 *
 * @param spic spiflash控制器组
 * @param reg 状态寄存器
 * @param status 读取到的状态值
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_read_status_register(spic_base_t spic,spic_cmd_code_t reg,uint8_t *status)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;

    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = (uint32_t)spi_dma_used_buf;
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = reg;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    *status = spi_dma_used_buf[0];
    return RETURN_OK;
}


/**
 * @brief 写状态寄存器
 *
 * @param spic spiflash控制器组
 * @param reg1 状态寄存器1的值
 * @param reg2 状态寄存器2的值
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_write_status_register(spic_base_t spic,char reg1,char reg2)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;

    spi_dma_used_buf[0] = reg1;
    spi_dma_used_buf[1] = reg2;

    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = M2P_DMA;
    config.srcaddr = (uint32_t)spi_dma_used_buf;
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_WRSTATUSREG;
    command.type = SPIC_CMD_TYPE_WRSTATUSREG;
    command.byte = 2;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,WRITE_STATUS_REGISTER_TIMR *
                                             CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,
                                             WRITE_STATUS_REGISTER_TIMR *
                                               CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    /*配置DMA*/
    config.flowctrl = M2P_DMA;
    spi_dma_used_buf[0] = reg2;
    config.srcaddr = (uint32_t)spi_dma_used_buf;/*align*/
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_WRSTATUSREG2;
    command.type = SPIC_CMD_TYPE_WRSTATUSREG;
    command.byte = 1;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,WRITE_STATUS_REGISTER_TIMR *
                                             CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,
                                             WRITE_STATUS_REGISTER_TIMR *
                                               CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    return RETURN_OK;
}

/**
 * @brief 读取Unique ID
 *
 * @param spic spiflash控制器组
 * @param unique 华邦：64bit，GD：128bit
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_read_unique_id(spic_base_t spic,uint8_t* unique)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;

    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = (uint32_t)spi_dma_used_buf;
    config.transfersize = 5;
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_READ_UNIQUE_ID;
    command.byte = 20;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    memcpy((void*)unique,(void*)&spi_dma_used_buf[4],16);
    return RETURN_OK;
}

/**
 * @brief 读取jedec ID
 *
 * @param spic spiflash控制器组
 * @param jedec
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_read_jedec_id(spic_base_t spic,uint8_t* jedec)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;

    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = (uint32_t)spi_dma_used_buf;
    config.transfersize = 1;
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_READJEDECID;
    command.type = SPIC_CMD_TYPE_READJEDECID;
    command.byte = 3;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    memcpy((void*)jedec,(void*)spi_dma_used_buf,3);
    return RETURN_OK;
}


/**
 * @brief 检查BUSY状态
 *
 * @param spic spiflash控制器组
 * @param timeout 超时时间
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_check_busy(spic_base_t spic,int32_t timeout)
{
    uint8_t status;
    do
    {
        if(RETURN_ERR == spic_read_status_register(spic,
                                                   SPIC_CMD_CODE_READSTATUSREG1,&status))
        {
            return RETURN_ERR;
        }
    } while ((status & (0x1 << 0)) && timeout--);
    if(0 >= timeout)
    {
        return RETURN_ERR;
    }
    else
    {
        return RETURN_OK;
    }
}


/**
 * @brief 设置FLASH四线模式
 *
 * @param spic spiflash控制器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_quad_mode(spic_base_t spic)
{
    uint8_t status;
    uint8_t reg1,reg2;
    if(RETURN_ERR == spic_read_status_register(spic,
                                               SPIC_CMD_CODE_READSTATUSREG1,&status))
    {
        return RETURN_ERR;
    }
    reg1 = status;
    if(RETURN_ERR == spic_read_status_register(spic,
                                               SPIC_CMD_CODE_READSTATUSREG2,&status))
    {
        return RETURN_ERR;
    }
    reg2 = status;
    if(0 == (reg2 & (0x1 << 1)))
    {
        reg2 |= (0x1 << 1);
        if(RETURN_ERR == spic_write_status_register(spic,reg1,reg2))
        {
            return RETURN_ERR;
        }
    }
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief powerdown
 *
 * @param spic spiflash控制器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_powerdown(spic_base_t spic)
{
    volatile int32_t timeout;
    /*结构体初始化*/
    spic_command_t command;
    spic_struct_init(NULL,&command);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_POWERDOWN;
    command.type = SPIC_CMD_TYPE_POWERDOWN;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    timeout = 0x5FFF;
    while(timeout--);
    return RETURN_OK;
}

/**
 * @brief releasepowerdown
 *
 * @param spic spiflash控制器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_releasepowerdown(spic_base_t spic)
{
    volatile int32_t timeout;
    /*结构体初始化*/
    spic_command_t command;
    spic_struct_init(NULL,&command);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_RELEASEPOWERDOWN;
    command.type = SPIC_CMD_TYPE_RELEASEPOWERDOWN;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    timeout = 0x5FFF;
    while(timeout--);
    return RETURN_OK;
}

/**
 * @brief reset flash
 *
 * @param spic spiflash控制器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_reset(spic_base_t spic)
{
    volatile int32_t timeout;
    /*结构体初始化*/
    spic_command_t command;
    spic_struct_init(NULL,&command);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_ENABLERESET;
    command.type = SPIC_CMD_TYPE_POWERDOWN;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    command.code = SPIC_CMD_CODE_RESET;
    command.type = SPIC_CMD_TYPE_RELEASEPOWERDOWN;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    timeout = 0x3D70;
    while(timeout--);
    return RETURN_OK;
}

/**
 * @brief SPIflash保护设置
 *
 * @param spic spiflash控制器
 * @param cmd 保护开关
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_protect(spic_base_t spic,FunctionalState cmd)
{
    uint8_t status_reg;
    int32_t reg1 = 0,reg2 = 0,srp1 = 0,srp0 = 0,ret = 0;
    volatile spic_status_protect_t status = SPIC_RESV;

    if(RETURN_ERR == spic_read_status_register(spic,
                                               SPIC_CMD_CODE_READSTATUSREG1,&status_reg))
    {
        return RETURN_ERR;
    }
    reg1 = status_reg;
    if(RETURN_ERR == spic_read_status_register(spic,
                                               SPIC_CMD_CODE_READSTATUSREG2,&status_reg))
    {
        return RETURN_ERR;
    }
    reg2 = status_reg;

    srp0 = (reg1 &(1 << 7))?1:0;
    srp1 = (reg2 &(1 << 0))?1:0;
    status =(spic_status_protect_t)((srp1 << 1) | (srp0 << 0));

    switch (status)
    {
        case SPIC_SOFTWAREPROTECTION:
             ret=0;/*可以正常写，必须发写使能后*/
            break;
        case SPIC_HARDWAREPROTECTION:
            ret=1;/*硬件保护控制，如果/WP 引脚为低电平，不能写*/
            break;
        case SPIC_POWERSUPPLYLOCK_DOWN:
            ret=2;/*直到下一个 power-down power-up时序后，才能被写*/
            if(RETURN_ERR == spic_powerdown(spic))
            {
                return RETURN_ERR;
            }
            if(RETURN_ERR == spic_releasepowerdown(spic))
            {
                return RETURN_ERR;
            }
            break;
        case SPIC_ONETIMEPROGRAM:
            ret=3;/*一次性编程保护，不能再写*/
            return ret;
        default:
            ret=0XFF;
            break;
    }
    if(cmd != ENABLE)
    {
          reg1 &= ~(0x7 << 2);//block protec disable
          reg1 &= ~(0x1 << 5);//Top/Bottom Block protect disbale
          reg1 &= ~(0x1 << 6);//Sector /SEC
          reg2 &= ~(0x1 << 6);//CMP =0
          reg2 &= ~(0x7 << 3);//BP2,BP1,BP0
    }
    else
    {
          reg1 &= ~(0x7 << 2);
          reg1 |= (0x5 << 2);//保护前3M
          reg2 |= (0x1 << 6);//CMP =1
    }
    if(RETURN_ERR == spic_write_status_register(spic,reg1,reg2))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return ret;
}

/**
 * @brief 根据size得到最适合的burst
 *
 * @param size 传输字节数
 *
 * @return BURSTSIZEx burst大小
 */
static BURSTSIZEx spic_burst_adjust(uint32_t size)
{
    BURSTSIZEx busrtsize = BURSTSIZE1;
    if(0 == (size % 256))
    {
        busrtsize = BURSTSIZE64;
    }
    else if(0 == (size % 128))
    {
        busrtsize = BURSTSIZE32;
    }
    else if(0 == (size % 64))
    {
        busrtsize = BURSTSIZE16;
    }
    else if(0 == (size % 32))
    {
        busrtsize = BURSTSIZE8;
    }
    else if(0 == (size % 16))
    {
        busrtsize = BURSTSIZE4;
    }
    return busrtsize;
}

/**
 * @brief 根据size得到最适合的dma request size
 *
 * @param size 传输字节数
 *
 * @return dma_request_t request大小
 */
static dma_request_t spic_request_adjust(uint32_t size)
{
    dma_request_t requestsize = DMA_REQUEST_1WORD;
    if(0 == (size % 256))
    {
        requestsize = DMA_REQUEST_64WORD;
    }
    else if(0 == (size % 128))
    {
        requestsize = DMA_REQUEST_32WORD;
    }
    else if(0 == (size % 64))
    {
        requestsize = DMA_REQUEST_16WORD;
    }
    else if(0 == (size % 32))
    {
        requestsize = DMA_REQUEST_8WORD;
    }
    else if(0 == (size % 16))
    {
        requestsize = DMA_REQUEST_4WORD;
    }
    return requestsize;
}

/**
 * @brief 擦除FLASH安全寄存器
 *
 * @param spic spiflash控制器
 * @param reg 擦除寄存器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_erase_security_reg(spic_base_t spic,spic_security_reg_t reg)
{
    uint32_t addr = 0;
    switch (reg)
    {
        case SPIC_SECURITY_REG1:
            addr = 0x1000;
            break;
        case SPIC_SECURITY_REG2:
            addr = 0x2000;
            break;
        case SPIC_SECURITY_REG3:
            addr = 0x3000;
            break;
        default:
            break;
    }
    /*结构体初始化*/
    spic_command_t command;
    spic_struct_init(NULL,&command);
    /*发送命令*/
    command.code = SPIC_CMD_CODE_ERASE_SECURITY_REG;
    command.type = SPIC_CMD_TYPE_ERASESECURITYREG;
    command.addr = addr;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 写FLASH的安全寄存器
 *
 * @param spic spiflash控制器
 * @param reg 擦除寄存器
 * @param buf mem地址
 * @param addr FLASH寄存器地址：华邦(0 - 256),GD(0 - 1024)
 * @param size 写FLASH的字节数：华邦(0 - 256),GD(0 - 1024)
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_write_security_reg(spic_base_t spic,spic_security_reg_t reg,
                              uint32_t buf,uint32_t addr,uint32_t size)
{
    switch (reg)
    {
        case SPIC_SECURITY_REG1:
            addr |= 0x1000;
            break;
        case SPIC_SECURITY_REG2:
            addr |= 0x2000;
            break;
        case SPIC_SECURITY_REG3:
            addr |= 0x3000;
            break;
        default:
            break;
    }
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;
    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = M2P_DMA;
    config.srcaddr = buf;
    config.transfersize = size / 4;
    config.busrtsize = spic_burst_adjust(size);
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.busmode = SPIC_BUSMODE_STD;
    command.state = ENABLE;
    command.code = SPIC_CMD_CODE_WRITE_SECURITY_REG;
    command.type = SPIC_CMD_TYPE_WRSECURITYREG;
    command.addr = addr;
    command.byte = size;
    command.request = spic_request_adjust(size);
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 读FLASH的安全寄存器
 *
 * @param spic spiflash控制器
 * @param reg 擦除寄存器
 * @param buf mem地址
 * @param addr FLASH寄存器地址：华邦(0 - 256),GD(0 - 1024)
 * @param size 写FLASH的字节数：华邦(0 - 256),GD(0 - 1024)
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_read_security_reg(spic_base_t spic,spic_security_reg_t reg,
                              uint32_t buf,uint32_t addr,uint32_t size)
{
    switch (reg)
    {
        case SPIC_SECURITY_REG1:
            addr |= 0x1000;
            break;
        case SPIC_SECURITY_REG2:
            addr |= 0x2000;
            break;
        case SPIC_SECURITY_REG3:
            addr |= 0x3000;
            break;
        default:
            break;
    }
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;
    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = buf;
    config.transfersize = size / 4;
    config.busrtsize = spic_burst_adjust(size);
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.busmode = SPIC_BUSMODE_STD;
    command.state = ENABLE;
    command.code = SPIC_CMD_CODE_READ_SECURITY_REG;
    command.type = SPIC_CMD_TYPE_READSECURITYREG;
    command.addr = addr;
    command.byte = size;
    command.request = spic_request_adjust(size);
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief FLASH的安全寄存器上锁,慎用:上锁之后将导致该安全寄存器不可再此编程
 *
 * @param spic spiflash控制器
 * @param reg 寄存器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_security_reg_lock(spic_base_t spic,spic_security_reg_t reg)
{
    uint8_t status;
    uint32_t lock = 0;
    switch (reg)
    {
        case SPIC_SECURITY_REG1:
            lock = 0x1 << 3;
            break;
        case SPIC_SECURITY_REG2:
            lock = 0x1 << 4;
            break;
        case SPIC_SECURITY_REG3:
            lock = 0x1 << 5;
            break;
        default:
            break;
    }
    int32_t reg1,reg2;
    if(RETURN_ERR == spic_read_status_register(spic,
                                               SPIC_CMD_CODE_READSTATUSREG1,&status))
    {
        return RETURN_ERR;
    }
    reg1 = status;
    if(RETURN_ERR == spic_read_status_register(spic,
                                               SPIC_CMD_CODE_READSTATUSREG2,&status))
    {
        return RETURN_ERR;
    }
    reg2 = status;
    reg2 |= lock;
    if(RETURN_ERR == spic_write_status_register(spic,reg1,reg2))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 擦除FLASH
 *
 * @param spic spiflash控制器
 * @param code 擦除命令
 * @param addr 擦除地址
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_erase(spic_base_t spic,spic_cmd_code_t code,uint32_t addr)
{
    spic_cmd_type_t type;
    int32_t timeout = 0;
    switch (code)
    {
        case SPIC_CMD_CODE_SECTORERASE4K:
            type = SPIC_CMD_TYPE_SECTORERASE4K;
            timeout = SECTOR_ERASE_4K_TIMR * CPU_MS;
            break;
        case SPIC_CMD_CODE_BLOCKERASE64K:
            type = SPIC_CMD_TYPE_BLOCKERASE64K;
            timeout = BLOCK_ERASE_64K_TIMR * CPU_MS;
            break;
        default:
            type = SPIC_CMD_TYPE_CHIPERASE;
            timeout = CHIP_ERASE_TIMR * CPU_MS; // TODO:BUG!!! ALREADY WARNING
            break;
    }
    /*结构体初始化*/
    spic_command_t command;
    spic_struct_init(NULL,&command);
    /*发送命令*/
    command.code = code;
    command.type = type;
    command.addr = addr;
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,timeout))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_check_busy(spic,timeout))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 写FLASH的某一页
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr FLASH地址
 * @param size 写FLASH的字节数
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_quad_write_page(spic_base_t spic,uint32_t buf,uint32_t addr,
                                    uint32_t size)
{
    if(RETURN_ERR == spic_quad_mode(spic))
    {
        return RETURN_ERR;
    }
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;
    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = M2P_DMA;
    config.srcaddr = buf;
    config.transfersize = size / 4;
    config.busrtsize = spic_burst_adjust(size);
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.busmode = SPIC_BUSMODE_QUAD;
    command.state = ENABLE;
    command.code = SPIC_CMD_CODE_QUADINPUTPAGEPROGRAM;
    command.type = SPIC_CMD_TYPE_PROGRAM;
    command.addr = addr;
    command.byte = size;
    command.request = spic_request_adjust(size);
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,
                                             PAGE_PROGRAMMING_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,
                                             PAGE_PROGRAMMING_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 写FLASH的某一页（单线）
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr FLASH地址
 * @param size 写FLASH的字节数
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_std_write_page(spic_base_t spic,uint32_t buf,uint32_t addr,
                                    uint32_t size)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;
    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = M2P_DMA;
    config.srcaddr = buf;
    config.transfersize = size / 4;
    config.busrtsize = spic_burst_adjust(size);
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.busmode = SPIC_BUSMODE_STD;
    command.state = ENABLE;
    command.code = SPIC_CMD_CODE_PAGEPROGRAM;
    command.type = SPIC_CMD_TYPE_PROGRAM;
    command.addr = addr;
    command.byte = size;
    command.request = spic_request_adjust(size);
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,
                                             PAGE_PROGRAMMING_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,
                                             PAGE_PROGRAMMING_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 读FLASH的某一块
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr FLASH地址
 * @param size 读FLASH的字节数
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t spic_quad_read_page(spic_base_t spic,uint32_t buf,uint32_t addr,
                                    uint32_t size)
{
    //mprintf("Flash Read 0\n");
    if(RETURN_ERR == spic_quad_mode(spic))
    {
        return RETURN_ERR;
    }
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;
    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = buf;
    config.transfersize = size / 4;
    config.busrtsize = spic_burst_adjust(size);
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.busmode = SPIC_BUSMODE_QUAD;
    command.state = ENABLE;
    command.code = SPIC_CMD_CODE_FASTREADQUADOUTPUT;
    command.type = SPIC_CMD_TYPE_READ;
    command.addr = addr;
    command.byte = size;
    command.request = spic_request_adjust(size);
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief 读FLASH的某一块（单线）
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr FLASH地址
 * @param size 读FLASH的字节数
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t spic_std_read_page(spic_base_t spic,uint32_t buf,uint32_t addr,
                                    uint32_t size)
{
    /*结构体初始化*/
    spic_command_t command;
    dma_config_t config;
    spic_struct_init(&config,&command);
    /*配置DMA*/
    config.flowctrl = P2M_DMA;
    config.destaddr = buf;
    config.transfersize = size / 4;
    config.busrtsize = spic_burst_adjust(size);
    DMAC_CHANNEL0_LOCK();
    spic_dma_config(DMACChannel0,&config);
    /*发送命令*/
    command.busmode = SPIC_BUSMODE_STD;
    command.state = ENABLE;
    command.code = SPIC_CMD_CODE_FASTREAD;
    command.type = SPIC_CMD_TYPE_READ;
    command.addr = addr;
    command.byte = size;
    command.request = spic_request_adjust(size);
    spic_send_command(spic,&command);
    if(RETURN_ERR == spic_wait_command_vaild(spic,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == wait_dma_translate_flag(DMACChannel0,OTHER_TIMR * CPU_MS))
    {
        return RETURN_ERR;
    }
    DMAC_CHANNEL0_UNLOCK();
    if(RETURN_ERR == spic_check_busy(spic,OTHER_TIMR * SPIC_MS))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief FLASH初始化
 *
 * @param spic spiflash控制器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t flash_init(spic_base_t spic)
{
    /*no free!!!*/
    #if 0//CONFIG_DIRVER_BUF_USED_FREEHEAP_EN
    if(NULL == spi_dma_used_buf)
    {
        spi_dma_used_buf = (uint8_t *)pvPortMalloc(STATUS_REG_DMA_BUF_SIZE);
        if(NULL == spi_dma_used_buf)
        {
            return RETURN_ERR;
        }
    }
    if(NULL == spi_dma_buf)
    {
        spi_dma_buf = (uint8_t *)pvPortMalloc(256);
        if(NULL == spi_dma_buf)
        {
            return RETURN_ERR;
        }
    }
    #endif
    Scu_Setdevice_Reset(HAL_GDMA_BASE);
    Scu_Setdevice_ResetRelease(HAL_GDMA_BASE);
    Scu_SetDeviceGate(HAL_GDMA_BASE,ENABLE);
    eclic_irq_enable(GDMA_IRQn);
    clear_dma_translate_flag(DMACChannel0);
    Scu_Setdevice_Reset((uint32_t)spic);
    Scu_Setdevice_ResetRelease((uint32_t)spic);

    Scu_SetDeviceGate((uint32_t)spic,DISABLE);
    Scu_Setdiv_Parameter((uint32_t)spic,2);
    Scu_SetDeviceGate((uint32_t)spic,ENABLE);
    scu_spiflash_noboot_set();
    spic_write_status_time(spic,0); //2019/9/10,兼容"普冉”低功耗flash
    spic_page_program_tpp(spic,0);
    spic_erase4k_tce(spic,0);
    spic_erase32k_tce(spic,0);
    spic_erase64k_tce(spic,0);
    spic_chiperase_tce(spic,0);
    spic_cs_time(spic,0,0);
    spic_powerdown_time(spic,0,0);
    if(RETURN_ERR == spic_powerdown(spic))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_releasepowerdown(spic))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_reset(spic))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == spic_protect(spic,ENABLE))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
}

/**
 * @brief FLASH擦除
 *
 * @param spic spiflash控制器
 * @param addr 地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t flash_policy_erase(spic_base_t spic,uint32_t addr,uint32_t size)
{
    /* other erase */
    uint32_t current_addr = addr;
    uint32_t current_size = size;
    do
    {
        if((0 == (current_addr % ERASE_64K)) && (size >= ERASE_64K))
        {
            current_size = ERASE_64K;
            if(RETURN_ERR == spic_erase(spic,SPIC_CMD_CODE_BLOCKERASE64K,
                                        current_addr))
            {
                return RETURN_ERR;
            }
        }
        else if((0 == (current_addr % ERASE_4K)) && (size >= ERASE_4K))
        {
            current_size = ERASE_4K;
            if(RETURN_ERR == spic_erase(spic,SPIC_CMD_CODE_SECTORERASE4K,
                                        current_addr))
            {
                return RETURN_ERR;
            }
        }
        else
        {
            return RETURN_ERR;
        }
        size -= current_size;
        current_addr += current_size;
    }
    while(size > 0);
    return RETURN_OK;
}

/**
 * @brief spiflash 写规则
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr flash地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t flash_write_rule(spic_base_t spic,uint32_t buf,uint32_t addr,
                                uint32_t size)
{
    uint32_t write_64word = 64 * 4,write_64word_count = 0;
    uint32_t write_32word = 32 * 4,write_32word_count = 0;
    uint32_t write_16word = 16 * 4,write_16word_count = 0;
    uint32_t write_8word  = 8 * 4,write_8word_count = 0;
    uint32_t write_4word  = 4 * 4,write_4word_count = 0;
    uint32_t write_1word  = 4,write_1word_count = 0;
    write_64word_count = size / write_64word;
    write_32word_count = (size % write_64word) / write_32word;
    write_16word_count = (size % write_32word) / write_16word;
    write_8word_count = (size % write_16word) / write_8word;
    write_4word_count = (size % write_8word) / write_4word;
    write_1word_count = (size % write_4word) / write_1word;
    for(int i = 0;i < write_64word_count;i++)
    {
        if(RETURN_ERR == spic_quad_write_page(spic,buf,addr,write_64word))
        {
            return RETURN_ERR;
        }
        buf += write_64word;
        addr += write_64word;
    }
    for(int i = 0;i < write_32word_count;i++)
    {
        if(RETURN_ERR == spic_quad_write_page(spic,buf,addr,write_32word))
        {
            return RETURN_ERR;
        }
        buf += write_32word;
        addr += write_32word;
    }
    for(int i = 0;i < write_16word_count;i++)
    {
        if(RETURN_ERR == spic_quad_write_page(spic,buf,addr,write_16word))
        {
            return RETURN_ERR;
        }
        buf += write_16word;
        addr += write_16word;
    }
    for(int i = 0;i < write_8word_count;i++)
    {
        if(RETURN_ERR == spic_quad_write_page(spic,buf,addr,write_8word))
        {
            return RETURN_ERR;
        }
        buf += write_8word;
        addr += write_8word;
    }
    for(int i = 0;i < write_4word_count;i++)
    {
        if(RETURN_ERR == spic_quad_write_page(spic,buf,addr,write_4word))
        {
            return RETURN_ERR;
        }
        buf += write_4word;
        addr += write_4word;
    }
    for(int i = 0;i < write_1word_count;i++)
    {
        if(RETURN_ERR == spic_quad_write_page(spic,buf,addr,write_1word))
        {
            return RETURN_ERR;
        }
        buf += write_1word;
        addr += write_1word;
    }
    return RETURN_OK;
}

/**
 * @brief 写FLASH
 *
 * @param spic spiflash控制器
 * @param addr FLASH地址
 * @param buf mem地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t flash_quad_write(spic_base_t spic,uint32_t addr,uint32_t buf,
                         uint32_t size)
{
    uint32_t size_word = 0;
    uint32_t page_size_head = 0,page_size_tail = 0,page_count = 0;
    page_size_head = addr % FLASH_PAGE_SIZE;
    if(0 != page_size_head)
    {
        page_size_head = FLASH_PAGE_SIZE - page_size_head;
    }
    if(page_size_head > size)
    {
        page_size_head = size;
    }
    page_size_tail = ((size - page_size_head) % FLASH_PAGE_SIZE);
    page_count = ((size - page_size_head) / FLASH_PAGE_SIZE);
    if(0 != page_size_head)
    {
        memcpy((void*)spi_dma_buf,(void*)buf,page_size_head);
        if(page_size_head % 4)
        {
            size_word = ((page_size_head + 4) / 4) * 4;
        }
        else
        {
            size_word = page_size_head;
        }
        for(int i = 0;i < (size_word - page_size_head);i++)
        {
            spi_dma_buf[page_size_head + i] = 0xFF;
        }
        if(RETURN_ERR == flash_write_rule(spic,(uint32_t)spi_dma_buf,addr,size_word))
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi write error\n");
            return RETURN_ERR;
        }
        buf += page_size_head;
        addr += page_size_head;
    }
    for(int i = 0;i < page_count;i++)
    {
        memcpy((void*)spi_dma_buf,(void*)buf,FLASH_PAGE_SIZE);
        if(RETURN_ERR == flash_write_rule(spic,(uint32_t)spi_dma_buf,addr,FLASH_PAGE_SIZE))
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi write error\n");
            return RETURN_ERR;
        }
        buf += FLASH_PAGE_SIZE;
        addr += FLASH_PAGE_SIZE;
    }
    if(0 != page_size_tail)
    {
        memcpy((void*)spi_dma_buf,(void*)buf,page_size_tail);
        if(page_size_tail % 4)
        {
            size_word = ((page_size_tail + 4) / 4) * 4;
        }
        else
        {
            size_word = page_size_tail;
        }
        for(int i = 0;i < (size_word - page_size_tail);i++)
        {
            spi_dma_buf[page_size_tail + i] = 0xFF;
        }
        if(RETURN_ERR == flash_write_rule(spic,(uint32_t)spi_dma_buf,addr,size_word))
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi write error\n");
            return RETURN_ERR;
        }
    }
    return RETURN_OK;
}

/**
 * @brief 读FLASH
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr FLASH地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
static int32_t flash_quad_read(spic_base_t spic,uint32_t buf,uint32_t addr,
                        uint32_t size)
{
    uint32_t size_word = 0;
    uint32_t block_size_tail = 0,block_count = 0;
    block_size_tail = size % FLASH_PAGE_SIZE;
    block_count = size / FLASH_PAGE_SIZE;

    for(int i = 0;i < block_count;i++)
    {
        if(RETURN_ERR == spic_quad_read_page(spic,(uint32_t)spi_dma_buf,addr,
                                              FLASH_PAGE_SIZE))
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi read error\n");
            return RETURN_ERR;
        }
        memcpy((void*)buf,(void*)spi_dma_buf,FLASH_PAGE_SIZE);
        buf += FLASH_PAGE_SIZE;
        addr += FLASH_PAGE_SIZE;
    }
    if(0 != block_size_tail)
    {
        if(block_size_tail % 4)
        {
            size_word = ((block_size_tail + 4) / 4) * 4;
        }
        else
        {
            size_word = block_size_tail;
        }
        if(RETURN_ERR == spic_quad_read_page(spic,(uint32_t)spi_dma_buf,addr,size_word))
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi read error\n");
            return RETURN_ERR;
        }
        memcpy((void*)buf,(void*)spi_dma_buf,block_size_tail);
    }
    return RETURN_OK;
}

/**
 * @brief FLASH擦除,兼容32M以上Flash
 *
 * @param spic spiflash控制器
 * @param addr 地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t flash_erase(spic_base_t spic,uint32_t addr,uint32_t size)
{
#if FLASH_MSF
    uint32_t extend_addr,flash_addr;
    extend_addr = addr >> 24;
    flash_addr = 0xFFFFFF & addr;
    if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
    {
        return RETURN_ERR;
    }
    if(RETURN_ERR == flash_policy_erase(spic,flash_addr,size))
    {
        return RETURN_ERR;
    }
    return RETURN_OK;
#else
    return flash_policy_erase(spic,addr,size);
#endif
}

/**
 * @brief 写FLASH,兼容32M以上Flash
 *
 * @param spic spiflash控制器
 * @param addr FLASH地址
 * @param buf mem地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t flash_write(spic_base_t spic,uint32_t addr,uint32_t buf,uint32_t size)
{
#if FLASH_MSF
    uint32_t extend_addr,mem_addr,flash_addr,write_size,residue_size,block_count;
    residue_size = size;
    mem_addr = buf;
    extend_addr = addr >> 24;
    flash_addr = 0xFFFFFF & addr;
    write_size = FLASH_SISZE_16M - flash_addr;
    if(write_size < residue_size)
    {
        /* 有跨界情况 */
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_write(spic,flash_addr,mem_addr,write_size))
        {
            return RETURN_ERR;
        }
        flash_addr += write_size;
        mem_addr += write_size;
        residue_size -= write_size;
        extend_addr += 1;
    }
    else
    {
        /* 无跨界情况 */
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_write(spic,flash_addr,mem_addr,residue_size))
        {
            return RETURN_ERR;
        }
        return RETURN_OK;
    }
    /* 计算剩余多少块 */
    block_count = residue_size / FLASH_SISZE_16M;
    for(int i = 0;i < block_count;i++)
    {
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_write(spic,flash_addr,mem_addr,FLASH_SISZE_16M))
        {
            return RETURN_ERR;
        }
        flash_addr += FLASH_SISZE_16M;
        mem_addr += FLASH_SISZE_16M;
        residue_size -= FLASH_SISZE_16M;
        extend_addr += 1;
    }
    /* 是否有剩余不满一块的数据 */
    if(residue_size)
    {
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_write(spic,flash_addr,mem_addr,residue_size))
        {
            return RETURN_ERR;
        }
    }
    return RETURN_OK;
#else
    return flash_quad_write(spic,addr,buf,size);
#endif
}

/**
 * @brief 读FLASH,兼容32M以上Flash
 *
 * @param spic spiflash控制器
 * @param buf mem地址
 * @param addr FLASH地址
 * @param size 大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t flash_read(spic_base_t spic,uint32_t buf,uint32_t addr,uint32_t size)
{
#if FLASH_MSF
    uint32_t extend_addr,mem_addr,flash_addr,write_size,residue_size,block_count;
    residue_size = size;
    mem_addr = buf;
    extend_addr = addr >> 24;
    flash_addr = 0xFFFFFF & addr;
    write_size = FLASH_SISZE_16M - flash_addr;
    if(write_size < residue_size)
    {
        /* 有跨界情况 */
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_read(spic,mem_addr,flash_addr,write_size))
        {
            return RETURN_ERR;
        }
        flash_addr += write_size;
        mem_addr += write_size;
        residue_size -= write_size;
        extend_addr += 1;
    }
    else
    {
        /* 无跨界情况 */
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_read(spic,mem_addr,flash_addr,residue_size))
        {
            return RETURN_ERR;
        }
        return RETURN_OK;
    }
    /* 计算剩余多少块 */
    block_count = residue_size / FLASH_SISZE_16M;
    for(int i = 0;i < block_count;i++)
    {
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_read(spic,mem_addr,flash_addr,FLASH_SISZE_16M))
        {
            return RETURN_ERR;
        }
        flash_addr += FLASH_SISZE_16M;
        mem_addr += FLASH_SISZE_16M;
        residue_size -= FLASH_SISZE_16M;
        extend_addr += 1;
    }
    /* 是否有剩余不满一块的数据 */
    if(residue_size)
    {
        if(RETURN_ERR == spic_write_extend_addr_register(spic,extend_addr))
        {
            return RETURN_ERR;
        }
        if(RETURN_ERR == flash_quad_read(spic,mem_addr,flash_addr,residue_size))
        {
            return RETURN_ERR;
        }
    }
    return RETURN_OK;
#else
    return flash_quad_read(spic,buf,addr,size);
#endif
}

/**
 * @brief FLASH配置DNN模式
 *
 * @param spic spiflash控制器
 * @param start_addr DNN模型起始地址
 * @param size DNN模型大小
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t dnn_mode_config(spic_base_t spic,uint32_t start_addr,uint32_t size)
{
    if((0 != (start_addr % 4)) || (0 != (size % 4)))
    {
        //return RETURN_ERR;
    }
    spic_register_t* spic_p = (spic_register_t*)spic;
    uint32_t tmp = 0;
    int32_t timeout = 0;
    timeout = OTHER_TIMR * CPU_MS;
    if(RETURN_ERR == spic_wait_command_vaild(spic,timeout))
    {
    	ci_logerr(LOG_QSPI_DRIVER,"spic cmd error\n");
        return RETURN_ERR;
    }
    while(spic_p->spic_csr_29 && timeout--);
    if(0 >= timeout)
    {
        ci_logerr(LOG_QSPI_DRIVER,"mode config error\n");
        return RETURN_ERR;
    }
    /* 读操作取消CS时间 */
    tmp = spic_p->spic_csr_15;
    tmp &= ~(0x1f << 5);
    tmp |= (0x4 << 5);
    spic_p->spic_csr_15 = tmp;
    /* DNN 模式下spi的dummy等待周期 */
    tmp = spic_p->spic_csr_18;
    tmp &= ~(0x1f << 17);
    tmp |= (0x6 << 17);
    spic_p->spic_csr_18 = tmp;
    /* 预取值的起始地址（word对齐，低2位为0） */
    spic_p->spic_csr_20 = start_addr;
    /* 预取值的终止地址（word对齐，低2位为0） */
    spic_p->spic_csr_21 = start_addr + size;
    /* 预取第一笔数据的地址（word对齐，低2位为0） */
    spic_p->spic_csr_22 = start_addr;
    tmp = spic_p->spic_csr_17;
    /* AHB1模式下地址跳转使能 */
    tmp |= (0x1 << 1);
    /* AHB1模式下地址连续判断使能 */
    tmp |= (0x1 << 4);
    /* AHB1模式下四线模式使能 */
    tmp |= (0x1 << 23);
    /* AHB1模式下spi端addr和M值发送方式 多线模式 */
    tmp |= (0x1 << 3);
    #if 0
    /* 内部fifo水线控制器，为提高效率尽量选大 */
    tmp |= (0x7 << 16);
    /* AHB1读操作预取使能 */
    tmp |= (0x1 << 19);
    /* Spi 端跨边界跳转使能（预取模式使用） */
    tmp &= ~(0x1 << 14);
    #endif
    /* 内部fifo水线控制器，为提高效率尽量选大 */
    tmp &= ~(0x7 << 16);
    tmp |= (0x0 << 16);
    /* AHB1读操作预取使能 */
	tmp &= ~(0x1 << 19);
    tmp |= (0x1 << 19);
    /* Spi 端跨边界跳转使能（预取模式使用） */
    tmp |= (0x1 << 14);
    spic_p->spic_csr_17 = tmp;
    return RETURN_OK;
}

/**
 * @brief FLASH DNN模式
 *
 * @param spic spiflash控制器
 * @param cmd ENABLE/DISABLE
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t flash_dnn_mode(spic_base_t spic,FunctionalState cmd)
{
    spic_register_t* spic_p = (spic_register_t*)spic;
    uint32_t tmp = 0;
    int32_t timeout = 0;
    static uint8_t mode_flag = 0;
    /* 模式切换 */
    if(ENABLE == cmd)
    {
        /* 判断是否IDLE */
        timeout = OTHER_TIMR * CPU_MS;
        if(RETURN_ERR == spic_wait_command_vaild(spic,timeout))
        {
        	ci_logerr(LOG_QSPI_DRIVER,"qspi busy\n");
            return RETURN_ERR;
        }
		
        while(spic_p->spic_csr_29 && timeout--);
        if(0 >= timeout)
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi busy\n");
            return RETURN_ERR;
        }
        /* 如果已经是AHB1就返回 */
        if((spic_p->spic_csr_17 & 0x1) && (mode_flag))
        {
            ci_logwarn(LOG_QSPI_DRIVER,"flash already is DNN mode\n");
            return RETURN_OK;
        }
        mode_flag = 1;
        /* AHB0 切换到 AHB1 */
        tmp = spic_p->spic_csr_07;
        if(1 == (0x1 & tmp))
        {
            spic_p->spic_csr_07 |= (0x1 << 1);
        }
        tmp = spic_p->spic_csr_17;
        /* AHB1读模式的命令值 */
        tmp &= ~(0xff << 6);
        tmp |= (0xeb << 6);
        /* AHB1模式必须写1 */
        tmp |= (0x1 << 24);
        /* AHB1模式使能 */
        tmp |= (0x1 << 0);
        spic_p->spic_csr_17 = tmp;
    }
    else
    {
        /* 如果已经是AHB0就返回 */
        if((!(spic_p->spic_csr_17 & 0x1)) && (!mode_flag))
        {
            ci_logwarn(LOG_QSPI_DRIVER,"flash already is normal mode\n");
            return RETURN_OK;
        }
        mode_flag = 0;
        /* AHB1 切换到 AHB0 */
        spic_p->spic_csr_17 &= ~(0x1 << 0);
        timeout = OTHER_TIMR * SPIC_MS;
        /* 查询中断，清除中断 */
        while(!(spic_p->spic_csr_26 & (0x1 << 1)) && timeout--);
        if(0 >= timeout)
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi no dnn mdoe close int\n");
            return RETURN_ERR;
        }
        spic_p->spic_csr_27 |= (0x1 << 1);
        /* 清除FIFO */
        spic_p->spic_csr_17 |= (0x1 << 15);
        timeout = OTHER_TIMR * SPIC_MS;
        /* 查询中断，清除中断 */
        while(!(spic_p->spic_csr_26 & (0x1 << 0)) && timeout--);
        if(0 >= timeout)
        {
            ci_logerr(LOG_QSPI_DRIVER,"qspi no dnn fifo clear int\n");
            return RETURN_ERR;
        }
        spic_p->spic_csr_27 |= (0x1 << 0);
    }
    return RETURN_OK;
}

/**
 * @brief 检查FLASH是否为DNN模式
 *
 * @param spic spiflash控制器
 *
 * @retval RETURN_OK
 * @retval RETURN_ERR
 */
int32_t flash_is_dnn_mode(spic_base_t spic)
{
    spic_register_t* spic_p = (spic_register_t*)spic;

    if((spic_p->spic_csr_17 & 0x1))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}


/**
 * @brief 检测FLASH模式
 *
 * @param spic spiflash控制器
 *
 * @return int32_t 1;quad mode 0;stand mode
 */
uint32_t flash_check_mode(spic_base_t spic)
{
    uint8_t reg2;

    spic_read_status_register(spic,SPIC_CMD_CODE_READSTATUSREG2,&reg2);

    if( 0 != (reg2&(1<<1)) )
    {
        return 1;/*quad mode*/
    }
    else
    {
        return 0;/*stand mode*/
    }
}
