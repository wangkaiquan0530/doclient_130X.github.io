/**
 * @file ci112x_init.c
 * @brief C环境启动预初始
 * @version 1.0.0
 * @date 2019-11-21
 *
 * @copyright Copyright (c) 2019  Chipintelli Technology Co., Ltd.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ci112x_core_eclic.h"
#include "ci112x_core_misc.h"
#include "ci112x_core_timer.h"
#include "ci112x_scu.h"
#include "ci112x_uart.h"
#include "ci112x_spiflash.h"
#include "ci_flash_data_info.h"
#include "platform_config.h"
#include "flash_rw_process.h"

void _init()
{
	scu_pll_12d288_config(MAIN_FREQUENCY*2);
    /* ECLIC init */
    eclic_init(ECLIC_NUM_INTERRUPTS);
    eclic_mode_enable();

    disable_mcycle_minstret();
}


void _fini()
{
}


void SystemInit(void)
{
    /*SCU中设置外设默认时钟分频*/
    Unlock_Ckconfig();
    Unlock_Rstconfig();
    unlock_sysctrl_config();

    /*设置复位模式的默认值*/
    SCU->SOFT_SYSRST_CFG |= (0x3 << 4);
    SCU->CLKDIV_PARAM0_CFG = (0x2 << 22)|(0x10 << 16)|(0x18 << 4)|(0x1 << 0);/*AHB div 1[bit0~5],APB div 2[bit8~13]*/
    SCU->CLKDIV_PARAM1_CFG = (0x10 << 9)|(0xc << 3)|(0x2 << 0);
    SCU->CLKDIV_PARAM2_CFG = 0x208;
    SCU->CLKDIV_PARAM3_CFG = 0x545454;

    /*设置I2S 时钟默认值*/
    SCU->IIS1_CLK_CFG = 0x04;
    SCU->IIS2_CLK_CFG = 0x04;
    SCU->IIS3_CLK_CFG = 0x04;  //二代较一代增加IIS3

    /*设置SCU 中断屏蔽寄存器默认值*/
    //SCU->SCU_INT_MASK = 0x0;

    /*设置低功耗唤醒状态寄存器默认值*/
    SCU->INT_STATE_REG = 0xFFFFFF;/*set 1 to clear status*/

    /*设置中断屏蔽寄存器默认值*/
    SCU->WAKEUP_MASK_CFG = 0x0;

    /*设置滤波参数默认值*/
    SCU->EXT0_FILTER_CFG = 0xFFFF;
    SCU->EXT1_FILTER_CFG = 0xFFFF;

    /*IO 复用默认值, 避免操作到PSRAM的IO*/
    SCU->IO_REUSE0_CFG = 0X0;
    SCU->IO_REUSE1_CFG = 0X0;

    /*复位外设*/
    /*because maybe run code in qspi, so no reset qspi,cache*/
    Unlock_Rstconfig();

    SCU->SOFT_PRERST0_CFG = (0x1<<6);   //no qspi0[6]
    SCU->SOFT_PRERST1_CFG = 0x0;
    for(volatile unsigned int i=0x3FFF;i;i--);
    SCU->SOFT_PRERST0_CFG = 0xFFFFFFFF;
    SCU->SOFT_PRERST1_CFG = 0xFFFFFFFF;

    /*设置模拟IO成数字功能*/
    SCU->AD_IO_REUSE0_CFG = 0;

    /*关闭外设时钟,*/
    SCU->SYS_CLKGATE_CFG = 0x3F0;
    SCU->PRE_CLKGATE0_CFG = 0x281;
    SCU->PRE_CLKGATE1_CFG = 0x0;

    Lock_Ckconfig();
    Lock_Rstconfig();
    lock_sysctrl_config();

    set_timer_stop();
    clear_timer_clksrc();
}


void __low_level_init_in_flash_code(uint32_t _data_lma,
										uint32_t _data,
										uint32_t _edata,
										uint32_t text_in_flash_start,
										uint32_t in_flash_size)
{
	if(in_flash_size == 0)
	{
		return;
	}

#if TEXT_INIT_IN_FLASH  //请需要调试的时候再打开
	uint32_t dst_addr = text_in_flash_start-0x1fbb0000;
	uint32_t erase_addr = (dst_addr/4096)*4096;
	uint32_t size = in_flash_size;
	uint32_t erase_end_addr = ((dst_addr+size+4095)/4096)*4096;
	uint32_t erase_size = erase_end_addr-erase_addr;
	uint32_t src_addr = _data_lma + _edata - _data;

	volatile uint32_t *maskrom_tab = (volatile uint32_t *)MASK_ROM_WINDOW_ADDR;
	volatile uint32_t *check_sram = (volatile uint32_t *)(MASK_ROM_WINDOW_ADDR+0x1ff00000);

	//调试模式下要把程序搬运到正确的flash空间，非调试不需要
	if(((*maskrom_tab) != 0x3B490fC7) && ((*maskrom_tab) == (*check_sram)))
	{
		SystemInit();
		eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO0);
		eclic_global_interrupt_enable();
		enable_mcycle_minstret();
		init_platform();

		//UARTPollingConfig(UART0);
		//mprintf("src_addr 0x%x dst_addr 0x%x virtual_addr 0x%x size 0x%x\n",src_addr,dst_addr,text_in_flash_start,size);

		eclic_irq_set_priority(GDMA_IRQn, 7, 0);
		flash_init(QSPI0);
		uint32_t asr_bin_addr = 0;
		post_read_flash((char *)(&asr_bin_addr),FILECONFIG_SPIFLASH_START_ADDR+0xCC,4);
		if(erase_addr+erase_size > asr_bin_addr)
		{
			// 请打包固件时给待调试的user.bin预留足够的flash空间
	        while(1)
	        	asm volatile ("ebreak");
		}
		post_erase_flash(erase_addr,erase_size);
		post_write_flash((char *)src_addr,dst_addr,size);
		uint8_t zore = 0;
		post_write_flash((char *)(&zore),FILECONFIG_SPIFLASH_START_ADDR+0xB6,1);
		mprintf("src_addr 0x%x dst_addr 0x%x virtual_addr 0x%x size 0x%x\n",src_addr,dst_addr,text_in_flash_start,size);
	}
#endif

    scu_run_in_flash();
}
