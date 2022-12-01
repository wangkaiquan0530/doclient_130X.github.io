#ifndef _PLATFORM_CONFIG_H
#define _PLATFORM_CONFIG_H

#include <stdint.h>
#include "ci112x_scu.h"
#include <stdbool.h>


uint32_t get_ipcore_clk(void);
uint32_t get_ahb_clk(void);
uint32_t get_apb_clk(void);
uint32_t get_systick_clk(void);
uint32_t get_iispll_clk(void);
uint32_t get_osc_clk(void);

void set_ipcore_clk(uint32_t clk);
void set_ahb_clk(uint32_t clk);
void set_apb_clk(uint32_t clk);
void set_systick_clk(uint32_t clk);
void set_iispll_clk(uint32_t clk);

void init_platform(void);
void pa_switch_io_init(void);
bool get_pa_control_level_flag(void);
int vad_start_mark(void);
int vad_end_mark(void);
void init_dma_channel0_mutex(void);

/**
 * @addtogroup ci112x_i2c
 * @{
 */
void i2c_io_init(void);
/**
 * @}
 */
void apds9960_io_init(void);

#endif
