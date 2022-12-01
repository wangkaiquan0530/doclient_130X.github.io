/**
 * @file ci112x_scu.c
 * @brief 系统配置
 * @version 1.0
 * @date 2018-04-09
 * 
 * @copyright Copyright (c) 2019 Chipintelli Technology Co., Ltd.
 * 
 */
#include "ci112x_core_eclic.h"
#include "ci112x_core_misc.h"
#include "ci112x_scu.h"
#include "ci_assert.h"
#include "ci112x_spiflash.h"


#define SYSCFG_UNLOCK_MAGIC 0x51ac0ffe
#define CKCFG_UNLOCK_MAGIC  0x51ac0ffe
#define RSTCFG_UNLOCK_MAGIC 0x51ac0ffe
#define SOFTWARE_RESET_KEY  0xdeadbeef

#define DIV_MASK1   0x1
#define DIV_MASK2   0x3
#define DIV_MASK3   0x7
#define DIV_MASK4   0xf
#define DIV_MASK5   0x1f
#define DIV_MASK6   0x3f
#define DIV_MASK7   0x7f
#define DIV_MASK8   0xff
#define DIV_MASK9   0x1ff
#define DIV_MASK12  0xfff


/**
  * @功能:延迟函数
  * @注意:无
  * @参数:cnt 延迟时间 毫秒
  * @返回值:无
  */
static void _delay(unsigned int cnt)
{
    volatile unsigned int i,j = 0;
    for(i = 0; i < cnt; i++)
    {
        for(j = 0; j < 132; j++)
        {
            __NOP();
        }
    }
}


static void _delay_verify(unsigned int cnt)
{
    volatile unsigned int i,j = 0;
    for(i = 0; i < cnt; i++)
    {
        for(j = 0; j < 132/5; j++)
        {
        	__NOP();
        }
    }
}



static uint32_t scu_enter_critical(void)
{
    uint32_t  base_pri = eclic_get_mth();
    eclic_set_mth((0x6<<5)|0x1f); //0XDF
    return base_pri;
}

static void scu_exit_critical(uint32_t base_pri)
{
    eclic_set_mth(base_pri);
}


typedef struct
{
    uint32_t dsmpd:1;
    uint32_t fout1pd:1;
    uint32_t fout2pd:1;
    uint32_t foutvcopd:1;
    uint32_t refdiv:6;
    uint32_t fbdiv:8;
    uint32_t postdiv1:4;
    uint32_t postdiv2:4;
    uint32_t bypass:1;
    uint32_t resetlock:1;
    uint32_t rev:4;
}pll_cfg0_t;


typedef struct
{
    union pll_reg0 
    {
        uint32_t reg0_int;
        pll_cfg0_t reg0_bits;
    }reg0;
    
    uint32_t reg1;
}pll_set_t;


/*must be const!!! used in low_level_init function--for188MHz*/
const pll_set_t pll_12d288_upto_376 =
{
    .reg0=
    {
        .reg0_bits =
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 61,/*61.197916666, so here 58, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 2 -1,/* 720 mul 2, reg need config -1,so vco out 720 in 320~1280Mhz*/
            .postdiv2 = 2 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 0x32AAAA,/*61.197916666, so here 0.197916666, Q24*/
};

/*must be const!!! used in low_level_init function--for186MHz*/
const pll_set_t pll_12d288_upto_372 =
{
    .reg0=
    {
        .reg0_bits =
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 60,/*60.546875, so here 58, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 2 -1,/* 720 mul 2, reg need config -1,so vco out 720 in 320~1280Mhz*/
            .postdiv2 = 2 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 9175040,/*60.546875, so here 0.546875, Q24*/
};


/*must be const!!! used in low_level_init function--for180MHz*/
const pll_set_t pll_12d288_upto_360 =
{
    .reg0=
    {
        .reg0_bits =
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 58,/*58.59375, so here 58, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 2 -1,/* 720 mul 2, reg need config -1,so vco out 720 in 320~1280Mhz*/
            .postdiv2 = 2 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 9961472,/*58.59375, so here 0.6875, Q24*/
};


/*must be const!!! used in low_level_init function--for168MHz*/
const pll_set_t pll_12d288_upto_336 =
{
    .reg0=
    {
        .reg0_bits = 
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 54,/*54.6875, so here 54, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 2 -1,/* 336 mul 2, reg need config -1,so vco out 672 in 320~1280Mhz*/
            .postdiv2 = 2 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 11534336,/*54.6875, so here 0.6875, Q24*/
};


/*must be const!!! used in low_level_init function--for164MHz*/
const pll_set_t pll_12d288_upto_328 =
{
    .reg0=
    {
        .reg0_bits = 
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 80,/*80.078125, so here 53, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 3 -1,/* 336 mul 2, reg need config -1,so vco out 672 in 320~1280Mhz*/
            .postdiv2 = 3 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 1310720,/*80.078125, so here 078125, Q24*/
};


/*must be const!!! used in low_level_init function--for180MHz*/
const pll_set_t pll_12d288_upto_180 =
{
    .reg0=
    {
        .reg0_bits =
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 58,/*58.59375, so here 58, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 4 -1,/* 720 mul 2, reg need config -1,so vco out 720 in 320~1280Mhz*/
            .postdiv2 = 4 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 9961472,/*58.59375, so here 0.6875, Q24*/
};
/*must be const!!! used in low_level_init function--for164MHz*/
const pll_set_t pll_12d288_upto_164 =
{
    .reg0=
    {
        .reg0_bits = 
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 53,/*53.38541667, so here 53, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 4 -1,/* 336 mul 2, reg need config -1,so vco out 672 in 320~1280Mhz*/
            .postdiv2 = 4 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 6466218,/*53.38541667, so here 0.38541667, Q24*/
};

/*must be const!!! used in low_level_init function--for164MHz*/
const pll_set_t pll_12d288_upto_82 =
{
    .reg0=
    {
        .reg0_bits = 
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 53,/*53.38541667, so here 53, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 8 -1,/* 336 mul 2, reg need config -1,so vco out 672 in 320~1280Mhz*/
            .postdiv2 = 8 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 6466218,/*53.38541667, so here 0.38541667, Q24*/
};


/*must be const!!! used in low_level_init function--for164MHz*/
const pll_set_t pll_12d288_upto_41 =
{
    .reg0=
    {
        .reg0_bits = 
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 0,/*no powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 53,/*53.38541667, so here 53, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 16 -1,/* 336 mul 2, reg need config -1,so vco out 672 in 320~1280Mhz*/
            .postdiv2 = 16 -1,
            .bypass = 0,/*no bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 6466218,/*53.38541667, so here 0.38541667, Q24*/
};


#define PLL_REF_MAX_FRE (640000000.0f)
#define PLL_REF_MIN_FRE (5000000.0f)

#define PLL_REFDIV_MAX_FRE (64000000.0f)
#define PLL_REFDIV_MIN_FRE (5000000.0f)

#define PLL_FBDIV_FRAC_MAX_FRE (1280000000.0f)
#define PLL_FBDIV_FRAC_MIN_FRE (320000000.0f)

#define PLL_OUT_FRE_MAX_FRE (1280000000.0f)
#define PLL_OUT_FRE_MIN_FRE (20000000.0f)

#define PLL_FRAC (16777216.0f)//2^24 = 16777216


static uint8_t pll_get_postdiv(float dst,uint8_t postdiv,float pll_after_refdiv)
{
    float fbdiv_frac = dst*(float)postdiv / pll_after_refdiv;
    uint8_t fbdiv = (uint8_t)fbdiv_frac;
    if( (fbdiv >= 8) || (postdiv >= 16) )
    {
        return postdiv;
    }
    else
    {
        postdiv++;
        pll_get_postdiv(dst,postdiv,pll_after_refdiv);
    }
    return 0;
}


void setpll(float ref,float dst,pll_set_t* pll_str_p)
{
    if( (ref > PLL_REF_MAX_FRE) || (ref < PLL_REF_MIN_FRE) )
    {
        CI_ASSERT(0,"ref fre too high or too low\n");
    }
    if( (dst > PLL_OUT_FRE_MAX_FRE) || (dst < PLL_OUT_FRE_MIN_FRE) )
    {
        CI_ASSERT(0,"dst fre too high or too low\n");
    }

    uint8_t refdiv = 1;
    if(ref > PLL_REFDIV_MAX_FRE)
    {
        float refdiv_f = (ref/PLL_REFDIV_MAX_FRE + 1.0f);
        refdiv = (uint8_t)refdiv_f;
    }
    float pll_after_refdiv = ref/(float)refdiv;//step1

    uint8_t fbdiv = 0;
    uint32_t frac = 0;
    uint8_t postdiv = 1;
    
    if(dst > PLL_FBDIV_FRAC_MIN_FRE)
    {
        postdiv = 1;
    }
    else
    {
//        postdiv = (uint8_t)(PLL_FBDIV_FRAC_MIN_FRE/dst + 1.0f);
        postdiv = (uint8_t)(PLL_FBDIV_FRAC_MIN_FRE/dst);
        if(postdiv >= 16)
        {
            postdiv = 16;
        }
    }

//    postdiv = pll_get_postdiv(dst,postdiv,pll_after_refdiv);
    float fbdiv_frac = 0.0f;
    
    float step2_f = 0.0f;
    
    uint16_t timeout_flag = 0;
    
    while(1)
    {
        fbdiv_frac = dst*(float)postdiv / pll_after_refdiv;
        step2_f = pll_after_refdiv*fbdiv_frac;
        fbdiv = (uint8_t)fbdiv_frac;
        if( (fbdiv >= 8) 
           && (fbdiv <= 251) 
           && (step2_f >= PLL_FBDIV_FRAC_MIN_FRE) 
           && (step2_f <= PLL_FBDIV_FRAC_MAX_FRE)  
           )
        {
            break;
        }
        else
        {
            timeout_flag++;
            if(timeout_flag > 2000)
            {
                CI_ASSERT(0,"timeout_flag err\n");
            }
            if( (step2_f < PLL_FBDIV_FRAC_MIN_FRE) || (fbdiv < 8) )
            {
                postdiv++;
                if(postdiv > 16)
                {
                    CI_ASSERT(0,"postdiv err\n");
                }
            }
            else
            {
                postdiv--;
                if(postdiv < 1)
                {
                    CI_ASSERT(0,"postdiv err\n");
                }
            }
            
        }
    }
    timeout_flag = 0;
    
    float frac_f = fbdiv_frac - (float)fbdiv;
    if( (frac_f < 0.0f) || (frac_f >= 1.0f) )
    {
        CI_ASSERT(0,"frac_f err\n");
    }
    frac = (uint32_t)(frac_f*PLL_FRAC);

    pll_str_p->reg0.reg0_bits.fbdiv = fbdiv;
    pll_str_p->reg0.reg0_bits.postdiv1 = postdiv - 1;
    pll_str_p->reg0.reg0_bits.refdiv = refdiv;
    pll_str_p->reg1 = frac;
}


float get_pll_fre(float ref,pll_set_t* pll_str_p)
{
    uint8_t fbdiv = pll_str_p->reg0.reg0_bits.fbdiv;
    uint32_t frac = pll_str_p->reg1;
    uint8_t postdiv = pll_str_p->reg0.reg0_bits.postdiv1 + 1;
    uint8_t refdiv = pll_str_p->reg0.reg0_bits.refdiv;

    if( fbdiv < 8 || (fbdiv > 251) )
    {
        CI_ASSERT(0,"fbdiv err\n");
    }
    if(frac > (uint32_t)PLL_FRAC)
    {
        CI_ASSERT(0,"frac err\n");
    }
    if( (postdiv < 1) || (postdiv > 16) )
    {
        CI_ASSERT(0,"postdiv err\n");
    }
    if( (refdiv < 1) || (refdiv > 63) )
    {
        CI_ASSERT(0,"refdiv err\n");
    }

    if((ref < PLL_REF_MIN_FRE) || (ref > PLL_REF_MAX_FRE))
    {
        CI_ASSERT(0,"ref err\n");
    }

    float step1 = ref/(float)refdiv;
    if((step1 < PLL_REFDIV_MIN_FRE) || (step1 > PLL_REFDIV_MAX_FRE))
    {
        CI_ASSERT(0,"step1 err\n");
    }

    float step2 = step1*((float)fbdiv + (float)frac/PLL_FRAC);
    if( (step2 < PLL_FBDIV_FRAC_MIN_FRE) || (step2 > PLL_FBDIV_FRAC_MAX_FRE) )
    {
        CI_ASSERT(0,"step2 err\n");
    }

    float dst = step2/(float)postdiv;
    if( (dst < PLL_OUT_FRE_MIN_FRE) || (dst > PLL_OUT_FRE_MAX_FRE))
    {
        CI_ASSERT(0,"dst err\n");
    }

    return dst;
}

#include <math.h>

pll_set_t pll_str;

void pll_set_test_fun(void)
{


    float osc_ref = 12288000.0f;
    for(uint32_t dst = 36167000;dst<1280000000;dst += 2000)
    {
        float dst_f = (float)dst; 
        setpll(osc_ref,dst_f,&pll_str);
        float dsf_cal = get_pll_fre(osc_ref,&pll_str);
        float dst_bias = (dsf_cal - dst_f)/dst_f;
        //dst_bias = sqrtf(dst_bias);
        if( (dst_bias > 0.0000005f) || (dst_bias < -0.0000005f) )
        {
            mprintf("dst_bias = %f\n",dst_bias);
            mprintf("dsf_cal = %f\n",dsf_cal);
            mprintf("dst_f = %f\n",dst_f);
          
            uint8_t fbdiv = pll_str.reg0.reg0_bits.fbdiv;
            uint32_t frac = pll_str.reg1;
            uint8_t postdiv = pll_str.reg0.reg0_bits.postdiv1;
            uint8_t refdiv = pll_str.reg0.reg0_bits.refdiv;
            asm volatile ("ebreak");
            
            
            mprintf("dst bias too high\n");

            mprintf("fbdiv = %d\n",fbdiv);
            mprintf("frac = %d\n",frac);
            mprintf("postdiv = %d\n",postdiv);
            mprintf("refdiv = %d\n",refdiv);
        }
        else
        {
          mprintf("dst fre:%d ok  bias = %f\n",dst,dst_bias);
        }
    }
}


/*must be const!!! used in low_level_init function--for164MHz*/
const pll_set_t pll_12d288_upto_bp =
{
    .reg0=
    {
        .reg0_bits = 
        {
            .dsmpd = 0,/*no powerdown DE*/
            .fout1pd = 1,/*powerdown postdiv*/
            .fout2pd = 1,
            .foutvcopd = 0,/*no powerdown*/
            .refdiv = 1,/*out 5~64Mhz, so used div 1, out 12.288*/
            .fbdiv = 53,/*53.38541667, so here 53, first cal refdiv and postdiv then cal this*/
            .postdiv1 = 16 -1,/* 336 mul 2, reg need config -1,so vco out 672 in 320~1280Mhz*/
            .postdiv2 = 16 -1,
            .bypass = 1,/*bypass*/
            .resetlock = 0,/*when config reset pll lock signal*/
        },
    },

    .reg1 = 6466218,/*53.38541667, so here 0.38541667, Q24*/
};


int scu_pll_set_high_freq(pll_set_t *pll_set)
{
    uint32_t  base_pri = scu_enter_critical();

    Unlock_Ckconfig();
    Unlock_Rstconfig();

    /*选择CPU时钟为PLL倍频前,使用晶振时钟*/
    SCU->CLK_SEL_CFG |= PLL_CONFIG_BEFORE;

    SCU->SOFT_PLLRST_CFG &= ~(0x1 << 0);/*pll global powerdown*/

    /*a few reference periods*/
    __NOP();

    /*配置PLL*/
    SCU->SYS_PLL0_CFG = pll_set->reg0.reg0_int;
    SCU->SYS_PLL1_CFG = pll_set->reg1;
    /*设置PLL配置参数生效*/
    SCU->PLL_UPDATE_CFG |= (0x1 << 0);

    /*1 microsecond*/
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    SCU->SOFT_PLLRST_CFG |= (0x1 << 0);/*pll global poweron*/

    _delay(1);/*now cpu 8M run so delay need small, now need more than 0.1ms,real is 0.5ms*/

    /*选择CPU时钟为PLL倍频后*/
    SCU->CLK_SEL_CFG &= PLL_CONFIG_AFTER;
    
    Lock_Ckconfig();
    Lock_Rstconfig();

    scu_exit_critical(base_pri);
    return RETURN_OK;
}


int scu_pll_set_high_freq_verify(const pll_set_t *pll_set)
{
    uint32_t  base_pri = scu_enter_critical();

    Unlock_Ckconfig();
    Unlock_Rstconfig();

    /*选择CPU时钟为PLL倍频前,使用晶振时钟*/
    SCU->CLK_SEL_CFG |= PLL_CONFIG_BEFORE;

    SCU->SOFT_PLLRST_CFG &= ~(0x1 << 0);/*pll global powerdown*/

    /*a few reference periods*/
    __NOP();

    /*配置PLL*/
    SCU->SYS_PLL0_CFG = pll_set->reg0.reg0_int;
    SCU->SYS_PLL1_CFG = pll_set->reg1;
    /*设置PLL配置参数生效*/
    SCU->PLL_UPDATE_CFG |= (0x1 << 0);

    /*1 microsecond*/
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    SCU->SOFT_PLLRST_CFG |= (0x1 << 0);/*pll global poweron*/

    _delay_verify(1);/*short delay for verify platform*/

    /*选择CPU时钟为PLL倍频后*/
    SCU->CLK_SEL_CFG &= PLL_CONFIG_AFTER;

    Lock_Ckconfig();
    Lock_Rstconfig();

    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
 * @brief 配置scu_pll
 * @note 注意可选时钟有效性
 * 
 * @param clk pll 时钟
 */
void scu_pll_12d288_config(uint32_t clk)
{
    pll_set_t const *pll_12d288_upto_config = &pll_12d288_upto_328;
    switch(clk)
    {
        case 376000000:
        {
    		pll_12d288_upto_config = &pll_12d288_upto_376;
    		break;
    	}
    	// case 372000000:
    	// {
    	// 	pll_12d288_upto_config = &pll_12d288_upto_372;
    	// 	break;
    	// }
    	case 360000000:
    	{
    		pll_12d288_upto_config = &pll_12d288_upto_360;
    		break;
    	}
        // case 336000000:
        // {
        //     pll_12d288_upto_config = &pll_12d288_upto_336;
        //     break;
        // }
        // case 328000000:
        // {
        //     pll_12d288_upto_config = &pll_12d288_upto_328;
        //     break;
        // }
        case 188000000:     //降频还是按照180的一半配置
        {
            pll_12d288_upto_config = &pll_12d288_upto_180;
            break;
        }
        // case 164000000:
        // {
        //     pll_12d288_upto_config = &pll_12d288_upto_164;
        //     break;
        // }
        // case 82000000:
        // {
        //     pll_12d288_upto_config = &pll_12d288_upto_82;
        //     break;
        // }
        // case 41000000:
        // {
        //     pll_12d288_upto_config = &pll_12d288_upto_41;
        //     break;
        // }
        // case 0: //bypass
        // {
        //     pll_12d288_upto_config = &pll_12d288_upto_bp;
        //     break;
        // }
        default:
        {
            while(1);
        }
    }

    //scu_pll_set_high_freq(pll_12d288_upto_config);
    scu_pll_set_high_freq_verify(pll_12d288_upto_config);
}


/**
  * @功能:解锁系统控制寄存器
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void unlock_sysctrl_config(void)
{
    SCU->SYSCFG_LOCK_CFG = SYSCFG_UNLOCK_MAGIC;
    while(!(SCU->SYSCFG_LOCK_CFG));
}


/**
  * @功能:解锁时钟相关寄存器
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Unlock_Ckconfig(void)
{
    SCU->CKCFG_LOCK = CKCFG_UNLOCK_MAGIC;
    while(!(SCU->CKCFG_LOCK));
}


/**
  * @功能:解锁复位相关寄存器
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Unlock_Rstconfig(void)
{
    SCU->RSTCFG_LOCK = RSTCFG_UNLOCK_MAGIC;
    while(!(SCU->RSTCFG_LOCK));
}


/**
  * @功能:锁定配置寄存器
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void lock_sysctrl_config(void)
{
    SCU->SYSCFG_LOCK_CFG = 0x0;
}


/**
  * @功能:锁定配置寄存器
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Lock_Ckconfig(void)
{
    SCU->CKCFG_LOCK = 0x0;
}


/**
  * @功能:锁定复位寄存器
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Lock_Rstconfig(void)
{
    SCU->RSTCFG_LOCK = 0x0;
}


/**
  * @功能:配置外设分频参数之前需将PARAM_EN0_CFG寄存器相应位置0
  * @注意:无
  * @参数:device_base：外设地址
  * @返回值:int8_t：错误类型
  */
int8_t Scu_Para_En_Disable(uint32_t device_base)
{
    uint32_t para_en0;
    
    uint32_t  base_pri = scu_enter_critical();
    
    para_en0 = SCU->PARAM_EN0_CFG;
    
    if(AHB_BASE == device_base)
    {
        para_en0 &= ~(1 << 0);
    }
    else if(M4_SYSTICK_BASE == device_base)
    {
        para_en0 &= ~(1 << 1);
    }
    else if(TARCE_CLOCK_DIV == device_base)
    {
        para_en0 &= ~(1 << 2);
    }
    else if(HAL_IWDG_BASE == device_base)
    {
        para_en0 &= ~(1 << 3);
    }
    else if(HAL_DNN_BASE == device_base)
    {
        para_en0 &= ~(1 << 4);
    }
    else if(HAL_FFT_BASE == device_base)
    {
        para_en0 &= ~(1 << 5);
    }
    else if(HAL_VAD_BASE == device_base)
    {
        para_en0 &= ~(1 << 6);
    }
    else if(HAL_QSPI0_BASE == device_base)
    {
        para_en0 &= ~(1 << 7);
    }
    else if(HAL_ADC_BASE == device_base)
    {
        para_en0 &= ~(1 << 8);
    }
    else if(HAL_TIMER0_BASE == device_base)
    {
        para_en0 &= ~(1 << 9);
    }
    else if(HAL_UART0_BASE == device_base)
    {
        para_en0 &= ~(1 << 10);
    }
    else if(HAL_UART1_BASE == device_base)
    {
        para_en0 &= ~(1 << 11);
    }
    else if(HAL_IIS1_BASE == device_base)
    {
        para_en0 &= ~(1 << 12);
    }
    else if(HAL_IIS2_BASE == device_base)
    {
        para_en0 &= ~(1 << 13);
    }
    else if(HAL_IIS3_BASE == device_base)
    {
        para_en0 &= ~(1 << 14);
    }
    else
    {
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    
    if(SCU->PARAM_EN0_CFG != para_en0)
    {
        SCU->PARAM_EN0_CFG = para_en0;
    }
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:配置完外设分频参数之前需将PARAM_EN0_CFG寄存器相应位置1
  * @注意:无
  * @参数:device_base：外设地址
  * @返回值:int8_t：错误类型
  */
int8_t Scu_Para_En_Enable(uint32_t device_base)
{
    uint32_t para_en0;
    
    uint32_t  base_pri = scu_enter_critical();
    
    para_en0 = SCU->PARAM_EN0_CFG;
    
    if(AHB_BASE == device_base)
    {
        para_en0 |= (1 << 0);
    }
    else if(M4_SYSTICK_BASE == device_base)
    {
        para_en0 |= (1 << 1);
    }
    else if(TARCE_CLOCK_DIV == device_base)
    {
        para_en0 |= (1 << 2);
    }
    else if(HAL_IWDG_BASE == device_base)
    {
        para_en0 |= (1 << 3);
    }
    else if(HAL_DNN_BASE == device_base)
    {
        para_en0 |= (1 << 4);
    }
    else if(HAL_FFT_BASE == device_base)
    {
        para_en0 |= (1 << 5);
    }
    else if(HAL_VAD_BASE == device_base)
    {
        para_en0 |= (1 << 6);
    }
    else if(HAL_QSPI0_BASE == device_base)
    {
        para_en0 |= (1 << 7);
    }
    else if(HAL_ADC_BASE == device_base)
    {
        para_en0 |= (1 << 8);
    }
    else if(HAL_TIMER0_BASE == device_base)
    {
        para_en0 |= (1 << 9);
    }
    else if(HAL_UART0_BASE == device_base)
    {
        para_en0 |= (1 << 10);
    }
    else if(HAL_UART1_BASE == device_base)
    {
        para_en0 |= (1 << 11);
    }
    else if(HAL_IIS1_BASE == device_base)
    {
        para_en0 |= (1 << 12);
    }
    else if(HAL_IIS2_BASE == device_base)
    {
        para_en0 |= (1 << 13);
    }
    else if(HAL_IIS3_BASE == device_base)
    {
        para_en0 |= (1 << 14);
    }
    else
    {
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    
    if(SCU->PARAM_EN0_CFG != para_en0)
    {
        SCU->PARAM_EN0_CFG = para_en0;
    }
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:设置外设时钟分频
  * 更改分频系数的时候：
  * 关闭外设gate；
  * 需要先将divider复位；
  * 分频参数配置好；
  * 然后使能配置的分频参数有效；
  * 解除divider的复位。
  * @参数:device_base，需要设置的外设基址 div_num，分频输
  * @返回值: PARA_ERROR: 参数错误 RETURN_OK, 配置完成
  */
int Scu_Setdiv_Parameter(unsigned int device_base,unsigned int div_num)
{
    uint32_t new_div0,new_div1,new_div2,new_div3;
    
    uint32_t  base_pri = scu_enter_critical();
    
    new_div0 = SCU->CLKDIV_PARAM0_CFG;
    new_div1 = SCU->CLKDIV_PARAM1_CFG;
    new_div2 = SCU->CLKDIV_PARAM2_CFG;
    new_div3 = SCU->CLKDIV_PARAM3_CFG;

    if(device_base == AHB_BASE)
    {
        new_div0 &= ~(DIV_MASK4 << 0);
        new_div0 |= ((div_num  & DIV_MASK4 ) << 0);
    }
    else if(device_base == M4_SYSTICK_BASE)
    {
        new_div0 &= ~(DIV_MASK12 << 4);
        new_div0 |= ((div_num & DIV_MASK12) << 4);
    }
    else if(device_base == TARCE_CLOCK_DIV)
    {
        new_div0 &= ~(DIV_MASK6 << 10);
        new_div0 |= ((div_num & DIV_MASK6) << 10);
    }
    else if(device_base == HAL_IWDG_BASE)
    {
        new_div0 &= ~(DIV_MASK6 << 16);
        new_div0 |= ((div_num & DIV_MASK6) << 16);
    }
    else if(device_base == HAL_DNN_BASE)
    {
        new_div0 &= ~(DIV_MASK3 << 22);
        new_div0 |= ((div_num & DIV_MASK3) << 22);
    }
    else if(device_base == HAL_FFT_BASE)
    {
        new_div0 &= ~(DIV_MASK3 << 25);
        new_div0 |= ((div_num & DIV_MASK3) << 25);
    }
    else if(device_base == HAL_QSPI0_BASE)
    {
        new_div1 &= ~(DIV_MASK3 << 0);
        new_div1 |= ((div_num & DIV_MASK3) << 0);
    }
    else if(device_base == HAL_ADC_BASE)
    {
        new_div1 &= ~(DIV_MASK6 << 3);
        new_div1 |= ((div_num & DIV_MASK6) << 3);
    }
    else if(device_base == HAL_TIMER0_BASE)
    {
        new_div1 &= ~(DIV_MASK6 << 9);
        new_div1 |= ((div_num & DIV_MASK6) << 9);
    }
    else if(device_base == HAL_VAD_BASE)
    {
        new_div1 &= ~(DIV_MASK8 << 15);
        new_div1 |= ((div_num & DIV_MASK8) << 15);
    }
    else if(device_base == HAL_UART0_BASE)
    {
        new_div2 &= ~(DIV_MASK6 << 0);
        new_div2 |= ((div_num & DIV_MASK6) << 0);
    }
    else if(device_base == HAL_UART1_BASE)
    {
        new_div2 &= ~(DIV_MASK6 << 6);
        new_div2 |= ((div_num & DIV_MASK6) << 6);
    }
    else if(device_base == HAL_IIS1_BASE)
    {
        new_div3 &= ~(DIV_MASK8 << 0);
        new_div3 |= ((div_num & DIV_MASK8) << 0);
    }
    else if(device_base == HAL_IIS2_BASE)
    {
        new_div3 &= ~(DIV_MASK8 << 8);
        new_div3 |= ((div_num & DIV_MASK8) << 8);
    }
    else if(device_base == HAL_IIS3_BASE)
    {
        new_div3 &= ~(DIV_MASK8 << 16);
        new_div3 |= ((div_num & DIV_MASK8) << 16);
    }
    else 
    {
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    
    Unlock_Ckconfig();
    Unlock_Rstconfig();
    
    //先置0
    Scu_Para_En_Disable(device_base);
    
    _delay(1);
    
    if(SCU->CLKDIV_PARAM0_CFG != new_div0)
    {
        SCU->CLKDIV_PARAM0_CFG = new_div0;
    }
    
    if(SCU->CLKDIV_PARAM1_CFG != new_div1)
    {
        SCU->CLKDIV_PARAM1_CFG = new_div1;
    }
    
    if(SCU->CLKDIV_PARAM2_CFG != new_div2)
    {
        SCU->CLKDIV_PARAM2_CFG = new_div2;
    }
    
    if(SCU->CLKDIV_PARAM3_CFG != new_div3)
    {
        SCU->CLKDIV_PARAM3_CFG = new_div3;
    }
    
    //先置0
    Scu_Para_En_Enable(device_base);
    /*分频使能之后，delay*/
    _delay(1);

    Lock_Ckconfig();
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:设置系统时钟开关
  * @注意:无
  * @参数:device_base，需要设置的外设基址 gate:  0 关闭    1 打开
  * @返回值: PARA_ERROR: 参数错误 RETURN_OK, 配置完成
  */
int Scu_SetClkgate(unsigned int device_base,int gate)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    //if(device_base == SRAM2_GATE)
    {
        //SCU->SYS_CLKGATE_CFG &= ~(0x1 << 1);
        //SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 1);
    }
    if(device_base == SRAM0_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 5);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 5);
    }
    else if(device_base == SRAM1_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 6);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 6);
    }
    else if(device_base == SRAM2_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 7);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 7);
    }
    else if(device_base == SRAM3_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 8);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 8);
    }
    else if(device_base == STCLK_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 3);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 3);
    }
    else if(device_base == CM4CORE_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 4);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 4);
    }
    else if(device_base == CM4_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 0);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 0);
    }
    else if(device_base == SLEEPING_MODE_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 1);
        SCU->SYS_CLKGATE_CFG |= ((0x1 & gate) << 1);
    }
    else if(device_base == SLEEPDEEP_MODE_GATE)
    {
        SCU->SYS_CLKGATE_CFG &= ~(0x1 << 2);
        SCU->SYS_CLKGATE_CFG |= ((0x1 << gate) << 2);
    }
    else
    {
        Lock_Ckconfig();
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }

    Lock_Ckconfig();
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:设置时钟开关
  * @注意:无
  * @参数:device_base，需要设置的外设基址 gate:  0 关闭    1 打开
  * @返回值: PARA_ERROR: 参数错误 RETURN_OK, 配置完成
  */
int Scu_SetDeviceGate(unsigned int device_base, int gate)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    if(device_base == TARCE_CLOCK_DIV)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 0);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 0);
    }
    else if(device_base == HAL_IWDG_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 1);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 1);
    }
    else if(device_base == HAL_TWDG_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 2);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 2);
    }
    else if(device_base == HAL_IWDG_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 3);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 3);
    }
    else if(device_base == HAL_TWDG_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 4);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 4);
    }
    else if(device_base == HAL_DNN_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 5);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 5);
    }
    else if(device_base == HAL_FFT_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 6);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 6);
    }
    else if(device_base == HAL_VAD_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 7);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 7);
    }
    else if(device_base == HAL_QSPI0_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 8);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 8);
    }
    else if(device_base == HAL_ADC_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 9);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 9);
    }
    else if(device_base == HAL_ALC_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 10);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 10);
    }
    else if(device_base == HAL_CODEC_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 11);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 11);
    }
    else if(device_base == HAL_IISDMA0_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 12);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 12);
    }
    else if(device_base == HAL_GDMA_BASE)
    {
        SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 13);
        SCU->PRE_CLKGATE0_CFG |= ((0x1 & gate) << 13);
    }
    else if(device_base == HAL_MCLK1_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 0);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 0);
    }
    else if(device_base == HAL_MCLK2_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 1);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 1);
    }
    else if(device_base == HAL_MCLK3_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 2);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 2);
    }
    else if(device_base == HAL_IIS1_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 3);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 3);
    }
    else if(device_base == HAL_IIS2_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 4);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 4);
    }
    else if(device_base == HAL_IIS3_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 5);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 5);
    }
    else if(device_base == HAL_TIMER0_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 6);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 6);
    }
    else if(device_base == HAL_TIMER1_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 7);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 7);
    }
    else if(device_base == HAL_TIMER2_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 8);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 8);
    }
    else if(device_base == HAL_TIMER3_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 9);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 9);
    }
    else if(device_base == HAL_GPIO0_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 10);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 10);
    }
    else if(device_base == HAL_GPIO1_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 11);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 11);
    }
    else if(device_base == HAL_GPIO2_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 12);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 12);
    }
    else if(device_base == HAL_GPIO3_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 13);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 13);
    }
    else if(device_base == HAL_GPIO4_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 14);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 14);
    }
    else if(device_base == HAL_UART0_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 15);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 15);
    }
    else if(device_base == HAL_UART1_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 16);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 16);
    }
    else if(device_base == HAL_PWM0_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 17);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 17);
    }
    else if(device_base == HAL_PWM1_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 18);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 18);
    }
    else if(device_base == HAL_PWM2_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 19);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 19);
    }
    else if(device_base == HAL_PWM3_BASE)
	{
		SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 20);
		SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 20);
	}
    else if(device_base == HAL_PWM4_BASE)
	{
		SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 21);
		SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 21);
	}
    else if(device_base == HAL_PWM5_BASE)
	{
		SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 22);
		SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 22);
	}
	else if(device_base == HAL_IIC0_BASE)
    {
        SCU->PRE_CLKGATE1_CFG &= ~(0x1 << 23);
        SCU->PRE_CLKGATE1_CFG |= ((0x1 & gate) << 23);
    }
    else
    {
        Lock_Ckconfig();
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    Lock_Ckconfig();
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:设置watchdog计数和复位受到CPU HALTED信号控制
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_SetWDG_Halt(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    SCU->PRE_CLKGATE0_CFG |= (0x1 << 4);
    Lock_Ckconfig();
    scu_exit_critical(base_pri);
}


/**
  * @功能:设置watchdog计数和复位是不受CPU HALTED信号控制
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_CleanWDG_Halt(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    SCU->PRE_CLKGATE0_CFG &= ~(0x1 << 4);
    Lock_Ckconfig();
    scu_exit_critical(base_pri);
}


/**
  * @功能:软件复位系统
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_SoftwareRst_System(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    SCU->SOFT_SYSRST_CFG &= ~(0x3 << 8);
    SCU->SOFT_SYSRST_CFG |= (0x2 << 8);
    SCU->SOFT_RST_PARAM = SOFTWARE_RESET_KEY;
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
}


/**
  * @功能:软件复位系统总线
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_SoftwareRst_SystemBus(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    SCU->SOFT_SYSRST_CFG &= ~(0x3 << 8);
    SCU->SOFT_SYSRST_CFG |= (0x3 << 8);
    SCU->SOFT_RST_PARAM = SOFTWARE_RESET_KEY;
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
}



/**
  * @功能:配置Twatchdog复位时复位系统
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_Twdg_RstSys_Config(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    SCU->SOFT_SYSRST_CFG &= ~(0x3 << 2);
    SCU->SOFT_SYSRST_CFG |= (0x2 << 2);
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
}


/**
  * @功能:配置Twatchdog复位时复位系统总线
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_Twdg_RstSysBus_Config(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    volatile unsigned int tmp;
    tmp = SCU->SOFT_SYSRST_CFG;
    tmp &= ~(0x3 << 2);
    tmp |= (0x2 << 2);
    SCU->SOFT_SYSRST_CFG = tmp;
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
}



/**
  * @功能:配置iwatchdog复位时复位系统
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_Iwdg_RstSys_Config(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    volatile unsigned int tmp;
    tmp = SCU->SOFT_SYSRST_CFG;
    tmp &= ~(0x3 << 0);
    tmp |= (0x2 << 0);
    SCU->SOFT_SYSRST_CFG = tmp;
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
}


/**
  * @功能:配置iwatchdog复位时复位系统总线
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_Iwdg_RstSysBus_Config(void)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    SCU->SOFT_SYSRST_CFG &= ~(0x3 << 0);
    SCU->SOFT_SYSRST_CFG |= (0x3 << 0);
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
}


/**
  * @功能:配置外设复位
  * @注意:配合Scu_Setdevice_ResetRelease  使用，先reset,然后release，外设复位完成
  * @参数:device_base 设备基地址
  * @返回值:PARA_ERROR: 参数错误   RETURN_OK: 配置完成
  */
int Scu_Setdevice_Reset(unsigned int device_base)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    if(device_base == HAL_GDMA_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 11);
    }
    else if(device_base == HAL_IISDMA0_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 10);
    }
    else if(device_base == HAL_CODEC_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 9);
    }
    else if(device_base == HAL_ALC_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 8);
    }
    else if(device_base == HAL_ADC_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 7);
    }
    else if(device_base == HAL_QSPI0_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 6);
    }
    else if(device_base == HAL_VAD_FFT_REG_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 5);
    }
    else if(device_base == HAL_VAD_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 4);
    }
    else if(device_base == HAL_FFT_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 3);
    }
    else if(device_base == HAL_DNN_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 2);
    }
    else if(device_base == HAL_TWDG_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 1);
    }
    else if(device_base == HAL_IWDG_BASE)
    {
        SCU->SOFT_PRERST0_CFG &= ~(0x1 << 0);
    }

    else if(device_base == HAL_IIC0_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 15);
    }
    else if(device_base == HAL_PWM5_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 14);
    }
    else if(device_base == HAL_PWM4_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 14);
    }
    else if(device_base == HAL_PWM3_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 13);
    }
    else if(device_base == HAL_PWM2_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 13);
    }
    else if(device_base == HAL_PWM1_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 12);
    }
    else if(device_base == HAL_PWM0_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 12);
    }
    else if(device_base == HAL_UART1_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 11);
    }
    else if(device_base == HAL_UART0_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 10);
    }
    else if(device_base == HAL_GPIO4_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 9);
    }
    else if(device_base == HAL_GPIO3_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 8);
    }
    else if(device_base == HAL_GPIO2_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 7);
    }
    else if(device_base == HAL_GPIO1_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 6);
    }
    else if(device_base == HAL_GPIO0_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 5);
    }
    else if(device_base == HAL_TIMER3_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 4);
    }
    else if(device_base == HAL_TIMER2_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 4);
    }
    else if(device_base == HAL_TIMER1_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 3);
    }
    else if(device_base == HAL_TIMER0_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 3);
    }
    else if(device_base == HAL_IIS3_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 2);
    }
    else if(device_base == HAL_IIS2_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 1);
    }
    else if(device_base == HAL_IIS1_BASE)
    {
        SCU->SOFT_PRERST1_CFG &= ~(0x1 << 0);
    }

    else
    {
        Lock_Rstconfig();
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    
    _delay(2);
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:配置外设复位释放
  * @注意:配合Scu_Setdevice_ResetRelease  使用，先reset,然后release，外设复位完成
  * @参数:device_base 设备基地址
  * @返回值:PARA_ERROR: 参数错误   RETURN_OK: 配置完成
  */
int Scu_Setdevice_ResetRelease(unsigned int device_base)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Rstconfig();
    if(device_base == HAL_GDMA_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 11);
    }
    else if(device_base == HAL_IISDMA0_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 10);
    }
    else if(device_base == HAL_CODEC_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 9);
    }
    else if(device_base == HAL_ALC_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 8);
    }
    else if(device_base == HAL_ADC_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 7);
    }
    else if(device_base == HAL_QSPI0_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 6);
    }
    else if(device_base == HAL_VAD_FFT_REG_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 5);
    }
    else if(device_base == HAL_VAD_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 4);
    }
    else if(device_base == HAL_FFT_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 3);
    }
    else if(device_base == HAL_DNN_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 2);
    }
    else if(device_base == HAL_TWDG_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 1);
    }
    else if(device_base == HAL_IWDG_BASE)
    {
        SCU->SOFT_PRERST0_CFG |= (0x1 << 0);
    }

    else if(device_base == HAL_IIC0_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 15);
    }
    else if(device_base == HAL_PWM5_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 14);
    }
    else if(device_base == HAL_PWM4_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 14);
    }
    else if(device_base == HAL_PWM3_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 13);
    }
    else if(device_base == HAL_PWM2_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 13);
    }
    else if(device_base == HAL_PWM1_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 12);
    }
    else if(device_base == HAL_PWM0_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 12);
    }
    else if(device_base == HAL_UART1_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 11);
    }
    else if(device_base == HAL_UART0_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 10);
    }
    else if(device_base == HAL_GPIO4_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 9);
    }
    else if(device_base == HAL_GPIO3_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 8);
    }
    else if(device_base == HAL_GPIO2_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 7);
    }
    else if(device_base == HAL_GPIO1_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 6);
    }
    else if(device_base == HAL_GPIO0_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 5);
    }
    else if(device_base == HAL_TIMER3_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 4);
    }
    else if(device_base == HAL_TIMER2_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 4);
    }
    else if(device_base == HAL_TIMER1_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 3);
    }
    else if(device_base == HAL_TIMER0_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 3);
    }
    else if(device_base == HAL_IIS3_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 2);
    }
    else if(device_base == HAL_IIS2_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 1);
    }
    else if(device_base == HAL_IIS1_BASE)
    {
        SCU->SOFT_PRERST1_CFG |= (0x1 << 0);
    }

    else
    {
        Lock_Rstconfig();
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    
    _delay(2);
    Lock_Rstconfig();
    scu_exit_critical(base_pri);
    return RETURN_OK;
}


/**
  * @功能:设置IIS时钟参数
  * @注意:无
  * @参数:device_base IIS基地址
  * @返回值:PARA_ERROR: 参数错误  RETURN_OK: 配置成功
  */
#if 0
int Scu_SetIISCLK_Config(IIS_CLK_CONFIGTypeDef *piis_config)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    if(piis_config->device_select == IISNum0)
    {
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    else if(piis_config->device_select == IISNum1)
    {
        /*先选时钟源*/
        SCU->CLK_SEL_CFG &= ~(1 << 1);
        SCU->CLK_SEL_CFG |= (piis_config->clk_source << 1);
        Scu_Para_En_Disable((uint32_t)IIS1);
        _delay(1);
        /*配置分频系数 */
        SCU->CLKDIV_PARAM3_CFG &= ~(DIV_MASK8 << 0);
        SCU->CLKDIV_PARAM3_CFG |= ((piis_config->div_param & DIV_MASK8) << 0);
        Scu_Para_En_Enable((uint32_t)IIS1);
        _delay(1);
        SCU->IIS1_CLK_CFG &= ~(1 << 7);
        _delay(1);
        SCU->IIS1_CLK_CFG = (piis_config->mclk_out_en << 5)
                        | (piis_config->sck_wid << 2) | (piis_config->over_sample << 3)
                        | (piis_config->model_sel << 0) | (piis_config->sck_ext_inv_sel << 1);
        SCU->IIS1_CLK_CFG |= (1 << 7);
        
        if(0 == piis_config->model_sel)
        {
            SCU->IIS1_CLK_CFG |= (1 << 6);
        }
        else
        {
            SCU->IIS1_CLK_CFG &= ~(1 << 6);
        }
    }
    else if(piis_config->device_select == IISNum2)
    {
        SCU->CLK_SEL_CFG &= ~(1 << 2);
        SCU->CLK_SEL_CFG |= (piis_config->clk_source << 2);
        Scu_Para_En_Disable((uint32_t)IIS2);
        _delay(1);
        SCU->CLKDIV_PARAM3_CFG &= ~(DIV_MASK8 << 8);
        SCU->CLKDIV_PARAM3_CFG |= ((piis_config->div_param & DIV_MASK8) << 8);
        Scu_Para_En_Enable((uint32_t)IIS2);
        _delay(1);
        SCU->IIS2_CLK_CFG &= ~(1 << 7);
        _delay(1);
        SCU->IIS2_CLK_CFG = (piis_config->mclk_out_en << 5) 
                        | (piis_config->sck_wid << 2) | (piis_config->over_sample << 3) 
                        | (piis_config->model_sel << 0) | (piis_config->sck_ext_inv_sel << 1);
        SCU->IIS2_CLK_CFG |= (1 << 7);
        
        if(0 == piis_config->model_sel)
        {
            SCU->IIS2_CLK_CFG |= (1 << 6);
        }
        else
        {
            SCU->IIS2_CLK_CFG &= ~(1 << 6);
        }
    }
    else if(piis_config->device_select == IISNum3)
    {
        SCU->CLK_SEL_CFG &= ~(1 << 3);
        SCU->CLK_SEL_CFG |= (piis_config->clk_source << 3);
        Scu_Para_En_Disable((uint32_t)IIS3);
        _delay(1);
        SCU->CLKDIV_PARAM3_CFG &= ~(DIV_MASK8 << 16);
        SCU->CLKDIV_PARAM3_CFG |= ((piis_config->div_param & DIV_MASK8) << 16);
        Scu_Para_En_Enable((uint32_t)IIS3);
        _delay(1);
        SCU->IIS3_CLK_CFG &= ~(1 << 7);
        _delay(1);
        SCU->IIS3_CLK_CFG = (piis_config->mclk_out_en << 5) 
                        | (piis_config->sck_wid << 2) | (piis_config->over_sample << 3)
                        | (piis_config->model_sel << 0) | (piis_config->sck_ext_inv_sel << 1);
        SCU->IIS3_CLK_CFG |= (1 << 7);
        
        if(0 == piis_config->model_sel)
        {
            SCU->IIS3_CLK_CFG |= (1 << 6);
        }
        else
        {
            SCU->IIS3_CLK_CFG &= ~(1 << 6);
        }
    }
    else
    {
        scu_exit_critical(base_pri);
        Lock_Ckconfig();
        return PARA_ERROR;
    }
    _delay(1);
    scu_exit_critical(base_pri);
    Lock_Ckconfig();
    return RETURN_OK;
}
#else
int Scu_SetIISCLK_Config(IIS_CLK_CONFIGTypeDef *piis_config)
{
    uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    if(piis_config->device_select == IISNum0)
    {
        scu_exit_critical(base_pri);
        return PARA_ERROR;
    }
    else if(piis_config->device_select == IISNum1)
    {
        Scu_Para_En_Disable((uint32_t)IIS1);
        _delay(1);
        SCU->CLKDIV_PARAM3_CFG &= ~(DIV_MASK8 << 0);
        SCU->CLKDIV_PARAM3_CFG |= ((piis_config->div_param & DIV_MASK8) << 0);
        Scu_Para_En_Enable((uint32_t)IIS1);
        _delay(1);
        SCU->CLK_SEL_CFG &= ~(1 << 1);
        SCU->CLK_SEL_CFG |= (piis_config->clk_source << 1);
        _delay(1);
        Scu_SetDeviceGate(HAL_MCLK1_BASE,1);
        Unlock_Ckconfig();
        SCU->IIS1_CLK_CFG &= ~(1 << 7);
        _delay(1);
        SCU->IIS1_CLK_CFG = (piis_config->mclk_out_en << 5)
                        | (piis_config->sck_wid << 2) | (piis_config->over_sample << 3)
                        | (piis_config->model_sel << 0) | (piis_config->sck_ext_inv_sel << 1);
        SCU->IIS1_CLK_CFG |= (1 << 7);
        
        if(0 == piis_config->model_sel)
        {
            SCU->IIS1_CLK_CFG |= (1 << 6);
        }
        else
        {
            SCU->IIS1_CLK_CFG &= ~(1 << 6);
        }
    }
    else if(piis_config->device_select == IISNum2)
    {
        Scu_Para_En_Disable((uint32_t)IIS2);
        _delay(1);
        SCU->CLKDIV_PARAM3_CFG &= ~(DIV_MASK8 << 8);
        SCU->CLKDIV_PARAM3_CFG |= ((piis_config->div_param & DIV_MASK8) << 8);
        Scu_Para_En_Enable((uint32_t)IIS2);
        _delay(1);
        SCU->CLK_SEL_CFG &= ~(1 << 2);
        SCU->CLK_SEL_CFG |= (piis_config->clk_source << 2);
        _delay(1);
        Scu_SetDeviceGate(HAL_MCLK2_BASE,1);
        Unlock_Ckconfig();
        SCU->IIS2_CLK_CFG &= ~(1 << 7);
        _delay(1);
        SCU->IIS2_CLK_CFG = (piis_config->mclk_out_en << 5) 
                        | (piis_config->sck_wid << 2) | (piis_config->over_sample << 3) 
                        | (piis_config->model_sel << 0) | (piis_config->sck_ext_inv_sel << 1);
        SCU->IIS2_CLK_CFG |= (1 << 7);
        
        if(0 == piis_config->model_sel)
        {
            SCU->IIS2_CLK_CFG |= (1 << 6);
        }
        else
        {
            SCU->IIS2_CLK_CFG &= ~(1 << 6);
        }
    }
    else if(piis_config->device_select == IISNum3)
    {
        Scu_Para_En_Disable((uint32_t)IIS3);
        _delay(1);
        SCU->CLKDIV_PARAM3_CFG &= ~(DIV_MASK8 << 16);
        SCU->CLKDIV_PARAM3_CFG |= ((piis_config->div_param & DIV_MASK8) << 16);
        //SCU->CLKDIV_PARAM3_CFG |= ((15) << 16);
        Scu_Para_En_Enable((uint32_t)IIS3);
        _delay(1);
        SCU->CLK_SEL_CFG &= ~(1 << 3);
        SCU->CLK_SEL_CFG |= (piis_config->clk_source << 3);
        _delay(1);
        Scu_SetDeviceGate(HAL_MCLK3_BASE,1);
        Unlock_Ckconfig();
        SCU->IIS3_CLK_CFG &= ~(1 << 7);
        _delay(1);
        SCU->IIS3_CLK_CFG = (piis_config->mclk_out_en << 5) 
                        | (piis_config->sck_wid << 2) | (piis_config->over_sample << 3)
                        | (piis_config->model_sel << 0) | (piis_config->sck_ext_inv_sel << 1);
        SCU->IIS3_CLK_CFG |= (1 << 7);
        
        if(0 == piis_config->model_sel)
        {
            SCU->IIS3_CLK_CFG |= (1 << 6);
        }
        else
        {
            SCU->IIS3_CLK_CFG &= ~(1 << 6);
        }
    }
    else
    {
        scu_exit_critical(base_pri);
        Lock_Ckconfig();
        return PARA_ERROR;
    }
    _delay(1);
    scu_exit_critical(base_pri);
    Lock_Ckconfig();
    return RETURN_OK;
}
#endif


void iis3_test_mode_set(iis_mode_sel_t mode)
{
    if(IIS_SLAVE == mode)
    {
        SCU->CODEC_TEST_CFG |= (1 << 3);
    }
    else
    {
        SCU->CODEC_TEST_CFG &= ~(1 << 3);
    }
}


void iis2_test_mode_set(iis_mode_sel_t mode,iis2_test_mode_data_source_t data)
{
    if(IIS_SLAVE == mode)
    {
        SCU->CODEC_TEST_CFG |= (1 << 2);
    }
    else
    {
        SCU->CODEC_TEST_CFG &= ~(1 << 2);
    }
    SCU->CODEC_TEST_CFG &= ~(1 << 0);
    SCU->CODEC_TEST_CFG |= (data << 0);
} 


/**
  * @功能:配置在低功耗模式下的中断唤醒源
  * @注意:无
  * @参数:irq    外设中断号
  * @返回值:无
  */
void Scu_SetWakeup_Source(int irq)
{
    uint32_t  base_pri = scu_enter_critical();

    uint32_t WAKE_UP_MASK = SCU->WAKEUP_MASK_CFG;
    
    if(irq == EXTI0_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 1);
    }
    else if(irq == EXTI1_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 2);
    }
    else if(irq == DNN_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 5);
    }
    else if(irq == IIS_DMA0_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 6);
    }
    else if(irq == ADC_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 7);
    }
    else if(irq == UART0_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 8);
    }
    else if(irq == UART1_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 9);
    }
    else if(irq == TIMER0_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 10);
    }
    else if(irq == TIMER1_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 11);
    }
    else if(irq == TIMER2_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 12);
    }
    else if(irq == TIMER3_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 13);
    }
    else if(irq == IIS1_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 14);
    }
    else if(irq == IIS2_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 15);
    }  
    else if(irq == IIS3_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 16);
    }
    else if(irq == ALC_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 17);
    }
    else if(irq == GPIO0_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 19);
    }
    else if(irq == GPIO1_IRQn)
    {
        WAKE_UP_MASK |= (0x1 << 20);
    }
    SCU->WAKEUP_MASK_CFG = WAKE_UP_MASK;
    
    scu_exit_critical(base_pri);
}


void Scu_SetNMIIRQ(int irq)
{
    uint32_t  base_pri = scu_enter_critical();

    Unlock_Ckconfig();
    unlock_sysctrl_config();
    uint32_t SOFT_SYS_CTRL_CFG = SCU->SYS_CTRL_CFG;
    
    SOFT_SYS_CTRL_CFG &= ~(0xf << 3);
    if(irq == TWDG_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x1 << 3);
    }
    else if(irq == IWDG_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x2 << 3);
    }
    else if(irq == EXTI0_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x3 << 3);
    }
    else if(irq == EXTI1_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x4 << 3);
    }
    else if(irq == DNN_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x7 << 3);
    }
    else if(irq == IIS_DMA0_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x8 << 3);
    }
    else if(irq == ADC_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x9 << 3);
    }
    else if(irq == UART0_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x10 << 3);
    }
    else if(irq == UART1_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x11 << 3);
    }
    else if(irq == TIMER0_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x12 << 3);
    }
    else if(irq == TIMER1_IRQn)
    {
        SOFT_SYS_CTRL_CFG |= (0x13 << 3);
    }
    SCU->SYS_CTRL_CFG = SOFT_SYS_CTRL_CFG;
    
    Lock_Ckconfig();
    lock_sysctrl_config();

    scu_exit_critical(base_pri);
}

/**
  * @功能:配置管脚复用对应功能
  * @注意:无
  * @参数:pin : 管脚名;  io_function: 第X功能选择 select
  * @返回值:无
  */
void Scu_SetIOReuse(PinPad_Name pin,IOResue_FUNCTION io_function)
{
    uint32_t new_iocfg,new_iocfg1;

    uint32_t  base_pri = scu_enter_critical();
    
    new_iocfg = SCU->IO_REUSE0_CFG;
    new_iocfg1 =  SCU->IO_REUSE1_CFG;

    switch(pin)
    {
    	case UART0_TX_PAD:
			new_iocfg &= ~(0x3 << 0);
			new_iocfg |= (io_function << 0);
			break;
		case UART0_RX_PAD:
			new_iocfg &= ~(0x3 << 2);
			new_iocfg |= (io_function << 2);
			break;
		case UART1_TX_PAD:
			new_iocfg &= ~(0x3 << 4);
			new_iocfg |= (io_function << 4);
			break;
		case UART1_RX_PAD:
			new_iocfg &= ~(0x3 << 6);
			new_iocfg |= (io_function << 6);
			break;
		case I2C0_SCL_PAD:
		case I2C0_SDA_PAD:
			new_iocfg &= ~(0x3 << 8);
			new_iocfg |= (io_function << 8);
			break;
		case I2S1_MCLK_PAD:
		case I2S1_SCLK_PAD:
		case I2S1_LRCLK_PAD:
			new_iocfg &= ~(0x3 << 10);
			new_iocfg |= (io_function << 10);
			break;
		case I2S1_SDO_PAD:
			new_iocfg &= ~(0x3 << 12);
			new_iocfg |= (io_function << 12);
			break;
		case I2S1_SDI_PAD:
			new_iocfg &= ~(0x3 << 14);
			new_iocfg |= (io_function << 14);
			break;
		case GPIO0_6_PAD:
		case GPIO0_5_PAD:
			new_iocfg &= ~(0x3 << 16);
			new_iocfg |= (io_function << 16);
			break;
		case PWM0_PAD:
			new_iocfg &= ~(0x3 << 18);
			new_iocfg |= (io_function << 18);
			break;
		case PWM1_PAD:
			new_iocfg &= ~(0x3 << 20);
			new_iocfg |= (io_function << 20);
			break;
		case PWM2_PAD:
			new_iocfg &= ~(0x3 << 22);
			new_iocfg |= (io_function << 22);
			break;
		case PWM3_PAD:
			new_iocfg &= ~(0x3 << 24);
			new_iocfg |= (io_function << 24);
			break;
		case PWM4_PAD:
			new_iocfg &= ~(0x3 << 26);
			new_iocfg |= (io_function << 26);
			break;
		case PWM5_PAD:
			new_iocfg &= ~(0x3 << 28);
			new_iocfg |= (io_function << 28);
			break;
		case AIN0_PAD:
			new_iocfg &= ~(0x3 << 30);
			new_iocfg |= (io_function << 30);
			break;
		case AIN1_PAD:
			new_iocfg1 &= ~(0x3 << 0);
			new_iocfg1 |= (io_function << 0);
			break;
		case GPIO0_7_PAD:
			new_iocfg1 &= ~(0x3 << 2);
			new_iocfg1 |= (io_function << 2);
			break;
		case GPIO1_0_PAD:
			new_iocfg1 &= ~(0x3 << 4);
			new_iocfg1 |= (io_function << 4);
			break;
		case GPIO0_4_PAD:
			new_iocfg1 &= ~(0x3 << 6);
			new_iocfg1 |= (io_function << 6);
			break;
		case GPIO3_2_PAD:
			new_iocfg1 &= ~(0x3 << 8);
			new_iocfg1 |= (io_function << 8);
			break;
		case AIN2_PAD:
			new_iocfg1 &= ~(0x3 << 10);
			new_iocfg1 |= (io_function << 10);
			break;
		case AIN3_PAD:
			new_iocfg1 &= ~(0x3 << 12);
			new_iocfg1 |= (io_function << 12);
			break;
		default:
			break;
    }

    if(SCU->IO_REUSE0_CFG != new_iocfg)
    {
    	SCU->IO_REUSE0_CFG = new_iocfg;
    }

    if(SCU->IO_REUSE1_CFG != new_iocfg1)
    {
    	SCU->IO_REUSE1_CFG = new_iocfg1;
    }
    scu_exit_critical(base_pri);
}


/**
  * @功能:配置管脚数字模拟功能
  * @注意:无
  * @参数:pin : 管脚名;  adio_mode: 第X功能选择 select
  * @返回值:无
  */
void Scu_SetADIO_REUSE(PinPad_Name pin,ADIOResue_MODE adio_mode)
{
    if (pin >= AIN0_PAD && pin <= AIN3_PAD)
    {
        uint32_t  base_pri = scu_enter_critical();
        uint32_t new_adiocfg = SCU->AD_IO_REUSE0_CFG;

        if(DIGITAL_MODE == adio_mode)
        {
            new_adiocfg &= ~(0x1 << (pin - AIN0_PAD));
            switch(pin)
            {
                case AIN0_PAD:
                    new_adiocfg &= ~(0x1 << 0);
                    break;
                case AIN1_PAD:
                    new_adiocfg &= ~(0x1 << 1);
                    break;
                case AIN2_PAD:
					new_adiocfg &= ~(0x1 << 2);
					break;
				case AIN3_PAD:
					new_adiocfg &= ~(0x1 << 3);
					break;
            }
        }
        else if(ANALOG_MODE == adio_mode)
        {
            new_adiocfg |= (0x1 << (pin - AIN0_PAD));
            switch(pin)
            {
                case AIN0_PAD:
                    new_adiocfg |= (0x1 << 0);
                    break;
                case AIN1_PAD:
                    new_adiocfg |= (0x1 << 1);
                    break;
                case AIN2_PAD:
					new_adiocfg |= (0x1 << 2);
					break;
				case AIN3_PAD:
					new_adiocfg |= (0x1 << 3);
					break;
            }
        }

        if(SCU->AD_IO_REUSE0_CFG != new_adiocfg)
        {
            SCU->AD_IO_REUSE0_CFG = new_adiocfg;
        }
        scu_exit_critical(base_pri);
    }
}


#if 0
/**
  * @功能:对所有SCU控制的外设复位，并释放复位
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_AllPreipheralRst(void)
{
    uint32_t  base_pri = scu_enter_critical();
	Unlock_Rstconfig();
	SCU->SOFT_PRERST_CTRL0 = 0x0;
	SCU->SOFT_PRERST_CTRL1 = 0x0;
	for(volatile unsigned int i=0xFFFF;i;i--);
	SCU->SOFT_PRERST_CTRL0 = 0xFFFFFFFF;
	SCU->SOFT_PRERST_CTRL1 = 0xFFFFFFFF;
	Lock_Rstconfig();
    scu_exit_critical(base_pri);
}
#endif

#if 0
/**
  * @功能:关闭SCU控制的部分外设(除wdg、sdram)时钟
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_AllPreipheralclkclose(void)
{
    uint32_t  base_pri = scu_enter_critical();
	Unlock_Ckconfig();
	SCU->PRE_CLKGATE0 &= ~((0xFF<<0)|(0xF<<11)|(0xF<<20)|(0x3F<<26)); 
	SCU->PRE_CLKGATE1 &= ~((0x1FF<<0)|(0x7<<12)|(0xF<<16)|(0x7<<20)); 
	Lock_Ckconfig();
    scu_exit_critical(base_pri);
}
#endif

#if 0
/**
  * @功能:打开SCU控制的部分外设(除wdg、sdram)时钟
  * @注意:无
  * @参数:无
  * @返回值:无
  */
void Scu_AllPreipheralclkopen(void)
{
    uint32_t  base_pri = scu_enter_critical();
	Unlock_Ckconfig();
	SCU->PRE_CLKGATE0 |= ((0xFF<<0)|(0xF<<11)|(0xF<<20)|(0x3F<<26)); 
	SCU->PRE_CLKGATE1 |= ((0x1FF<<0)|(0x7<<12)|(0xF<<16)|(0x7<<20)); 
	Lock_Ckconfig();
    scu_exit_critical(base_pri);
}
#endif

#if 0
void scu_qspi_clksrc_choice(unsigned int device_base,uint32_t pll_src_en)
{
    uint32_t  base_pri = scu_enter_critical();
    if(device_base == HAL_QSPI0_BASE)
    {
        if(1 == pll_src_en)
        {
            SCU->SPI0_CLK_CFG |= (1<<6); 
        }
        else
        {
            SCU->SPI0_CLK_CFG &= ~(1<<6); 
        }
    }
    scu_exit_critical(base_pri);
}
#endif

/**
  * @功能:配置QSPI 0 非boot 模式
  * @注意:无
  * @参数:1.无
  * @返回值:无
  */
void scu_spiflash_noboot_set(void)
{
    uint32_t  base_pri = scu_enter_critical();
    unlock_sysctrl_config();
    SCU->SYS_CTRL_CFG &= ~(1<<0);
    lock_sysctrl_config();
    scu_exit_critical(base_pri);
}


/**
 * @brief 设置io上下拉电阻使能
 * 
 * @param pin 
 * @param cmd 
 */
void Scu_SetIOPull(PinPad_Name pin,FunctionalState cmd)
{
	uint32_t new_iocfg,new_iocfg1;

	uint32_t  base_pri = scu_enter_critical();

	new_iocfg = SCU->IOPULL0_CFG;
	new_iocfg1 =  SCU->IOPULL1_CFG;
	if(cmd)
	{
        switch(pin)
		{
			case UART0_TX_PAD:
				new_iocfg &= ~(0x1 << 0);
				break;
			case UART0_RX_PAD:
				new_iocfg &= ~(0x1 << 1);
				break;
			case UART1_TX_PAD:
				new_iocfg &= ~(0x1 << 2);
				break;
			case UART1_RX_PAD:
				new_iocfg &= ~(0x1 << 3);
				break;
			case I2C0_SCL_PAD:
				new_iocfg &= ~(0x1 << 4);
				break;
			case I2C0_SDA_PAD:
				new_iocfg &= ~(0x1 << 5);
				break;
			case I2S1_MCLK_PAD:
				new_iocfg &= ~(0x1 << 6);
				break;
			case GPIO0_6_PAD:
				new_iocfg &= ~(0x1 << 11);
				break;
			case GPIO0_5_PAD:
				new_iocfg &= ~(0x1 << 12);
				break;
			case PWM0_PAD:
				new_iocfg &= ~(0x1 << 19);
				break;
			case PWM1_PAD:
				new_iocfg &= ~(0x1 << 20);
				break;
			case PWM2_PAD:
				new_iocfg &= ~(0x1 << 21);
				break;
			case PWM3_PAD:
				new_iocfg &= ~(0x1 << 22);
				break;
			case PWM4_PAD:
				new_iocfg &= ~(0x1 << 23);
				break;
			case PWM5_PAD:
				new_iocfg &= ~(0x1 << 24);
				break;
			case AIN0_PAD:
				new_iocfg &= ~(0x1 << 25);
				break;
			case AIN1_PAD:
				new_iocfg &= ~(0x1 << 26);
				break;
			case GPIO0_7_PAD:
				new_iocfg &= ~(0x1 << 27);
				break;
			case GPIO1_0_PAD:
				new_iocfg &= ~(0x1 << 28);
				break;
			case GPIO3_2_PAD:
				new_iocfg &= ~(0x1 << 29);
				break;
			case AIN2_PAD:
				new_iocfg &= ~(0x1 << 30);
				break;
			case AIN3_PAD:
				new_iocfg &= ~(0x1 << 31);
				break;
			default:
				break;
		}
	}
	else
	{
		switch(pin)
		{
			case UART0_TX_PAD:
				new_iocfg |= (0x1 << 0);
				break;
			case UART0_RX_PAD:
				new_iocfg |= (0x1 << 1);
				break;
			case UART1_TX_PAD:
				new_iocfg |= (0x1 << 2);
				break;
			case UART1_RX_PAD:
				new_iocfg |= (0x1 << 3);
				break;
			case I2C0_SCL_PAD:
				new_iocfg |= (0x1 << 4);
				break;
			case I2C0_SDA_PAD:
				new_iocfg |= (0x1 << 5);
				break;
			case I2S1_MCLK_PAD:
				new_iocfg |= (0x1 << 6);
				break;
			case GPIO0_6_PAD:
				new_iocfg |= (0x1 << 11);
				break;
			case GPIO0_5_PAD:
				new_iocfg |= (0x1 << 12);
				break;
			case PWM0_PAD:
				new_iocfg |= (0x1 << 19);
				break;
			case PWM1_PAD:
				new_iocfg |= (0x1 << 20);
				break;
			case PWM2_PAD:
				new_iocfg |= (0x1 << 21);
				break;
			case PWM3_PAD:
				new_iocfg |= (0x1 << 22);
				break;
			case PWM4_PAD:
				new_iocfg |= (0x1 << 23);
				break;
			case PWM5_PAD:
				new_iocfg |= (0x1 << 24);
				break;
			case AIN0_PAD:
				new_iocfg |= (0x1 << 25);
				break;
			case AIN1_PAD:
				new_iocfg |= (0x1 << 26);
				break;
			case GPIO0_7_PAD:
				new_iocfg |= (0x1 << 27);
				break;
			case GPIO1_0_PAD:
				new_iocfg |= (0x1 << 28);
				break;
			case GPIO3_2_PAD:
				new_iocfg |= (0x1 << 29);
				break;
			case AIN2_PAD:
				new_iocfg |= (0x1 << 30);
				break;
			case AIN3_PAD:
				new_iocfg |= (0x1 << 31);
				break;
			default:
				break;
		}
	}

	if(SCU->IOPULL0_CFG != new_iocfg)
	{
		SCU->IOPULL0_CFG = new_iocfg;
	}

	if(SCU->IOPULL1_CFG != new_iocfg1)
	{
		SCU->IOPULL1_CFG = new_iocfg1;
	}
	scu_exit_critical(base_pri);
}


void scu_iwdg_osc_clk(void)
{
	uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    SCU->CLK_SEL_CFG |= (1<<6);
    Lock_Ckconfig();
	scu_exit_critical(base_pri);

}

void scu_iwdg_ipcore_clk(void)
{
	uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    SCU->CLK_SEL_CFG &= ~(1<<6);
    Lock_Ckconfig();
    scu_exit_critical(base_pri);
}

void scu_run_in_flash(void)
{
	uint32_t  base_pri = scu_enter_critical();
    Unlock_Ckconfig();
    unlock_sysctrl_config();
    SCU->SYS_CTRL_CFG |= (1<<7);
    lock_sysctrl_config();
    Lock_Ckconfig();
    scu_exit_critical(base_pri);
}


int32_t Scu_GetSysResetState(void)
{
    uint32_t  scu_state = SCU->SCU_STATE_REG;
    SCU->SCU_STATE_REG |= scu_state;
    if(scu_state & (0x17 << 3))
    {
        return RETURN_ERR;
    }
    else
    {
        return RETURN_OK;
    }
}


/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/






