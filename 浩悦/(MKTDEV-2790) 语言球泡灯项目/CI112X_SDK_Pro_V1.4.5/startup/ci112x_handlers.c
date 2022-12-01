/**
 * @file ci112x_handlers.c
 * @brief 
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
#include "ci_assert.h"
#include "ci_log.h"

typedef struct
{
	uint32_t zer0, ra, sp, gp, tp, t0, t1, t2, fp, s1, a0, a1, a2, a3, a4, a5, a6, a7, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, t3, t4, t5, t6, mstatus, mepc, msubm, mcause;
}exception_registers_t;

__WEAK void	_exit(int __status)
{
    CI_ASSERT(0,"application exit\n");
    for(;;);
}

__WEAK uintptr_t handle_nmi(void)
{
    ci_loginfo(LOG_FAULT_INT, "nmi\n");
    _exit(1);
    return 0;
}

uintptr_t handle_trap(uintptr_t mcause, uintptr_t sp)
{
    if ((mcause&0xFFF) == 0xFFF)
    {
        handle_nmi();
    }
    else
    {
        exception_registers_t *regs = (exception_registers_t *)sp;
        ci_loginfo(LOG_FAULT_INT,"application trap\n");
        ci_loginfo(LOG_FAULT_INT, "----------------------------------\n");
        ci_loginfo(LOG_FAULT_INT, "ra\t = 0x%x\n",regs->ra);
        ci_loginfo(LOG_FAULT_INT, "sp\t = 0x%x\n",regs->sp);
        ci_loginfo(LOG_FAULT_INT, "t0\t = 0x%x\n",regs->t0);
        ci_loginfo(LOG_FAULT_INT, "t1\t = 0x%x\n",regs->t1);
        ci_loginfo(LOG_FAULT_INT, "t2\t = 0x%x\n",regs->t2);
        ci_loginfo(LOG_FAULT_INT, "fp(s0)\t = 0x%x\n",regs->fp);
        ci_loginfo(LOG_FAULT_INT, "s1\t = 0x%x\n",regs->s1);
        ci_loginfo(LOG_FAULT_INT, "a0\t = 0x%x\n",regs->a0);
        ci_loginfo(LOG_FAULT_INT, "a1\t = 0x%x\n",regs->a1);
        ci_loginfo(LOG_FAULT_INT, "a2\t = 0x%x\n",regs->a2);
        ci_loginfo(LOG_FAULT_INT, "a3\t = 0x%x\n",regs->a3);
        ci_loginfo(LOG_FAULT_INT, "a4\t = 0x%x\n",regs->a4);
        ci_loginfo(LOG_FAULT_INT, "a5\t = 0x%x\n",regs->a5);
        ci_loginfo(LOG_FAULT_INT, "a6\t = 0x%x\n",regs->a6);
        ci_loginfo(LOG_FAULT_INT, "a7\t = 0x%x\n",regs->a7);
        ci_loginfo(LOG_FAULT_INT, "s2\t = 0x%x\n",regs->s2);
        ci_loginfo(LOG_FAULT_INT, "s3\t = 0x%x\n",regs->s3);
        ci_loginfo(LOG_FAULT_INT, "s4\t = 0x%x\n",regs->s4);
        ci_loginfo(LOG_FAULT_INT, "s5\t = 0x%x\n",regs->s5);
        ci_loginfo(LOG_FAULT_INT, "s6\t = 0x%x\n",regs->s6);
        ci_loginfo(LOG_FAULT_INT, "s7\t = 0x%x\n",regs->s7);
        ci_loginfo(LOG_FAULT_INT, "s8\t = 0x%x\n",regs->s8);
        ci_loginfo(LOG_FAULT_INT, "s9\t = 0x%x\n",regs->s9);
        ci_loginfo(LOG_FAULT_INT, "s10\t = 0x%x\n",regs->s10);
        ci_loginfo(LOG_FAULT_INT, "s11\t = 0x%x\n",regs->s11);
        ci_loginfo(LOG_FAULT_INT, "t3\t = 0x%x\n",regs->t3);
        ci_loginfo(LOG_FAULT_INT, "t4\t = 0x%x\n",regs->t4);
        ci_loginfo(LOG_FAULT_INT, "t5\t = 0x%x\n",regs->t5);
        ci_loginfo(LOG_FAULT_INT, "t6\t = 0x%x\n",regs->t6);
        ci_loginfo(LOG_FAULT_INT, "mstatus\t = 0x%x\n",regs->mstatus);
        ci_loginfo(LOG_FAULT_INT, "msubm\t = 0x%x\n",regs->msubm);
        ci_loginfo(LOG_FAULT_INT, "mepc\t = 0x%x\n",regs->mepc);
        ci_loginfo(LOG_FAULT_INT, "mcause\t = 0x%x\n",regs->mcause);
        ci_loginfo(LOG_FAULT_INT, "mdcause\t = 0x%x\n",read_csr(0x7c9));//mdcause
        ci_loginfo(LOG_FAULT_INT, "mtval\t = 0x%x\n",read_csr(0x343));
        ci_loginfo(LOG_FAULT_INT, "----------------------------------\n");
		_exit(mcause);
    }
    return 0;
}
