/*
 * TI DaVinci DM365 chip specific setup
 *
 * Copyright (C) 2009 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>

#include <asm/mach/map.h>

#include <mach/dm365.h>
#include <mach/clock.h>
#include <mach/cputype.h>
#include <mach/edma.h>
#include <mach/psc.h>
#include <mach/mux.h>
#include <mach/irqs.h>
#include <mach/time.h>
#include <mach/serial.h>
#include <mach/common.h>

#include "clock.h"
#include "mux.h"

#define DM365_REF_FREQ		24000000	/* 24 MHz on the DM365 EVM */

static struct pll_data pll1_data = {
	.num		= 1,
	.phys_base	= DAVINCI_PLL1_BASE,
	.flags		= PLL_HAS_POSTDIV | PLL_HAS_PREDIV,
};

static struct pll_data pll2_data = {
	.num		= 2,
	.phys_base	= DAVINCI_PLL2_BASE,
	.flags		= PLL_HAS_POSTDIV | PLL_HAS_PREDIV,
};

static struct clk ref_clk = {
	.name		= "ref_clk",
	.rate		= DM365_REF_FREQ,
};

static struct clk pll1_clk = {
	.name		= "pll1",
	.parent		= &ref_clk,
	.flags		= CLK_PLL,
	.pll_data	= &pll1_data,
};

static struct clk pll1_aux_clk = {
	.name		= "pll1_aux_clk",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk pll1_sysclkbp = {
	.name		= "pll1_sysclkbp",
	.parent		= &pll1_clk,
	.flags 		= CLK_PLL | PRE_PLL,
	.div_reg	= BPDIV
};

static struct clk clkout0_clk = {
	.name		= "clkout0",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk pll1_sysclk1 = {
	.name		= "pll1_sysclk1",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV1,
};

static struct clk pll1_sysclk2 = {
	.name		= "pll1_sysclk2",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV2,
};

static struct clk pll1_sysclk3 = {
	.name		= "pll1_sysclk3",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV3,
};

static struct clk pll1_sysclk4 = {
	.name		= "pll1_sysclk4",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV4,
};

static struct clk pll1_sysclk5 = {
	.name		= "pll1_sysclk5",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV5,
};

static struct clk pll1_sysclk6 = {
	.name		= "pll1_sysclk6",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV6,
};

static struct clk pll1_sysclk7 = {
	.name		= "pll1_sysclk7",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV7,
};

static struct clk pll1_sysclk8 = {
	.name		= "pll1_sysclk8",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV8,
};

static struct clk pll1_sysclk9 = {
	.name		= "pll1_sysclk9",
	.parent		= &pll1_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV9,
};

static struct clk pll2_clk = {
	.name		= "pll2",
	.parent		= &ref_clk,
	.flags		= CLK_PLL,
	.pll_data	= &pll2_data,
};

static struct clk pll2_aux_clk = {
	.name		= "pll2_aux_clk",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk clkout1_clk = {
	.name		= "clkout1",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL | PRE_PLL,
};

static struct clk pll2_sysclk1 = {
	.name		= "pll2_sysclk1",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV1,
};

static struct clk pll2_sysclk2 = {
	.name		= "pll2_sysclk2",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV2,
};

static struct clk pll2_sysclk3 = {
	.name		= "pll2_sysclk3",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV3,
};

static struct clk pll2_sysclk4 = {
	.name		= "pll2_sysclk4",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV4,
};

static struct clk pll2_sysclk5 = {
	.name		= "pll2_sysclk5",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV5,
};

static struct clk pll2_sysclk6 = {
	.name		= "pll2_sysclk6",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV6,
};

static struct clk pll2_sysclk7 = {
	.name		= "pll2_sysclk7",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV7,
};

static struct clk pll2_sysclk8 = {
	.name		= "pll2_sysclk8",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV8,
};

static struct clk pll2_sysclk9 = {
	.name		= "pll2_sysclk9",
	.parent		= &pll2_clk,
	.flags		= CLK_PLL,
	.div_reg	= PLLDIV9,
};

static struct clk vpss_dac_clk = {
	.name		= "vpss_dac",
	.parent		= &pll1_sysclk3,
	.lpsc		= DM365_LPSC_DAC_CLK,
};

static struct clk vpss_master_clk = {
	.name		= "vpss_master",
	.parent		= &pll1_sysclk5,
	.lpsc		= DM365_LPSC_VPSSMSTR,
	.flags		= CLK_PSC,
};

static struct clk arm_clk = {
	.name		= "arm_clk",
	.parent		= &pll2_sysclk2,
	.lpsc		= DAVINCI_LPSC_ARM,
	.flags		= ALWAYS_ENABLED,
};

static struct clk uart0_clk = {
	.name		= "uart0",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_UART0,
};

static struct clk uart1_clk = {
	.name		= "uart1",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_UART1,
};

static struct clk i2c_clk = {
	.name		= "i2c",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_I2C,
};

static struct clk mmcsd0_clk = {
	.name		= "mmcsd0",
	.parent		= &pll1_sysclk8,
	.lpsc		= DAVINCI_LPSC_MMC_SD,
};

static struct clk mmcsd1_clk = {
	.name		= "mmcsd1",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_MMC_SD1,
};

static struct clk spi0_clk = {
	.name		= "spi0",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_SPI,
};

static struct clk spi1_clk = {
	.name		= "spi1",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_SPI1,
};

static struct clk spi2_clk = {
	.name		= "spi2",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_SPI2,
};

static struct clk spi3_clk = {
	.name		= "spi3",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_SPI3,
};

static struct clk spi4_clk = {
	.name		= "spi4",
	.parent		= &pll1_aux_clk,
	.lpsc		= DM365_LPSC_SPI4,
};

static struct clk gpio_clk = {
	.name		= "gpio",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_GPIO,
};

static struct clk aemif_clk = {
	.name		= "aemif",
	.parent		= &pll1_sysclk4,
	.lpsc		= DAVINCI_LPSC_AEMIF,
};

static struct clk pwm0_clk = {
	.name		= "pwm0",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_PWM0,
};

static struct clk pwm1_clk = {
	.name		= "pwm1",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_PWM1,
};

static struct clk pwm2_clk = {
	.name		= "pwm2",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_PWM2,
};

static struct clk pwm3_clk = {
	.name		= "pwm3",
	.parent		= &ref_clk,
	.lpsc		= DM365_LPSC_PWM3,
};

static struct clk timer0_clk = {
	.name		= "timer0",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_TIMER0,
};

static struct clk timer1_clk = {
	.name		= "timer1",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_TIMER1,
};

static struct clk timer2_clk = {
	.name		= "timer2",
	.parent		= &pll1_aux_clk,
	.lpsc		= DAVINCI_LPSC_TIMER2,
	.usecount	= 1,
};

static struct clk timer3_clk = {
	.name		= "timer3",
	.parent		= &pll1_aux_clk,
	.lpsc		= DM365_LPSC_TIMER3,
};

static struct clk usb_clk = {
	.name		= "usb",
	.parent		= &pll2_sysclk1,
	.lpsc		= DAVINCI_LPSC_USB,
};

static struct clk emac_clk = {
	.name		= "emac",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_EMAC,
};

static struct clk voicecodec_clk = {
	.name		= "voice_codec",
	.parent		= &pll2_sysclk4,
	.lpsc		= DM365_LPSC_VOICE_CODEC,
};

static struct clk asp0_clk = {
	.name		= "asp0",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_McBSP1,
};

static struct clk rto_clk = {
	.name		= "rto",
	.parent		= &pll1_sysclk4,
	.lpsc		= DM365_LPSC_RTO,
};

static struct clk mjcp_clk = {
	.name		= "mjcp",
	.parent		= &pll1_sysclk3,
	.lpsc		= DM365_LPSC_MJCP,
};

static struct davinci_clk dm365_clks[] = {
	CLK(NULL, "ref", &ref_clk),
	CLK(NULL, "pll1", &pll1_clk),
	CLK(NULL, "pll1_aux", &pll1_aux_clk),
	CLK(NULL, "pll1_sysclkbp", &pll1_sysclkbp),
	CLK(NULL, "clkout0", &clkout0_clk),
	CLK(NULL, "pll1_sysclk1", &pll1_sysclk1),
	CLK(NULL, "pll1_sysclk2", &pll1_sysclk2),
	CLK(NULL, "pll1_sysclk3", &pll1_sysclk3),
	CLK(NULL, "pll1_sysclk4", &pll1_sysclk4),
	CLK(NULL, "pll1_sysclk5", &pll1_sysclk5),
	CLK(NULL, "pll1_sysclk6", &pll1_sysclk6),
	CLK(NULL, "pll1_sysclk7", &pll1_sysclk7),
	CLK(NULL, "pll1_sysclk8", &pll1_sysclk8),
	CLK(NULL, "pll1_sysclk9", &pll1_sysclk9),
	CLK(NULL, "pll2", &pll2_clk),
	CLK(NULL, "pll2_aux", &pll2_aux_clk),
	CLK(NULL, "clkout1", &clkout1_clk),
	CLK(NULL, "pll2_sysclk1", &pll2_sysclk1),
	CLK(NULL, "pll2_sysclk2", &pll2_sysclk2),
	CLK(NULL, "pll2_sysclk3", &pll2_sysclk3),
	CLK(NULL, "pll2_sysclk4", &pll2_sysclk4),
	CLK(NULL, "pll2_sysclk5", &pll2_sysclk5),
	CLK(NULL, "pll2_sysclk6", &pll2_sysclk6),
	CLK(NULL, "pll2_sysclk7", &pll2_sysclk7),
	CLK(NULL, "pll2_sysclk8", &pll2_sysclk8),
	CLK(NULL, "pll2_sysclk9", &pll2_sysclk9),
	CLK(NULL, "vpss_dac", &vpss_dac_clk),
	CLK(NULL, "vpss_master", &vpss_master_clk),
	CLK(NULL, "arm", &arm_clk),
	CLK(NULL, "uart0", &uart0_clk),
	CLK(NULL, "uart1", &uart1_clk),
	CLK("i2c_davinci.1", NULL, &i2c_clk),
	CLK("davinci_mmc.0", NULL, &mmcsd0_clk),
	CLK("davinci_mmc.1", NULL, &mmcsd1_clk),
	CLK("spi_davinci.0", NULL, &spi0_clk),
	CLK("spi_davinci.1", NULL, &spi1_clk),
	CLK("spi_davinci.2", NULL, &spi2_clk),
	CLK("spi_davinci.3", NULL, &spi3_clk),
	CLK("spi_davinci.4", NULL, &spi4_clk),
	CLK(NULL, "gpio", &gpio_clk),
	CLK(NULL, "aemif", &aemif_clk),
	CLK(NULL, "pwm0", &pwm0_clk),
	CLK(NULL, "pwm1", &pwm1_clk),
	CLK(NULL, "pwm2", &pwm2_clk),
	CLK(NULL, "pwm3", &pwm3_clk),
	CLK(NULL, "timer0", &timer0_clk),
	CLK(NULL, "timer1", &timer1_clk),
	CLK("watchdog", NULL, &timer2_clk),
	CLK(NULL, "timer3", &timer3_clk),
	CLK(NULL, "usb", &usb_clk),
	CLK("davinci_emac.1", NULL, &emac_clk),
	CLK("voice_codec", NULL, &voicecodec_clk),
	CLK("soc-audio.0", NULL, &asp0_clk),
	CLK(NULL, "rto", &rto_clk),
	CLK(NULL, "mjcp", &mjcp_clk),
	CLK(NULL, NULL, NULL),
};

/*----------------------------------------------------------------------*/

#define PINMUX0		0x00
#define PINMUX1		0x04
#define PINMUX2		0x08
#define PINMUX3		0x0c
#define PINMUX4		0x10
#define INTMUX		0x18
#define EVTMUX		0x1c


static const struct mux_config dm365_pins[] = {
#ifdef CONFIG_DAVINCI_MUX
MUX_CFG(DM365,	MMCSD0,		0,   24,     1,	  0,	 false)

MUX_CFG(DM365,	SD1_CLK,	0,   16,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_CMD,	4,   30,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA3,	4,   28,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA2,	4,   26,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA1,	4,   24,    3,	  1,	 false)
MUX_CFG(DM365,	SD1_DATA0,	4,   22,    3,	  1,	 false)

MUX_CFG(DM365,	I2C_SDA,	3,   23,    3,	  2,	 false)
MUX_CFG(DM365,	I2C_SCL,	3,   21,    3,	  2,	 false)

MUX_CFG(DM365,	AEMIF_AR,	2,   0,     3,	  1,	 false)
MUX_CFG(DM365,	AEMIF_A3,	2,   2,     3,	  1,	 false)
MUX_CFG(DM365,	AEMIF_A7,	2,   4,     3,	  1,	 false)
MUX_CFG(DM365,	AEMIF_D15_8,	2,   6,     1,	  1,	 false)
MUX_CFG(DM365,	AEMIF_CE0,	2,   7,     1,	  0,	 false)

MUX_CFG(DM365,	MCBSP0_BDX,	0,   23,    1,	  1,	 false)
MUX_CFG(DM365,	MCBSP0_X,	0,   22,    1,	  1,	 false)
MUX_CFG(DM365,	MCBSP0_BFSX,	0,   21,    1,	  1,	 false)
MUX_CFG(DM365,	MCBSP0_BDR,	0,   20,    1,	  1,	 false)
MUX_CFG(DM365,	MCBSP0_R,	0,   19,    1,	  1,	 false)
MUX_CFG(DM365,	MCBSP0_BFSR,	0,   18,    1,	  1,	 false)

MUX_CFG(DM365,	SPI0_SCLK,	3,   28,    1,    1,	 false)
MUX_CFG(DM365,	SPI0_SDI,	3,   26,    3,    1,	 false)
MUX_CFG(DM365,	SPI0_SDO,	3,   25,    1,    1,	 false)
MUX_CFG(DM365,	SPI0_SDENA0,	3,   29,    3,    1,	 false)
MUX_CFG(DM365,	SPI0_SDENA1,	3,   26,    3,    2,	 false)

MUX_CFG(DM365,	UART0_RXD,	3,   20,    1,    1,	 false)
MUX_CFG(DM365,	UART0_TXD,	3,   19,    1,    1,	 false)
MUX_CFG(DM365,	UART1_RXD,	3,   17,    3,    2,	 false)
MUX_CFG(DM365,	UART1_TXD,	3,   15,    3,    2,	 false)
MUX_CFG(DM365,	UART1_RTS,	3,   23,    3,    1,	 false)
MUX_CFG(DM365,	UART1_CTS,	3,   21,    3,    1,	 false)

MUX_CFG(DM365,  EMAC_TX_EN,	3,   17,    3,    1,     false)
MUX_CFG(DM365,  EMAC_TX_CLK,	3,   15,    3,    1,     false)
MUX_CFG(DM365,  EMAC_COL,	3,   14,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD3,	3,   13,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD2,	3,   12,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD1,	3,   11,    1,    1,     false)
MUX_CFG(DM365,  EMAC_TXD0,	3,   10,    1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD3,	3,   9,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD2,	3,   8,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD1,	3,   7,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RXD0,	3,   6,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RX_CLK,	3,   5,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RX_DV,	3,   4,     1,    1,     false)
MUX_CFG(DM365,  EMAC_RX_ER,	3,   3,     1,    1,     false)
MUX_CFG(DM365,  EMAC_CRS,	3,   2,     1,    1,     false)
MUX_CFG(DM365,  EMAC_MDIO,	3,   1,     1,    1,     false)
MUX_CFG(DM365,  EMAC_MDCLK,	3,   0,     1,    1,     false)
#endif
};


static u8 dm365_default_priorities[DAVINCI_N_AINTC_IRQ] = {
	[IRQ_VDINT0]			= 2,
	[IRQ_VDINT1]			= 6,
	[IRQ_VDINT2]			= 6,
	[IRQ_HISTINT]			= 6,
	[IRQ_H3AINT]			= 6,
	[IRQ_PRVUINT]			= 6,
	[IRQ_RSZINT]			= 6,
	[IRQ_DM365_INSFINT]		= 7,
	[IRQ_VENCINT]			= 6,
	[IRQ_ASQINT]			= 6,
	[IRQ_IMXINT]			= 6,
	[IRQ_DM365_IMCOPINT]		= 4,
	[IRQ_USBINT]			= 4,
	[IRQ_DM365_RTOINT]		= 7,
	[IRQ_DM365_TINT5]		= 7,
	[IRQ_DM365_TINT6]		= 5,
	[IRQ_CCINT0]			= 5,
	[IRQ_CCERRINT]			= 5,
	[IRQ_TCERRINT0]			= 5,
	[IRQ_TCERRINT]			= 7,
	[IRQ_PSCIN]			= 4,
	[IRQ_DM365_SPINT2_1]		= 7,
	[IRQ_DM365_TINT7]		= 7,
	[IRQ_DM365_SDIOINT0]		= 7,
	[IRQ_MBXINT]			= 7,
	[IRQ_MBRINT]			= 7,
	[IRQ_MMCINT]			= 7,
	[IRQ_DM365_MMCINT1]		= 7,
	[IRQ_DM365_PWMINT3]		= 7,
	[IRQ_DDRINT]			= 4,
	[IRQ_AEMIFINT]			= 2,
	[IRQ_DM365_SDIOINT1]		= 2,
	[IRQ_TINT0_TINT12]		= 7,
	[IRQ_TINT0_TINT34]		= 7,
	[IRQ_TINT1_TINT12]		= 7,
	[IRQ_TINT1_TINT34]		= 7,
	[IRQ_PWMINT0]			= 7,
	[IRQ_PWMINT1]			= 3,
	[IRQ_PWMINT2]			= 3,
	[IRQ_I2C]			= 3,
	[IRQ_UARTINT0]			= 3,
	[IRQ_UARTINT1]			= 3,
	[IRQ_DM365_SPIINT0_0]		= 3,
	[IRQ_DM365_SPIINT3_0]		= 3,
	[IRQ_DM365_GPIO0]		= 3,
	[IRQ_DM365_GPIO1]		= 7,
	[IRQ_DM365_GPIO2]		= 4,
	[IRQ_DM365_GPIO3]		= 4,
	[IRQ_DM365_GPIO4]		= 7,
	[IRQ_DM365_GPIO5]		= 7,
	[IRQ_DM365_GPIO6]		= 7,
	[IRQ_DM365_GPIO7]		= 7,
	[IRQ_DM365_EMAC_RXTHRESH]	= 7,
	[IRQ_DM365_EMAC_RXPULSE]	= 7,
	[IRQ_DM365_EMAC_TXPULSE]	= 7,
	[IRQ_DM365_EMAC_MISCPULSE]	= 7,
	[IRQ_DM365_GPIO12]		= 7,
	[IRQ_DM365_GPIO13]		= 7,
	[IRQ_DM365_GPIO14]		= 7,
	[IRQ_DM365_GPIO15]		= 7,
	[IRQ_DM365_KEYINT]		= 7,
	[IRQ_DM365_TCERRINT2]		= 7,
	[IRQ_DM365_TCERRINT3]		= 7,
	[IRQ_DM365_EMUINT]		= 7,
};

static struct map_desc dm365_io_desc[] = {
	{
		.virtual	= IO_VIRT,
		.pfn		= __phys_to_pfn(IO_PHYS),
		.length		= IO_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= SRAM_VIRT,
		.pfn		= __phys_to_pfn(0x00010000),
		.length		= SZ_32K,
		/* MT_MEMORY_NONCACHED requires supersection alignment */
		.type		= MT_DEVICE,
	},
};

/* Contents of JTAG ID register used to identify exact cpu type */
static struct davinci_id dm365_ids[] = {
	{
		.variant	= 0x0,
		.part_no	= 0xb83e,
		.manufacturer	= 0x017,
		.cpu_id		= DAVINCI_CPU_ID_DM365,
		.name		= "dm365",
	},
};

static void __iomem *dm365_psc_bases[] = {
	IO_ADDRESS(DAVINCI_PWR_SLEEP_CNTRL_BASE),
};

struct davinci_timer_info dm365_timer_info = {
	.timers		= davinci_timer_instance,
	.clockevent_id	= T0_BOT,
	.clocksource_id	= T0_TOP,
};

static struct plat_serial8250_port dm365_serial_platform_data[] = {
	{
		.mapbase	= DAVINCI_UART0_BASE,
		.irq		= IRQ_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
				  UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{
		.mapbase	= DAVINCI_UART1_BASE,
		.irq		= IRQ_UARTINT1,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
				  UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{
		.flags		= 0
	},
};

static struct platform_device dm365_serial_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= dm365_serial_platform_data,
	},
};

static struct davinci_soc_info davinci_soc_info_dm365 = {
	.io_desc		= dm365_io_desc,
	.io_desc_num		= ARRAY_SIZE(dm365_io_desc),
	.jtag_id_base		= IO_ADDRESS(0x01c40028),
	.ids			= dm365_ids,
	.ids_num		= ARRAY_SIZE(dm365_ids),
	.cpu_clks		= dm365_clks,
	.psc_bases		= dm365_psc_bases,
	.psc_bases_num		= ARRAY_SIZE(dm365_psc_bases),
	.pinmux_base		= IO_ADDRESS(DAVINCI_SYSTEM_MODULE_BASE),
	.pinmux_pins		= dm365_pins,
	.pinmux_pins_num	= ARRAY_SIZE(dm365_pins),
	.intc_base		= IO_ADDRESS(DAVINCI_ARM_INTC_BASE),
	.intc_type		= DAVINCI_INTC_TYPE_AINTC,
	.intc_irq_prios		= dm365_default_priorities,
	.intc_irq_num		= DAVINCI_N_AINTC_IRQ,
	.timer_info		= &dm365_timer_info,
	.gpio_base		= IO_ADDRESS(DAVINCI_GPIO_BASE),
	.gpio_num		= 104,
	.gpio_irq		= 44,
	.serial_dev		= &dm365_serial_device,
	.sram_dma		= 0x00010000,
	.sram_len		= SZ_32K,
};

void __init dm365_init(void)
{
	davinci_common_init(&davinci_soc_info_dm365);
}
