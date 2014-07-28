/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_DEBUGV8_H
#define __ASM_DEBUGV8_H

#include <linux/types.h>

/* 32 bit register reads for aarch 64 bit */
#define dbg_readl(reg)			RSYSL_##reg()
/* 64 bit register reads for aarch 64 bit */
#define dbg_readq(reg)			RSYSQ_##reg()
/* 32 and 64 bit register writes for aarch 64 bit */
#define dbg_write(val, reg)		WSYS_##reg(val)

#define MRSL(reg)				\
({						\
uint32_t val;					\
asm volatile("mrs %0, "#reg : "=r" (val));	\
val;						\
})

#define MRSQ(reg)				\
({						\
uint64_t val;					\
asm volatile("mrs %0, "#reg : "=r" (val));	\
val;						\
})

#define MSR(val, reg)				\
({						\
asm volatile("msr "#reg", %0" : : "r" (val));	\
})

/*
 * Debug Feature Register
 *
 * Read only
 */
#define RSYSQ_ID_AA64DFR0_EL1()		MRSQ(ID_AA64DFR0_EL1)

/*
 * Debug Registers
 *
 * Available only in DBGv8
 *
 * Read only
 * MDCCSR_EL0, MDRAR_EL1, OSLSR_EL1, DBGDTRRX_EL0, DBGAUTHSTATUS_EL1
 *
 * Write only
 * DBGDTRTX_EL0, OSLAR_EL1
 */
/* 32 bit registers */
#define RSYSL_DBGDTRRX_EL0()		MRSL(DBGDTRRX_EL0)
#define RSYSL_MDCCSR_EL0()		MRSL(MDCCSR_EL0)
#define RSYSL_MDSCR_EL1()		MRSL(MDSCR_EL1)
#define RSYSL_OSDTRRX_EL1()		MRSL(OSDTRRX_EL1)
#define RSYSL_OSDTRTX_EL1()		MRSL(OSDTRTX_EL1)
#define RSYSL_OSDLR_EL1()		MRSL(OSDLR_EL1)
#define RSYSL_OSLSR_EL1()		MRSL(OSLSR_EL1)
#define RSYSL_MDCCINT_EL1()		MRSL(MDCCINT_EL1)
#define RSYSL_OSECCR_EL1()		MRSL(OSECCR_EL1)
#define RSYSL_DBGPRCR_EL1()		MRSL(DBGPRCR_EL1)
#define RSYSL_DBGBCR0_EL1()		MRSL(DBGBCR0_EL1)
#define RSYSL_DBGBCR1_EL1()		MRSL(DBGBCR1_EL1)
#define RSYSL_DBGBCR2_EL1()		MRSL(DBGBCR2_EL1)
#define RSYSL_DBGBCR3_EL1()		MRSL(DBGBCR3_EL1)
#define RSYSL_DBGBCR4_EL1()		MRSL(DBGBCR4_EL1)
#define RSYSL_DBGBCR5_EL1()		MRSL(DBGBCR5_EL1)
#define RSYSL_DBGBCR6_EL1()		MRSL(DBGBCR6_EL1)
#define RSYSL_DBGBCR7_EL1()		MRSL(DBGBCR7_EL1)
#define RSYSL_DBGBCR8_EL1()		MRSL(DBGBCR8_EL1)
#define RSYSL_DBGBCR9_EL1()		MRSL(DBGBCR9_EL1)
#define RSYSL_DBGBCR10_EL1()		MRSL(DBGBCR10_EL1)
#define RSYSL_DBGBCR11_EL1()		MRSL(DBGBCR11_EL1)
#define RSYSL_DBGBCR12_EL1()		MRSL(DBGBCR12_EL1)
#define RSYSL_DBGBCR13_EL1()		MRSL(DBGBCR13_EL1)
#define RSYSL_DBGBCR14_EL1()		MRSL(DBGBCR14_EL1)
#define RSYSL_DBGBCR15_EL1()		MRSL(DBGBCR15_EL1)
#define RSYSL_DBGWCR0_EL1()		MRSL(DBGWCR0_EL1)
#define RSYSL_DBGWCR1_EL1()		MRSL(DBGWCR1_EL1)
#define RSYSL_DBGWCR2_EL1()		MRSL(DBGWCR2_EL1)
#define RSYSL_DBGWCR3_EL1()		MRSL(DBGWCR3_EL1)
#define RSYSL_DBGWCR4_EL1()		MRSL(DBGWCR4_EL1)
#define RSYSL_DBGWCR5_EL1()		MRSL(DBGWCR5_EL1)
#define RSYSL_DBGWCR6_EL1()		MRSL(DBGWCR6_EL1)
#define RSYSL_DBGWCR7_EL1()		MRSL(DBGWCR7_EL1)
#define RSYSL_DBGWCR8_EL1()		MRSL(DBGWCR8_EL1)
#define RSYSL_DBGWCR9_EL1()		MRSL(DBGWCR9_EL1)
#define RSYSL_DBGWCR10_EL1()		MRSL(DBGWCR10_EL1)
#define RSYSL_DBGWCR11_EL1()		MRSL(DBGWCR11_EL1)
#define RSYSL_DBGWCR12_EL1()		MRSL(DBGWCR12_EL1)
#define RSYSL_DBGWCR13_EL1()		MRSL(DBGWCR13_EL1)
#define RSYSL_DBGWCR14_EL1()		MRSL(DBGWCR14_EL1)
#define RSYSL_DBGWCR15_EL1()		MRSL(DBGWCR15_EL1)
#define RSYSL_DBGCLAIMSET_EL1()		MRSL(DBGCLAIMSET_EL1)
#define RSYSL_DBGCLAIMCLR_EL1()		MRSL(DBGCLAIMCLR_EL1)
#define RSYSL_DBGAUTHSTATUS_EL1()	MRSL(DBGAUTHSTATUS_EL1)
#define RSYSL_DBGVCR32_EL2()		MRSL(DBGVCR32_EL2)
#define RSYSL_MDCR_EL2()		MRSL(MDCR_EL2)
#define RSYSL_MDCR_EL3()		MRSL(MDCR_EL3)
/* 64 bit registers */
#define RSYSQ_DBGDTR_EL0()		MRSQ(DBGDTR_EL0)
#define RSYSQ_MDRAR_EL1()		MRSQ(MDRAR_EL1)
#define RSYSQ_DBGBVR0_EL1()		MRSQ(DBGBVR0_EL1)
#define RSYSQ_DBGBVR1_EL1()		MRSQ(DBGBVR1_EL1)
#define RSYSQ_DBGBVR2_EL1()		MRSQ(DBGBVR2_EL1)
#define RSYSQ_DBGBVR3_EL1()		MRSQ(DBGBVR3_EL1)
#define RSYSQ_DBGBVR4_EL1()		MRSQ(DBGBVR4_EL1)
#define RSYSQ_DBGBVR5_EL1()		MRSQ(DBGBVR5_EL1)
#define RSYSQ_DBGBVR6_EL1()		MRSQ(DBGBVR6_EL1)
#define RSYSQ_DBGBVR7_EL1()		MRSQ(DBGBVR7_EL1)
#define RSYSQ_DBGBVR8_EL1()		MRSQ(DBGBVR8_EL1)
#define RSYSQ_DBGBVR9_EL1()		MRSQ(DBGBVR9_EL1)
#define RSYSQ_DBGBVR10_EL1()		MRSQ(DBGBVR10_EL1)
#define RSYSQ_DBGBVR11_EL1()		MRSQ(DBGBVR11_EL1)
#define RSYSQ_DBGBVR12_EL1()		MRSQ(DBGBVR12_EL1)
#define RSYSQ_DBGBVR13_EL1()		MRSQ(DBGBVR13_EL1)
#define RSYSQ_DBGBVR14_EL1()		MRSQ(DBGBVR14_EL1)
#define RSYSQ_DBGBVR15_EL1()		MRSQ(DBGBVR15_EL1)
#define RSYSQ_DBGWVR0_EL1()		MRSQ(DBGWVR0_EL1)
#define RSYSQ_DBGWVR1_EL1()		MRSQ(DBGWVR1_EL1)
#define RSYSQ_DBGWVR2_EL1()		MRSQ(DBGWVR2_EL1)
#define RSYSQ_DBGWVR3_EL1()		MRSQ(DBGWVR3_EL1)
#define RSYSQ_DBGWVR4_EL1()		MRSQ(DBGWVR4_EL1)
#define RSYSQ_DBGWVR5_EL1()		MRSQ(DBGWVR5_EL1)
#define RSYSQ_DBGWVR6_EL1()		MRSQ(DBGWVR6_EL1)
#define RSYSQ_DBGWVR7_EL1()		MRSQ(DBGWVR7_EL1)
#define RSYSQ_DBGWVR8_EL1()		MRSQ(DBGWVR8_EL1)
#define RSYSQ_DBGWVR9_EL1()		MRSQ(DBGWVR9_EL1)
#define RSYSQ_DBGWVR10_EL1()		MRSQ(DBGWVR10_EL1)
#define RSYSQ_DBGWVR11_EL1()		MRSQ(DBGWVR11_EL1)
#define RSYSQ_DBGWVR12_EL1()		MRSQ(DBGWVR12_EL1)
#define RSYSQ_DBGWVR13_EL1()		MRSQ(DBGWVR13_EL1)
#define RSYSQ_DBGWVR14_EL1()		MRSQ(DBGWVR14_EL1)
#define RSYSQ_DBGWVR15_EL1()		MRSQ(DBGWVR15_EL1)

/* 32 bit registers */
#define WSYS_DBGDTRTX_EL0(val)		MSR(val, DBGDTRTX_EL0)
#define WSYS_MDCCINT_EL1(val)		MSR(val, MDCCINT_EL1)
#define WSYS_MDSCR_EL1(val)		MSR(val, MDSCR_EL1)
#define WSYS_OSDTRRX_EL1(val)		MSR(val, OSDTRRX_EL1)
#define WSYS_OSDTRTX_EL1(val)		MSR(val, OSDTRTX_EL1)
#define WSYS_OSDLR_EL1(val)		MSR(val, OSDLR_EL1)
#define WSYS_OSECCR_EL1(val)		MSR(val, OSECCR_EL1)
#define WSYS_DBGPRCR_EL1(val)		MSR(val, DBGPRCR_EL1)
#define WSYS_DBGBCR0_EL1(val)		MSR(val, DBGBCR0_EL1)
#define WSYS_DBGBCR1_EL1(val)		MSR(val, DBGBCR1_EL1)
#define WSYS_DBGBCR2_EL1(val)		MSR(val, DBGBCR2_EL1)
#define WSYS_DBGBCR3_EL1(val)		MSR(val, DBGBCR3_EL1)
#define WSYS_DBGBCR4_EL1(val)		MSR(val, DBGBCR4_EL1)
#define WSYS_DBGBCR5_EL1(val)		MSR(val, DBGBCR5_EL1)
#define WSYS_DBGBCR6_EL1(val)		MSR(val, DBGBCR6_EL1)
#define WSYS_DBGBCR7_EL1(val)		MSR(val, DBGBCR7_EL1)
#define WSYS_DBGBCR8_EL1(val)		MSR(val, DBGBCR8_EL1)
#define WSYS_DBGBCR9_EL1(val)		MSR(val, DBGBCR9_EL1)
#define WSYS_DBGBCR10_EL1(val)		MSR(val, DBGBCR10_EL1)
#define WSYS_DBGBCR11_EL1(val)		MSR(val, DBGBCR11_EL1)
#define WSYS_DBGBCR12_EL1(val)		MSR(val, DBGBCR12_EL1)
#define WSYS_DBGBCR13_EL1(val)		MSR(val, DBGBCR13_EL1)
#define WSYS_DBGBCR14_EL1(val)		MSR(val, DBGBCR14_EL1)
#define WSYS_DBGBCR15_EL1(val)		MSR(val, DBGBCR15_EL1)
#define WSYS_DBGWCR0_EL1(val)		MSR(val, DBGWCR0_EL1)
#define WSYS_DBGWCR1_EL1(val)		MSR(val, DBGWCR1_EL1)
#define WSYS_DBGWCR2_EL1(val)		MSR(val, DBGWCR2_EL1)
#define WSYS_DBGWCR3_EL1(val)		MSR(val, DBGWCR3_EL1)
#define WSYS_DBGWCR4_EL1(val)		MSR(val, DBGWCR4_EL1)
#define WSYS_DBGWCR5_EL1(val)		MSR(val, DBGWCR5_EL1)
#define WSYS_DBGWCR6_EL1(val)		MSR(val, DBGWCR6_EL1)
#define WSYS_DBGWCR7_EL1(val)		MSR(val, DBGWCR7_EL1)
#define WSYS_DBGWCR8_EL1(val)		MSR(val, DBGWCR8_EL1)
#define WSYS_DBGWCR9_EL1(val)		MSR(val, DBGWCR9_EL1)
#define WSYS_DBGWCR10_EL1(val)		MSR(val, DBGWCR10_EL1)
#define WSYS_DBGWCR11_EL1(val)		MSR(val, DBGWCR11_EL1)
#define WSYS_DBGWCR12_EL1(val)		MSR(val, DBGWCR12_EL1)
#define WSYS_DBGWCR13_EL1(val)		MSR(val, DBGWCR13_EL1)
#define WSYS_DBGWCR14_EL1(val)		MSR(val, DBGWCR14_EL1)
#define WSYS_DBGWCR15_EL1(val)		MSR(val, DBGWCR15_EL1)
#define WSYS_DBGCLAIMSET_EL1(val)	MSR(val, DBGCLAIMSET_EL1)
#define WSYS_DBGCLAIMCLR_EL1(val)	MSR(val, DBGCLAIMCLR_EL1)
#define WSYS_OSLAR_EL1(val)		MSR(val, OSLAR_EL1)
#define WSYS_DBGVCR32_EL2(val)		MSR(val, DBGVCR32_EL2)
#define WSYS_MDCR_EL2(val)		MSR(val, MDCR_EL2)
#define WSYS_MDCR_EL3(val)		MSR(val, MDCR_EL3)
/* 64 bit registers */
#define WSYS_DBGDTR_EL0(val)		MSR(val, DBGDTR_EL0)
#define WSYS_DBGBVR0_EL1(val)		MSR(val, DBGBVR0_EL1)
#define WSYS_DBGBVR1_EL1(val)		MSR(val, DBGBVR1_EL1)
#define WSYS_DBGBVR2_EL1(val)		MSR(val, DBGBVR2_EL1)
#define WSYS_DBGBVR3_EL1(val)		MSR(val, DBGBVR3_EL1)
#define WSYS_DBGBVR4_EL1(val)		MSR(val, DBGBVR4_EL1)
#define WSYS_DBGBVR5_EL1(val)		MSR(val, DBGBVR5_EL1)
#define WSYS_DBGBVR6_EL1(val)		MSR(val, DBGBVR6_EL1)
#define WSYS_DBGBVR7_EL1(val)		MSR(val, DBGBVR7_EL1)
#define WSYS_DBGBVR8_EL1(val)		MSR(val, DBGBVR8_EL1)
#define WSYS_DBGBVR9_EL1(val)		MSR(val, DBGBVR9_EL1)
#define WSYS_DBGBVR10_EL1(val)		MSR(val, DBGBVR10_EL1)
#define WSYS_DBGBVR11_EL1(val)		MSR(val, DBGBVR11_EL1)
#define WSYS_DBGBVR12_EL1(val)		MSR(val, DBGBVR12_EL1)
#define WSYS_DBGBVR13_EL1(val)		MSR(val, DBGBVR13_EL1)
#define WSYS_DBGBVR14_EL1(val)		MSR(val, DBGBVR14_EL1)
#define WSYS_DBGBVR15_EL1(val)		MSR(val, DBGBVR15_EL1)
#define WSYS_DBGWVR0_EL1(val)		MSR(val, DBGWVR0_EL1)
#define WSYS_DBGWVR1_EL1(val)		MSR(val, DBGWVR1_EL1)
#define WSYS_DBGWVR2_EL1(val)		MSR(val, DBGWVR2_EL1)
#define WSYS_DBGWVR3_EL1(val)		MSR(val, DBGWVR3_EL1)
#define WSYS_DBGWVR4_EL1(val)		MSR(val, DBGWVR4_EL1)
#define WSYS_DBGWVR5_EL1(val)		MSR(val, DBGWVR5_EL1)
#define WSYS_DBGWVR6_EL1(val)		MSR(val, DBGWVR6_EL1)
#define WSYS_DBGWVR7_EL1(val)		MSR(val, DBGWVR7_EL1)
#define WSYS_DBGWVR8_EL1(val)		MSR(val, DBGWVR8_EL1)
#define WSYS_DBGWVR9_EL1(val)		MSR(val, DBGWVR9_EL1)
#define WSYS_DBGWVR10_EL1(val)		MSR(val, DBGWVR10_EL1)
#define WSYS_DBGWVR11_EL1(val)		MSR(val, DBGWVR11_EL1)
#define WSYS_DBGWVR12_EL1(val)		MSR(val, DBGWVR12_EL1)
#define WSYS_DBGWVR13_EL1(val)		MSR(val, DBGWVR13_EL1)
#define WSYS_DBGWVR14_EL1(val)		MSR(val, DBGWVR14_EL1)
#define WSYS_DBGWVR15_EL1(val)		MSR(val, DBGWVR15_EL1)

#endif
