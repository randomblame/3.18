/*
 * linux/arch/arm/mach-omap2/mux.c
 *
 * OMAP2 and OMAP3 pin multiplexing configurations
 *
 * Copyright (C) 2004 - 2008 Texas Instruments Inc.
 * Copyright (C) 2003 - 2008 Nokia Corporation
 *
 * Written by Tony Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#include <asm/system.h>

#include <mach/control.h>
#include <mach/mux.h>

#ifdef CONFIG_OMAP_MUX

static struct omap_mux_cfg arch_mux_cfg;

/* NOTE: See mux.h for the enumeration */

#ifdef CONFIG_ARCH_OMAP24XX
static struct pin_config __initdata_or_module omap24xx_pins[] = {
/*
 *	description			mux	mux	pull	pull	debug
 *					offset	mode	ena	type
 */

/* 24xx I2C */
MUX_CFG_24XX("M19_24XX_I2C1_SCL",	0x111,	0,	0,	0,	1)
MUX_CFG_24XX("L15_24XX_I2C1_SDA",	0x112,	0,	0,	0,	1)
MUX_CFG_24XX("J15_24XX_I2C2_SCL",	0x113,	0,	0,	1,	1)
MUX_CFG_24XX("H19_24XX_I2C2_SDA",	0x114,	0,	0,	0,	1)

/* Menelaus interrupt */
MUX_CFG_24XX("W19_24XX_SYS_NIRQ",	0x12c,	0,	1,	1,	1)

/* 24xx clocks */
MUX_CFG_24XX("W14_24XX_SYS_CLKOUT",	0x137,	0,	1,	1,	1)

/* 24xx GPMC chipselects, wait pin monitoring */
MUX_CFG_24XX("E2_GPMC_NCS2",		0x08e,	0,	1,	1,	1)
MUX_CFG_24XX("L2_GPMC_NCS7",		0x093,	0,	1,	1,	1)
MUX_CFG_24XX("L3_GPMC_WAIT0",		0x09a,	0,	1,	1,	1)
MUX_CFG_24XX("N7_GPMC_WAIT1",		0x09b,	0,	1,	1,	1)
MUX_CFG_24XX("M1_GPMC_WAIT2",		0x09c,	0,	1,	1,	1)
MUX_CFG_24XX("P1_GPMC_WAIT3",		0x09d,	0,	1,	1,	1)

/* 24xx McBSP */
MUX_CFG_24XX("Y15_24XX_MCBSP2_CLKX",	0x124,	1,	1,	0,	1)
MUX_CFG_24XX("R14_24XX_MCBSP2_FSX",	0x125,	1,	1,	0,	1)
MUX_CFG_24XX("W15_24XX_MCBSP2_DR",	0x126,	1,	1,	0,	1)
MUX_CFG_24XX("V15_24XX_MCBSP2_DX",	0x127,	1,	1,	0,	1)

/* 24xx GPIO */
MUX_CFG_24XX("M21_242X_GPIO11",		0x0c9,	3,	1,	1,	1)
MUX_CFG_24XX("P21_242X_GPIO12",		0x0ca,	3,	0,	0,	1)
MUX_CFG_24XX("AA10_242X_GPIO13",	0x0e5,	3,	0,	0,	1)
MUX_CFG_24XX("AA6_242X_GPIO14",		0x0e6,	3,	0,	0,	1)
MUX_CFG_24XX("AA4_242X_GPIO15",		0x0e7,	3,	0,	0,	1)
MUX_CFG_24XX("Y11_242X_GPIO16",		0x0e8,	3,	0,	0,	1)
MUX_CFG_24XX("AA12_242X_GPIO17",	0x0e9,	3,	0,	0,	1)
MUX_CFG_24XX("AA8_242X_GPIO58",		0x0ea,	3,	0,	0,	1)
MUX_CFG_24XX("Y20_24XX_GPIO60",		0x12c,	3,	0,	0,	1)
MUX_CFG_24XX("W4__24XX_GPIO74",		0x0f2,	3,	0,	0,	1)
MUX_CFG_24XX("N15_24XX_GPIO85",		0x103,	3,	0,	0,	1)
MUX_CFG_24XX("M15_24XX_GPIO92",		0x10a,	3,	0,	0,	1)
MUX_CFG_24XX("P20_24XX_GPIO93",		0x10b,	3,	0,	0,	1)
MUX_CFG_24XX("P18_24XX_GPIO95",		0x10d,	3,	0,	0,	1)
MUX_CFG_24XX("M18_24XX_GPIO96",		0x10e,	3,	0,	0,	1)
MUX_CFG_24XX("L14_24XX_GPIO97",		0x10f,	3,	0,	0,	1)
MUX_CFG_24XX("J15_24XX_GPIO99",		0x113,	3,	1,	1,	1)
MUX_CFG_24XX("V14_24XX_GPIO117",	0x128,	3,	1,	0,	1)
MUX_CFG_24XX("P14_24XX_GPIO125",	0x140,	3,	1,	1,	1)

/* 242x DBG GPIO */
MUX_CFG_24XX("V4_242X_GPIO49",		0xd3,	3,	0,	0,	1)
MUX_CFG_24XX("W2_242X_GPIO50",		0xd4,	3,	0,	0,	1)
MUX_CFG_24XX("U4_242X_GPIO51",		0xd5,	3,	0,	0,	1)
MUX_CFG_24XX("V3_242X_GPIO52",		0xd6,	3,	0,	0,	1)
MUX_CFG_24XX("V2_242X_GPIO53",		0xd7,	3,	0,	0,	1)
MUX_CFG_24XX("V6_242X_GPIO53",		0xcf,	3,	0,	0,	1)
MUX_CFG_24XX("T4_242X_GPIO54",		0xd8,	3,	0,	0,	1)
MUX_CFG_24XX("Y4_242X_GPIO54",		0xd0,	3,	0,	0,	1)
MUX_CFG_24XX("T3_242X_GPIO55",		0xd9,	3,	0,	0,	1)
MUX_CFG_24XX("U2_242X_GPIO56",		0xda,	3,	0,	0,	1)

/* 24xx external DMA requests */
MUX_CFG_24XX("AA10_242X_DMAREQ0",	0x0e5,	2,	0,	0,	1)
MUX_CFG_24XX("AA6_242X_DMAREQ1",	0x0e6,	2,	0,	0,	1)
MUX_CFG_24XX("E4_242X_DMAREQ2",		0x074,	2,	0,	0,	1)
MUX_CFG_24XX("G4_242X_DMAREQ3",		0x073,	2,	0,	0,	1)
MUX_CFG_24XX("D3_242X_DMAREQ4",		0x072,	2,	0,	0,	1)
MUX_CFG_24XX("E3_242X_DMAREQ5",		0x071,	2,	0,	0,	1)

/* UART3 */
MUX_CFG_24XX("K15_24XX_UART3_TX",	0x118,	0,	0,	0,	1)
MUX_CFG_24XX("K14_24XX_UART3_RX",	0x119,	0,	0,	0,	1)

/* MMC/SDIO */
MUX_CFG_24XX("G19_24XX_MMC_CLKO",	0x0f3,	0,	0,	0,	1)
MUX_CFG_24XX("H18_24XX_MMC_CMD",	0x0f4,	0,	0,	0,	1)
MUX_CFG_24XX("F20_24XX_MMC_DAT0",	0x0f5,	0,	0,	0,	1)
MUX_CFG_24XX("H14_24XX_MMC_DAT1",	0x0f6,	0,	0,	0,	1)
MUX_CFG_24XX("E19_24XX_MMC_DAT2",	0x0f7,	0,	0,	0,	1)
MUX_CFG_24XX("D19_24XX_MMC_DAT3",	0x0f8,	0,	0,	0,	1)
MUX_CFG_24XX("F19_24XX_MMC_DAT_DIR0",	0x0f9,	0,	0,	0,	1)
MUX_CFG_24XX("E20_24XX_MMC_DAT_DIR1",	0x0fa,	0,	0,	0,	1)
MUX_CFG_24XX("F18_24XX_MMC_DAT_DIR2",	0x0fb,	0,	0,	0,	1)
MUX_CFG_24XX("E18_24XX_MMC_DAT_DIR3",	0x0fc,	0,	0,	0,	1)
MUX_CFG_24XX("G18_24XX_MMC_CMD_DIR",	0x0fd,	0,	0,	0,	1)
MUX_CFG_24XX("H15_24XX_MMC_CLKI",	0x0fe,	0,	0,	0,	1)

/* Full speed USB */
MUX_CFG_24XX("J20_24XX_USB0_PUEN",	0x11d,	0,	0,	0,	1)
MUX_CFG_24XX("J19_24XX_USB0_VP",	0x11e,	0,	0,	0,	1)
MUX_CFG_24XX("K20_24XX_USB0_VM",	0x11f,	0,	0,	0,	1)
MUX_CFG_24XX("J18_24XX_USB0_RCV",	0x120,	0,	0,	0,	1)
MUX_CFG_24XX("K19_24XX_USB0_TXEN",	0x121,	0,	0,	0,	1)
MUX_CFG_24XX("J14_24XX_USB0_SE0",	0x122,	0,	0,	0,	1)
MUX_CFG_24XX("K18_24XX_USB0_DAT",	0x123,	0,	0,	0,	1)

MUX_CFG_24XX("N14_24XX_USB1_SE0",	0x0ed,	2,	0,	0,	1)
MUX_CFG_24XX("W12_24XX_USB1_SE0",	0x0dd,	3,	0,	0,	1)
MUX_CFG_24XX("P15_24XX_USB1_DAT",	0x0ee,	2,	0,	0,	1)
MUX_CFG_24XX("R13_24XX_USB1_DAT",	0x0e0,	3,	0,	0,	1)
MUX_CFG_24XX("W20_24XX_USB1_TXEN",	0x0ec,	2,	0,	0,	1)
MUX_CFG_24XX("P13_24XX_USB1_TXEN",	0x0df,	3,	0,	0,	1)
MUX_CFG_24XX("V19_24XX_USB1_RCV",	0x0eb,	2,	0,	0,	1)
MUX_CFG_24XX("V12_24XX_USB1_RCV",	0x0de,	3,	0,	0,	1)

MUX_CFG_24XX("AA10_24XX_USB2_SE0",	0x0e5,	2,	0,	0,	1)
MUX_CFG_24XX("Y11_24XX_USB2_DAT",	0x0e8,	2,	0,	0,	1)
MUX_CFG_24XX("AA12_24XX_USB2_TXEN",	0x0e9,	2,	0,	0,	1)
MUX_CFG_24XX("AA6_24XX_USB2_RCV",	0x0e6,	2,	0,	0,	1)
MUX_CFG_24XX("AA4_24XX_USB2_TLLSE0",	0x0e7,	2,	0,	0,	1)

/* Keypad GPIO*/
MUX_CFG_24XX("T19_24XX_KBR0",		0x106,	3,	1,	1,	1)
MUX_CFG_24XX("R19_24XX_KBR1",		0x107,	3,	1,	1,	1)
MUX_CFG_24XX("V18_24XX_KBR2",		0x139,	3,	1,	1,	1)
MUX_CFG_24XX("M21_24XX_KBR3",		0xc9,	3,	1,	1,	1)
MUX_CFG_24XX("E5__24XX_KBR4",		0x138,	3,	1,	1,	1)
MUX_CFG_24XX("M18_24XX_KBR5",		0x10e,	3,	1,	1,	1)
MUX_CFG_24XX("R20_24XX_KBC0",		0x108,	3,	0,	0,	1)
MUX_CFG_24XX("M14_24XX_KBC1",		0x109,	3,	0,	0,	1)
MUX_CFG_24XX("H19_24XX_KBC2",		0x114,	3,	0,	0,	1)
MUX_CFG_24XX("V17_24XX_KBC3",		0x135,	3,	0,	0,	1)
MUX_CFG_24XX("P21_24XX_KBC4",		0xca,	3,	0,	0,	1)
MUX_CFG_24XX("L14_24XX_KBC5",		0x10f,	3,	0,	0,	1)
MUX_CFG_24XX("N19_24XX_KBC6",		0x110,	3,	0,	0,	1)

/* 24xx Menelaus Keypad GPIO */
MUX_CFG_24XX("B3__24XX_KBR5",		0x30,	3,	1,	1,	1)
MUX_CFG_24XX("AA4_24XX_KBC2",		0xe7,	3,	0,	0,	1)
MUX_CFG_24XX("B13_24XX_KBC6",		0x110,	3,	0,	0,	1)

/* 2430 USB */
MUX_CFG_24XX("AD9_2430_USB0_PUEN",	0x133,	4,	0,	0,	1)
MUX_CFG_24XX("Y11_2430_USB0_VP",	0x134,	4,	0,	0,	1)
MUX_CFG_24XX("AD7_2430_USB0_VM",	0x135,	4,	0,	0,	1)
MUX_CFG_24XX("AE7_2430_USB0_RCV",	0x136,	4,	0,	0,	1)
MUX_CFG_24XX("AD4_2430_USB0_TXEN",	0x137,	4,	0,	0,	1)
MUX_CFG_24XX("AF9_2430_USB0_SE0",	0x138,	4,	0,	0,	1)
MUX_CFG_24XX("AE6_2430_USB0_DAT",	0x139,	4,	0,	0,	1)
MUX_CFG_24XX("AD24_2430_USB1_SE0",	0x107,	2,	0,	0,	1)
MUX_CFG_24XX("AB24_2430_USB1_RCV",	0x108,	2,	0,	0,	1)
MUX_CFG_24XX("Y25_2430_USB1_TXEN",	0x109,	2,	0,	0,	1)
MUX_CFG_24XX("AA26_2430_USB1_DAT",	0x10A,	2,	0,	0,	1)

/* 2430 HS-USB */
MUX_CFG_24XX("AD9_2430_USB0HS_DATA3",	0x133,	0,	0,	0,	1)
MUX_CFG_24XX("Y11_2430_USB0HS_DATA4",	0x134,	0,	0,	0,	1)
MUX_CFG_24XX("AD7_2430_USB0HS_DATA5",	0x135,	0,	0,	0,	1)
MUX_CFG_24XX("AE7_2430_USB0HS_DATA6",	0x136,	0,	0,	0,	1)
MUX_CFG_24XX("AD4_2430_USB0HS_DATA2",	0x137,	0,	0,	0,	1)
MUX_CFG_24XX("AF9_2430_USB0HS_DATA0",	0x138,	0,	0,	0,	1)
MUX_CFG_24XX("AE6_2430_USB0HS_DATA1",	0x139,	0,	0,	0,	1)
MUX_CFG_24XX("AE8_2430_USB0HS_CLK",	0x13A,	0,	0,	0,	1)
MUX_CFG_24XX("AD8_2430_USB0HS_DIR",	0x13B,	0,	0,	0,	1)
MUX_CFG_24XX("AE5_2430_USB0HS_STP",	0x13c,	0,	1,	1,	1)
MUX_CFG_24XX("AE9_2430_USB0HS_NXT",	0x13D,	0,	0,	0,	1)
MUX_CFG_24XX("AC7_2430_USB0HS_DATA7",	0x13E,	0,	0,	0,	1)

/* 2430 McBSP */
MUX_CFG_24XX("AD6_2430_MCBSP_CLKS",	0x011E,	0,	0,	0,	1)

MUX_CFG_24XX("AB2_2430_MCBSP1_CLKR",	0x011A,	0,	0,	0,	1)
MUX_CFG_24XX("AD5_2430_MCBSP1_FSR",	0x011B,	0,	0,	0,	1)
MUX_CFG_24XX("AA1_2430_MCBSP1_DX",	0x011C,	0,	0,	0,	1)
MUX_CFG_24XX("AF3_2430_MCBSP1_DR",	0x011D,	0,	0,	0,	1)
MUX_CFG_24XX("AB3_2430_MCBSP1_FSX",	0x011F,	0,	0,	0,	1)
MUX_CFG_24XX("Y9_2430_MCBSP1_CLKX",	0x0120,	0,	0,	0,	1)

MUX_CFG_24XX("AC10_2430_MCBSP2_FSX",	0x012E,	1,	0,	0,	1)
MUX_CFG_24XX("AD16_2430_MCBSP2_CLX",	0x012F,	1,	0,	0,	1)
MUX_CFG_24XX("AE13_2430_MCBSP2_DX",	0x0130,	1,	0,	0,	1)
MUX_CFG_24XX("AD13_2430_MCBSP2_DR",	0x0131,	1,	0,	0,	1)
MUX_CFG_24XX("AC10_2430_MCBSP2_FSX_OFF",0x012E,	0,	0,	0,	1)
MUX_CFG_24XX("AD16_2430_MCBSP2_CLX_OFF",0x012F,	0,	0,	0,	1)
MUX_CFG_24XX("AE13_2430_MCBSP2_DX_OFF",	0x0130,	0,	0,	0,	1)
MUX_CFG_24XX("AD13_2430_MCBSP2_DR_OFF",	0x0131,	0,	0,	0,	1)

MUX_CFG_24XX("AC9_2430_MCBSP3_CLKX",	0x0103,	0,	0,	0,	1)
MUX_CFG_24XX("AE4_2430_MCBSP3_FSX",	0x0104,	0,	0,	0,	1)
MUX_CFG_24XX("AE2_2430_MCBSP3_DR",	0x0105,	0,	0,	0,	1)
MUX_CFG_24XX("AF4_2430_MCBSP3_DX",	0x0106,	0,	0,	0,	1)

MUX_CFG_24XX("N3_2430_MCBSP4_CLKX",	0x010B,	1,	0,	0,	1)
MUX_CFG_24XX("AD23_2430_MCBSP4_DR",	0x010C,	1,	0,	0,	1)
MUX_CFG_24XX("AB25_2430_MCBSP4_DX",	0x010D,	1,	0,	0,	1)
MUX_CFG_24XX("AC25_2430_MCBSP4_FSX",	0x010E,	1,	0,	0,	1)

MUX_CFG_24XX("AE16_2430_MCBSP5_CLKX",	0x00ED,	1,	0,	0,	1)
MUX_CFG_24XX("AF12_2430_MCBSP5_FSX",	0x00ED,	1,	0,	0,	1)
MUX_CFG_24XX("K7_2430_MCBSP5_DX",	0x00EF,	1,	0,	0,	1)
MUX_CFG_24XX("M1_2430_MCBSP5_DR",	0x00F0,	1,	0,	0,	1)

/* 2430 MCSPI1 */
MUX_CFG_24XX("Y18_2430_MCSPI1_CLK",	0x010F,	0,	0,	0,	1)
MUX_CFG_24XX("AD15_2430_MCSPI1_SIMO",	0x0110,	0,	0,	0,	1)
MUX_CFG_24XX("AE17_2430_MCSPI1_SOMI",	0x0111,	0,	0,	0,	1)
MUX_CFG_24XX("U1_2430_MCSPI1_CS0",	0x0112,	0,	0,	0,	1)

/* Touchscreen GPIO */
MUX_CFG_24XX("AF19_2430_GPIO_85",	0x0113,	3,	0,	0,	1)

};

#define OMAP24XX_PINS_SZ	ARRAY_SIZE(omap24xx_pins)

#else
#define omap24xx_pins		NULL
#define OMAP24XX_PINS_SZ	0
#endif	/* CONFIG_ARCH_OMAP24XX */

#ifdef CONFIG_ARCH_OMAP34XX
static struct pin_config __initdata_or_module omap34xx_pins[] = {
/*
 *		Name, reg-offset,
 *		mux-mode | [active-mode | off-mode]
 */

/* 34xx I2C */
MUX_CFG_34XX("K21_34XX_I2C1_SCL", 0x1ba,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("J21_34XX_I2C1_SDA", 0x1bc,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AF15_34XX_I2C2_SCL", 0x1be,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AE15_34XX_I2C2_SDA", 0x1c0,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AF14_34XX_I2C3_SCL", 0x1c2,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AG14_34XX_I2C3_SDA", 0x1c4,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AD26_34XX_I2C4_SCL", 0xa00,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AE26_34XX_I2C4_SDA", 0xa02,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)

/* PHY - HSUSB: 12-pin ULPI PHY: Port 1*/
MUX_CFG_34XX("Y8_3430_USB1HS_PHY_CLK", 0x5da,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("Y9_3430_USB1HS_PHY_STP", 0x5d8,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AA14_3430_USB1HS_PHY_DIR", 0x5ec,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AA11_3430_USB1HS_PHY_NXT", 0x5ee,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W13_3430_USB1HS_PHY_D0", 0x5dc,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W12_3430_USB1HS_PHY_D1", 0x5de,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W11_3430_USB1HS_PHY_D2", 0x5e0,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y11_3430_USB1HS_PHY_D3", 0x5ea,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W9_3430_USB1HS_PHY_D4", 0x5e4,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y12_3430_USB1HS_PHY_D5", 0x5e6,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W8_3430_USB1HS_PHY_D6", 0x5e8,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y13_3430_USB1HS_PHY_D7", 0x5e2,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)

/* PHY - HSUSB: 12-pin ULPI PHY: Port 2*/
MUX_CFG_34XX("AA8_3430_USB2HS_PHY_CLK", 0x5f0,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AA10_3430_USB2HS_PHY_STP", 0x5f2,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AA9_3430_USB2HS_PHY_DIR", 0x5f4,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB11_3430_USB2HS_PHY_NXT", 0x5f6,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB10_3430_USB2HS_PHY_D0", 0x5f8,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB9_3430_USB2HS_PHY_D1", 0x5fa,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W3_3430_USB2HS_PHY_D2", 0x1d4,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T4_3430_USB2HS_PHY_D3", 0x1de,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T3_3430_USB2HS_PHY_D4", 0x1d8,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("R3_3430_USB2HS_PHY_D5", 0x1da,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("R4_3430_USB2HS_PHY_D6", 0x1dc,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T2_3430_USB2HS_PHY_D7", 0x1d6,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN)

/* TLL - HSUSB: 12-pin TLL Port 1*/
MUX_CFG_34XX("Y8_3430_USB1HS_TLL_CLK", 0x5da,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y9_3430_USB1HS_TLL_STP", 0x5d8,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AA14_3430_USB1HS_TLL_DIR", 0x5ec,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AA11_3430_USB1HS_TLL_NXT", 0x5ee,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W13_3430_USB1HS_TLL_D0", 0x5dc,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W12_3430_USB1HS_TLL_D1", 0x5de,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W11_3430_USB1HS_TLL_D2", 0x5e0,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y11_3430_USB1HS_TLL_D3", 0x5ea,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W9_3430_USB1HS_TLL_D4", 0x5e4,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y12_3430_USB1HS_TLL_D5", 0x5e6,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W8_3430_USB1HS_TLL_D6", 0x5e8,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y13_3430_USB1HS_TLL_D7", 0x5e2,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)

/* TLL - HSUSB: 12-pin TLL Port 2*/
MUX_CFG_34XX("AA8_3430_USB2HS_TLL_CLK", 0x5f0,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AA10_3430_USB2HS_TLL_STP", 0x5f2,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AA9_3430_USB2HS_TLL_DIR", 0x5f4,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB11_3430_USB2HS_TLL_NXT", 0x5f6,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB10_3430_USB2HS_TLL_D0", 0x5f8,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB9_3430_USB2HS_TLL_D1", 0x5fa,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W3_3430_USB2HS_TLL_D2", 0x1d4,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T4_3430_USB2HS_TLL_D3", 0x1de,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T3_3430_USB2HS_TLL_D4", 0x1d8,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("R3_3430_USB2HS_TLL_D5", 0x1da,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("R4_3430_USB2HS_TLL_D6", 0x1dc,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T2_3430_USB2HS_TLL_D7", 0x1d6,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLDOWN)

/* TLL - HSUSB: 12-pin TLL Port 3*/
MUX_CFG_34XX("AA6_3430_USB3HS_TLL_CLK", 0x180,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB3_3430_USB3HS_TLL_STP", 0x166,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AA3_3430_USB3HS_TLL_DIR", 0x168,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y3_3430_USB3HS_TLL_NXT", 0x16a,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AA5_3430_USB3HS_TLL_D0", 0x186,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y4_3430_USB3HS_TLL_D1", 0x184,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y5_3430_USB3HS_TLL_D2", 0x188,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W5_3430_USB3HS_TLL_D3", 0x18a,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB12_3430_USB3HS_TLL_D4", 0x16c,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB13_3430_USB3HS_TLL_D5", 0x16e,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AA13_3430_USB3HS_TLL_D6", 0x170,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AA12_3430_USB3HS_TLL_D7", 0x172,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)

/* PHY FSUSB: FS Serial for Port 1 (multiple PHY modes supported) */
MUX_CFG_34XX("AF10_3430_USB1FS_PHY_MM1_RXDP", 0x5d8,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AG9_3430_USB1FS_PHY_MM1_RXDM", 0x5ee,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W13_3430_USB1FS_PHY_MM1_RXRCV", 0x5dc,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W12_3430_USB1FS_PHY_MM1_TXSE0", 0x5de,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W11_3430_USB1FS_PHY_MM1_TXDAT", 0x5e0,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("Y11_3430_USB1FS_PHY_MM1_TXEN_N", 0x5ea,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_OUTPUT)

/* PHY FSUSB: FS Serial for Port 2 (multiple PHY modes supported) */
MUX_CFG_34XX("AF7_3430_USB2FS_PHY_MM2_RXDP", 0x5f2,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AH7_3430_USB2FS_PHY_MM2_RXDM", 0x5f6,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB10_3430_USB2FS_PHY_MM2_RXRCV", 0x5f8,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AB9_3430_USB2FS_PHY_MM2_TXSE0", 0x5fa,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("W3_3430_USB2FS_PHY_MM2_TXDAT", 0x1d4,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("T4_3430_USB2FS_PHY_MM2_TXEN_N", 0x1de,
		OMAP34XX_MUX_MODE5 | OMAP34XX_PIN_OUTPUT)

/* PHY FSUSB: FS Serial for Port 3 (multiple PHY modes supported) */
MUX_CFG_34XX("AH3_3430_USB3FS_PHY_MM3_RXDP", 0x166,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AE3_3430_USB3FS_PHY_MM3_RXDM", 0x16a,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AD1_3430_USB3FS_PHY_MM3_RXRCV", 0x186,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AE1_3430_USB3FS_PHY_MM3_TXSE0", 0x184,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AD2_3430_USB3FS_PHY_MM3_TXDAT", 0x188,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("AC1_3430_USB3FS_PHY_MM3_TXEN_N", 0x18a,
		OMAP34XX_MUX_MODE6 | OMAP34XX_PIN_OUTPUT)


/* 34XX GPIO - bidirectional, unless the name has an "_OUT" suffix.
 * (Always specify PIN_INPUT, except for names suffixed by "_OUT".)
 * No internal pullup/pulldown without "_UP" or "_DOWN" suffix.
 */
MUX_CFG_34XX("AF26_34XX_GPIO0", 0x1e0,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("AF22_34XX_GPIO9", 0xa18,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("AG9_34XX_GPIO23", 0x5ee,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("AH8_34XX_GPIO29", 0x5fa,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("U8_34XX_GPIO54_OUT", 0x0b4,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("U8_34XX_GPIO54_DOWN", 0x0b4,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN)
MUX_CFG_34XX("L8_34XX_GPIO63", 0x0ce,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("G25_34XX_GPIO86_OUT", 0x0fc,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AG4_34XX_GPIO134_OUT", 0x160,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AF4_34XX_GPIO135_OUT", 0x162,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AE4_34XX_GPIO136_OUT", 0x164,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("AF6_34XX_GPIO140_UP", 0x16c,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AE6_34XX_GPIO141", 0x16e,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("AF5_34XX_GPIO142", 0x170,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("AE5_34XX_GPIO143", 0x172,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("H19_34XX_GPIO164_OUT", 0x19c,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("J25_34XX_GPIO170", 0x1c6,
		OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT)

/* OMAP3 SDRC CKE signals to SDR/DDR ram chips */
MUX_CFG_34XX("H16_34XX_SDRC_CKE0", 0x262,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("H17_34XX_SDRC_CKE1", 0x264,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT)

/* MMC1 */
MUX_CFG_34XX("N28_3430_MMC1_CLK", 0x144,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("M27_3430_MMC1_CMD", 0x146,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("N27_3430_MMC1_DAT0", 0x148,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("N26_3430_MMC1_DAT1", 0x14a,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("N25_3430_MMC1_DAT2", 0x14c,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("P28_3430_MMC1_DAT3", 0x14e,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("P27_3430_MMC1_DAT4", 0x150,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("P26_3430_MMC1_DAT5", 0x152,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("R27_3430_MMC1_DAT6", 0x154,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("R25_3430_MMC1_DAT7", 0x156,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)

/* MMC2 */
MUX_CFG_34XX("AE2_3430_MMC2_CLK", 0x158,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AG5_3430_MMC2_CMD", 0x15A,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AH5_3430_MMC2_DAT0", 0x15c,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AH4_3430_MMC2_DAT1", 0x15e,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AG4_3430_MMC2_DAT2", 0x160,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AF4_3430_MMC2_DAT3", 0x162,
		OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)

/* MMC3 */
MUX_CFG_34XX("AF10_3430_MMC3_CLK", 0x5d8,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AC3_3430_MMC3_CMD", 0x1d0,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AE11_3430_MMC3_DAT0", 0x5e4,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AH9_3430_MMC3_DAT1", 0x5e6,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AF13_3430_MMC3_DAT2", 0x5e8,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP)
MUX_CFG_34XX("AF13_3430_MMC3_DAT3", 0x5e2,
		OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP)

/* SYS_NIRQ T2 INT1 */
MUX_CFG_34XX("AF26_34XX_SYS_NIRQ", 0x1E0,
		OMAP3_WAKEUP_EN | OMAP34XX_PIN_INPUT_PULLUP |
		OMAP34XX_MUX_MODE0)
};

#define OMAP34XX_PINS_SZ	ARRAY_SIZE(omap34xx_pins)

#else
#define omap34xx_pins		NULL
#define OMAP34XX_PINS_SZ	0
#endif	/* CONFIG_ARCH_OMAP34XX */

#if defined(CONFIG_OMAP_MUX_DEBUG) || defined(CONFIG_OMAP_MUX_WARNINGS)
static void __init_or_module omap2_cfg_debug(const struct pin_config *cfg, u16 reg)
{
	u16 orig;
	u8 warn = 0, debug = 0;

	if (cpu_is_omap24xx())
		orig = omap_ctrl_readb(cfg->mux_reg);
	else
		orig = omap_ctrl_readw(cfg->mux_reg);

#ifdef	CONFIG_OMAP_MUX_DEBUG
	debug = cfg->debug;
#endif
	warn = (orig != reg);
	if (debug || warn)
		printk(KERN_WARNING
			"MUX: setup %s (0x%p): 0x%04x -> 0x%04x\n",
			cfg->name, omap_ctrl_base_get() + cfg->mux_reg,
			orig, reg);
}
#else
#define omap2_cfg_debug(x, y)	do {} while (0)
#endif

#ifdef CONFIG_ARCH_OMAP24XX
static int __init_or_module omap24xx_cfg_reg(const struct pin_config *cfg)
{
	static DEFINE_SPINLOCK(mux_spin_lock);
	unsigned long flags;
	u8 reg = 0;

	spin_lock_irqsave(&mux_spin_lock, flags);
	reg |= cfg->mask & 0x7;
	if (cfg->pull_val)
		reg |= OMAP2_PULL_ENA;
	if (cfg->pu_pd_val)
		reg |= OMAP2_PULL_UP;
	omap2_cfg_debug(cfg, reg);
	omap_ctrl_writeb(reg, cfg->mux_reg);
	spin_unlock_irqrestore(&mux_spin_lock, flags);

	return 0;
}
#else
#define omap24xx_cfg_reg	NULL
#endif

#ifdef CONFIG_ARCH_OMAP34XX
static int __init_or_module omap34xx_cfg_reg(const struct pin_config *cfg)
{
	static DEFINE_SPINLOCK(mux_spin_lock);
	unsigned long flags;
	u16 reg = 0;

	spin_lock_irqsave(&mux_spin_lock, flags);
	reg |= cfg->mux_val;
	omap2_cfg_debug(cfg, reg);
	omap_ctrl_writew(reg, cfg->mux_reg);
	spin_unlock_irqrestore(&mux_spin_lock, flags);

	return 0;
}
#else
#define omap34xx_cfg_reg	NULL
#endif

int __init omap2_mux_init(void)
{
	if (cpu_is_omap24xx()) {
		arch_mux_cfg.pins	= omap24xx_pins;
		arch_mux_cfg.size	= OMAP24XX_PINS_SZ;
		arch_mux_cfg.cfg_reg	= omap24xx_cfg_reg;
	} else if (cpu_is_omap34xx()) {
		arch_mux_cfg.pins	= omap34xx_pins;
		arch_mux_cfg.size	= OMAP34XX_PINS_SZ;
		arch_mux_cfg.cfg_reg	= omap34xx_cfg_reg;
	}

	return omap_mux_register(&arch_mux_cfg);
}

#endif
