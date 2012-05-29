/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MDSS_MDP_HWIO_H
#define MDSS_MDP_HWIO_H

#include <linux/bitops.h>

#define MDSS_REG_HW_VERSION				0x0
#define MDSS_REG_HW_INTR_STATUS				0x10

#define MDSS_INTR_MDP				BIT(0)
#define MDSS_INTR_DSI0				BIT(4)
#define MDSS_INTR_DSI1				BIT(5)
#define MDSS_INTR_HDMI				BIT(8)
#define MDSS_INTR_EDP				BIT(12)

#define MDSS_MDP_REG_HW_VERSION				0x00100
#define MDSS_MDP_REG_DISP_INTF_SEL			0x00104
#define MDSS_MDP_REG_INTR_EN				0x00110
#define MDSS_MDP_REG_INTR_STATUS			0x00114
#define MDSS_MDP_REG_INTR_CLEAR				0x00118
#define MDSS_MDP_REG_HIST_INTR_EN			0x0011C
#define MDSS_MDP_REG_HIST_INTR_STATUS			0x00120
#define MDSS_MDP_REG_HIST_INTR_CLEAR			0x00124

#define MDSS_INTF_DSI	0x1
#define MDSS_INTF_HDMI	0x3
#define MDSS_INTF_LCDC	0x5
#define MDSS_INTF_EDP	0x9

#define MDSS_MDP_INTR_WB_0_DONE				BIT(0)
#define MDSS_MDP_INTR_WB_1_DONE				BIT(1)
#define MDSS_MDP_INTR_WB_2_DONE				BIT(4)
#define MDSS_MDP_INTR_PING_PONG_0_DONE			BIT(8)
#define MDSS_MDP_INTR_PING_PONG_1_DONE			BIT(9)
#define MDSS_MDP_INTR_PING_PONG_2_DONE			BIT(10)
#define MDSS_MDP_INTR_PING_PONG_0_RD_PTR		BIT(12)
#define MDSS_MDP_INTR_PING_PONG_1_RD_PTR		BIT(13)
#define MDSS_MDP_INTR_PING_PONG_2_RD_PTR		BIT(14)
#define MDSS_MDP_INTR_PING_PONG_0_WR_PTR		BIT(16)
#define MDSS_MDP_INTR_PING_PONG_1_WR_PTR		BIT(17)
#define MDSS_MDP_INTR_PING_PONG_2_WR_PTR		BIT(18)
#define MDSS_MDP_INTR_PING_PONG_0_AUTOREFRESH_DONE	BIT(20)
#define MDSS_MDP_INTR_PING_PONG_1_AUTOREFRESH_DONE	BIT(21)
#define MDSS_MDP_INTR_PING_PONG_2_AUTOREFRESH_DONE	BIT(22)
#define MDSS_MDP_INTR_INTF_0_UNDERRUN			BIT(24)
#define MDSS_MDP_INTR_INTF_0_VSYNC			BIT(25)
#define MDSS_MDP_INTR_INTF_1_UNDERRUN			BIT(26)
#define MDSS_MDP_INTR_INTF_1_VSYNC			BIT(27)
#define MDSS_MDP_INTR_INTF_2_UNDERRUN			BIT(28)
#define MDSS_MDP_INTR_INTF_2_VSYNC			BIT(29)
#define MDSS_MDP_INTR_INTF_3_UNDERRUN			BIT(30)
#define MDSS_MDP_INTR_INTF_3_VSYNC			BIT(31)

enum mdss_mdp_intr_type {
	MDSS_MDP_IRQ_WB_ROT_COMP = 0,
	MDSS_MDP_IRQ_WB_WFD = 4,
	MDSS_MDP_IRQ_PING_PONG_COMP = 8,
	MDSS_MDP_IRQ_PING_PONG_RD_PTR = 12,
	MDSS_MDP_IRQ_PING_PONG_WR_PTR = 16,
	MDSS_MDP_IRQ_PING_PONG_AUTO_REF = 20,
	MDSS_MDP_IRQ_INTF_UNDER_RUN = 24,
	MDSS_MDP_IRQ_INTF_VSYNC = 25,
};

enum mdss_mdp_ctl_index {
	MDSS_MDP_CTL0,
	MDSS_MDP_CTL1,
	MDSS_MDP_CTL2,
	MDSS_MDP_CTL3,
	MDSS_MDP_CTL4,
	MDSS_MDP_MAX_CTL
};

#define MDSS_MDP_REG_CTL_OFFSET(ctl) (0x00600 + ((ctl) * 0x100))

#define MDSS_MDP_REG_CTL_LAYER(lm)			((lm) * 0x004)
#define MDSS_MDP_REG_CTL_TOP				0x014
#define MDSS_MDP_REG_CTL_FLUSH				0x018
#define MDSS_MDP_REG_CTL_START				0x01C
#define MDSS_MDP_REG_CTL_PACK_3D			0x020

#define MDSS_MDP_CTL_OP_VIDEO_MODE		(0 << 17)
#define MDSS_MDP_CTL_OP_CMD_MODE		(1 << 17)

#define MDSS_MDP_CTL_OP_ROT0_MODE		0x1
#define MDSS_MDP_CTL_OP_ROT1_MODE		0x2
#define MDSS_MDP_CTL_OP_WB0_MODE		0x3
#define MDSS_MDP_CTL_OP_WB1_MODE		0x4
#define MDSS_MDP_CTL_OP_WFD_MODE		0x5

#define MDSS_MDP_CTL_OP_PACK_3D_ENABLE		BIT(19)
#define MDSS_MDP_CTL_OP_PACK_3D_FRAME_INT	(0 << 20)
#define MDSS_MDP_CTL_OP_PACK_3D_H_ROW_INT	(1 << 20)
#define MDSS_MDP_CTL_OP_PACK_3D_V_ROW_INT	(2 << 20)
#define MDSS_MDP_CTL_OP_PACK_3D_COL_INT		(3 << 20)

enum mdss_mdp_sspp_index {
	MDSS_MDP_SSPP_VIG0,
	MDSS_MDP_SSPP_VIG1,
	MDSS_MDP_SSPP_VIG2,
	MDSS_MDP_SSPP_RGB0,
	MDSS_MDP_SSPP_RGB1,
	MDSS_MDP_SSPP_RGB2,
	MDSS_MDP_SSPP_DMA0,
	MDSS_MDP_SSPP_DMA1,
	MDSS_MDP_MAX_SSPP
};

enum mdss_mdp_sspp_fetch_type {
	MDSS_MDP_PLANE_INTERLEAVED,
	MDSS_MDP_PLANE_PLANAR,
	MDSS_MDP_PLANE_PSEUDO_PLANAR,
};

enum mdss_mdp_sspp_chroma_samp_type {
	MDSS_MDP_CHROMA_RGB,
	MDSS_MDP_CHROMA_H2V1,
	MDSS_MDP_CHROMA_H1V2,
	MDSS_MDP_CHROMA_420
};

#define MDSS_MDP_REG_SSPP_OFFSET(pipe) (0x01200 + ((pipe) * 0x400))

#define MDSS_MDP_REG_SSPP_SRC_SIZE			0x000
#define MDSS_MDP_REG_SSPP_SRC_IMG_SIZE			0x004
#define MDSS_MDP_REG_SSPP_SRC_XY			0x008
#define MDSS_MDP_REG_SSPP_OUT_SIZE			0x00C
#define MDSS_MDP_REG_SSPP_OUT_XY			0x010
#define MDSS_MDP_REG_SSPP_SRC0_ADDR			0x014
#define MDSS_MDP_REG_SSPP_SRC1_ADDR			0x018
#define MDSS_MDP_REG_SSPP_SRC2_ADDR			0x01C
#define MDSS_MDP_REG_SSPP_SRC3_ADDR			0x020
#define MDSS_MDP_REG_SSPP_SRC_YSTRIDE0			0x024
#define MDSS_MDP_REG_SSPP_SRC_YSTRIDE1			0x028
#define MDSS_MDP_REG_SSPP_STILE_FRAME_SIZE		0x02C
#define MDSS_MDP_REG_SSPP_SRC_FORMAT			0x030
#define MDSS_MDP_REG_SSPP_SRC_UNPACK_PATTERN		0x034

#define MDSS_MDP_REG_SSPP_SRC_OP_MODE			0x038
#define MDSS_MDP_OP_DEINTERLACE			BIT(22)
#define MDSS_MDP_OP_DEINTERLACE_ODD		BIT(23)
#define MDSS_MDP_OP_FLIP_UD			BIT(14)
#define MDSS_MDP_OP_FLIP_LR			BIT(13)
#define MDSS_MDP_OP_BWC_EN			BIT(0)
#define MDSS_MDP_OP_BWC_LOSSLESS		(0 << 1)
#define MDSS_MDP_OP_BWC_Q_HIGH			(1 << 1)
#define MDSS_MDP_OP_BWC_Q_MED			(2 << 1)

#define MDSS_MDP_REG_SSPP_SRC_CONSTANT_COLOR		0x03C
#define MDSS_MDP_REG_SSPP_FETCH_CONFIG			0x048
#define MDSS_MDP_REG_SSPP_VC1_RANGE			0x04C

#define MDSS_MDP_REG_SSPP_CURRENT_SRC0_ADDR		0x0A4
#define MDSS_MDP_REG_SSPP_CURRENT_SRC1_ADDR		0x0A8
#define MDSS_MDP_REG_SSPP_CURRENT_SRC2_ADDR		0x0AC
#define MDSS_MDP_REG_SSPP_CURRENT_SRC3_ADDR		0x0B0
#define MDSS_MDP_REG_SSPP_LINE_SKIP_STEP_C03		0x0B4
#define MDSS_MDP_REG_SSPP_LINE_SKIP_STEP_C12		0x0B8

#define MDSS_MDP_REG_VIG_OP_MODE			0x200
#define MDSS_MDP_REG_VIG_QSEED2_CONFIG			0x204
#define MDSS_MDP_REG_VIG_QSEED2_C03_PHASESTEPX		0x210
#define MDSS_MDP_REG_VIG_QSEED2_C03_PHASESTEPY		0x214
#define MDSS_MDP_REG_VIG_QSEED2_C12_PHASESTEPX		0x218
#define MDSS_MDP_REG_VIG_QSEED2_C12_PHASESTEPY		0x21C
#define MDSS_MDP_REG_VIG_QSEED2_C03_INIT_PHASEX		0x220
#define MDSS_MDP_REG_VIG_QSEED2_C03_INIT_PHASEY		0x224
#define MDSS_MDP_REG_VIG_QSEED2_C12_INIT_PHASEX		0x228
#define MDSS_MDP_REG_VIG_QSEED2_C12_INIT_PHASEY		0x22C

#define MDSS_MDP_REG_SCALE_CONFIG			0x204
#define MDSS_MDP_REG_SCALE_PHASE_STEP_X			0x210
#define MDSS_MDP_REG_SCALE_PHASE_STEP_Y			0x214
#define MDSS_MDP_REG_SCALE_INIT_PHASE_X			0x220
#define MDSS_MDP_REG_SCALE_INIT_PHASE_Y			0x224

#define MDSS_MDP_REG_VIG_CSC_0_BASE			0x280
#define MDSS_MDP_REG_VIG_CSC_1_BASE			0x320

#define MDSS_MDP_SCALE_FILTER_NEAREST		0x0
#define MDSS_MDP_SCALE_FILTER_BIL		0x1
#define MDSS_MDP_SCALE_FILTER_PCMN		0x2
#define MDSS_MDP_SCALE_FILTER_CA		0x3
#define MDSS_MDP_SCALEY_EN			BIT(1)
#define MDSS_MDP_SCALEX_EN			BIT(0)

#define MDSS_MDP_NUM_REG_MIXERS 3
#define MDSS_MDP_NUM_WB_MIXERS 2

enum mdss_mdp_mixer_index {
	MDSS_MDP_LAYERMIXER0,
	MDSS_MDP_LAYERMIXER1,
	MDSS_MDP_LAYERMIXER2,
	MDSS_MDP_LAYERMIXER3,
	MDSS_MDP_LAYERMIXER4,
	MDSS_MDP_MAX_LAYERMIXER
};

enum mdss_mdp_stage_index {
	MDSS_MDP_STAGE_UNUSED,
	MDSS_MDP_STAGE_BASE,
	MDSS_MDP_STAGE_0,
	MDSS_MDP_STAGE_1,
	MDSS_MDP_STAGE_2,
	MDSS_MDP_STAGE_3,
	MDSS_MDP_MAX_STAGE
};

enum mdss_mdp_blend_index {
	MDSS_MDP_BLEND_STAGE0,
	MDSS_MDP_BLEND_STAGE1,
	MDSS_MDP_BLEND_STAGE2,
	MDSS_MDP_BLEND_STAGE3,
	MDSS_MDP_MAX_BLEND_STAGE,
};

#define MDSS_MDP_REG_LM_OFFSET(lm) (0x03200 + ((lm) * 0x400))

#define MDSS_MDP_REG_LM_OP_MODE				0x000
#define MDSS_MDP_REG_LM_OUT_SIZE			0x004
#define MDSS_MDP_REG_LM_BORDER_COLOR_0			0x008
#define MDSS_MDP_REG_LM_BORDER_COLOR_1			0x010

#define MDSS_MDP_REG_LM_BLEND_OFFSET(stage)	(0x20 + ((stage) * 0x30))
#define MDSS_MDP_REG_LM_BLEND_OP			0x00
#define MDSS_MDP_REG_LM_BLEND_FG_ALPHA			0x04
#define MDSS_MDP_REG_LM_BLEND_BG_ALPHA			0x08
#define MDSS_MDP_REG_LM_BLEND_FG_TRANSP_LOW0		0x0C
#define MDSS_MDP_REG_LM_BLEND_FG_TRANSP_LOW1		0x10
#define MDSS_MDP_REG_LM_BLEND_FG_TRANSP_HIGH0		0x14
#define MDSS_MDP_REG_LM_BLEND_FG_TRANSP_HIGH1		0x18
#define MDSS_MDP_REG_LM_BLEND_BG_TRANSP_LOW0		0x1C
#define MDSS_MDP_REG_LM_BLEND_BG_TRANSP_LOW1		0x20
#define MDSS_MDP_REG_LM_BLEND_BG_TRANSP_HIGH0		0x24
#define MDSS_MDP_REG_LM_BLEND_BG_TRANSP_HIGH1		0x28

#define MDSS_MDP_REG_LM_CURSOR_IMG_SIZE			0xE0
#define MDSS_MDP_REG_LM_CURSOR_SIZE			0xE4
#define MDSS_MDP_REG_LM_CURSOR_XY			0xE8
#define MDSS_MDP_REG_LM_CURSOR_STRIDE			0xDC
#define MDSS_MDP_REG_LM_CURSOR_FORMAT			0xEC
#define MDSS_MDP_REG_LM_CURSOR_BASE_ADDR		0xF0
#define MDSS_MDP_REG_LM_CURSOR_START_XY			0xF4
#define MDSS_MDP_REG_LM_CURSOR_BLEND_CONFIG		0xF8
#define MDSS_MDP_REG_LM_CURSOR_BLEND_PARAM		0xFC
#define MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_LOW0	0x100
#define MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_LOW1	0x104
#define MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_HIGH0	0x108
#define MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_HIGH1	0x10C

#define MDSS_MDP_LM_BORDER_COLOR		(1 << 24)
#define MDSS_MDP_LM_CURSOR_OUT			(1 << 25)
#define MDSS_MDP_BLEND_FG_ALPHA_FG_CONST	(0 << 0)
#define MDSS_MDP_BLEND_FG_ALPHA_BG_CONST	(1 << 0)
#define MDSS_MDP_BLEND_FG_ALPHA_FG_PIXEL	(2 << 0)
#define MDSS_MDP_BLEND_FG_ALPHA_BG_PIXEL	(3 << 0)
#define MDSS_MDP_BLEND_FG_INV_ALPHA		(1 << 2)
#define MDSS_MDP_BLEND_FG_MOD_ALPHA		(1 << 3)
#define MDSS_MDP_BLEND_FG_INV_MOD_ALPHA		(1 << 4)
#define MDSS_MDP_BLEND_FG_TRANSP_EN		(1 << 5)
#define MDSS_MDP_BLEND_BG_ALPHA_FG_CONST	(0 << 8)
#define MDSS_MDP_BLEND_BG_ALPHA_BG_CONST	(1 << 8)
#define MDSS_MDP_BLEND_BG_ALPHA_FG_PIXEL	(2 << 8)
#define MDSS_MDP_BLEND_BG_ALPHA_BG_PIXEL	(3 << 8)
#define MDSS_MDP_BLEND_BG_INV_ALPHA		(1 << 10)
#define MDSS_MDP_BLEND_BG_MOD_ALPHA		(1 << 11)
#define MDSS_MDP_BLEND_BG_INV_MOD_ALPHA		(1 << 12)
#define MDSS_MDP_BLEND_BG_TRANSP_EN		(1 << 13)

enum mdss_mdp_writeback_index {
	MDSS_MDP_WRITEBACK0,
	MDSS_MDP_WRITEBACK1,
	MDSS_MDP_WRITEBACK2,
	MDSS_MDP_WRITEBACK3,
	MDSS_MDP_WRITEBACK4,
	MDSS_MDP_MAX_WRITEBACK
};

#define MDSS_MDP_REG_WB_OFFSET(wb)	(0x11100 + ((wb) * 0x2000))

#define MDSS_MDP_REG_WB_DST_FORMAT			0x000
#define MDSS_MDP_REG_WB_DST_OP_MODE			0x004
#define MDSS_MDP_REG_WB_DST_PACK_PATTERN		0x008
#define MDSS_MDP_REG_WB_DST0_ADDR			0x00C
#define MDSS_MDP_REG_WB_DST1_ADDR			0x010
#define MDSS_MDP_REG_WB_DST2_ADDR			0x014
#define MDSS_MDP_REG_WB_DST3_ADDR			0x018
#define MDSS_MDP_REG_WB_DST_YSTRIDE0			0x01C
#define MDSS_MDP_REG_WB_DST_YSTRIDE1			0x020
#define MDSS_MDP_REG_WB_DST_YSTRIDE1			0x020
#define MDSS_MDP_REG_WB_DST_DITHER_BITDEPTH		0x024
#define MDSS_MDP_REG_WB_DST_MATRIX_ROW0			0x030
#define MDSS_MDP_REG_WB_DST_MATRIX_ROW1			0x034
#define MDSS_MDP_REG_WB_DST_MATRIX_ROW2			0x038
#define MDSS_MDP_REG_WB_DST_MATRIX_ROW3			0x03C
#define MDSS_MDP_REG_WB_ROTATION_DNSCALER		0x050
#define MDSS_MDP_REG_WB_N16_INIT_PHASE_X_C03		0x060
#define MDSS_MDP_REG_WB_N16_INIT_PHASE_X_C12		0x064
#define MDSS_MDP_REG_WB_N16_INIT_PHASE_Y_C03		0x068
#define MDSS_MDP_REG_WB_N16_INIT_PHASE_Y_C12		0x06C
#define MDSS_MDP_REG_WB_OUT_SIZE			0x074
#define MDSS_MDP_REG_WB_ALPHA_X_VALUE			0x078
#define MDSS_MDP_REG_WB_CSC_BASE			0x260

enum mdss_mdp_dspp_index {
	MDSS_MDP_DSPP0,
	MDSS_MDP_DSPP1,
	MDSS_MDP_DSPP2,
	MDSS_MDP_MAX_DSPP
};

enum mdss_mpd_intf_index {
	MDSS_MDP_NO_INTF,
	MDSS_MDP_INTF0,
	MDSS_MDP_INTF1,
	MDSS_MDP_INTF2,
	MDSS_MDP_INTF3,
	MDSS_MDP_MAX_INTF
};

#define MDSS_MDP_REG_INTF_OFFSET(intf)		(0x20F00 + ((intf) * 0x200))

#define MDSS_MDP_REG_INTF_TIMING_ENGINE_EN		0x000
#define MDSS_MDP_REG_INTF_CONFIG			0x004
#define MDSS_MDP_REG_INTF_HSYNC_CTL			0x008
#define MDSS_MDP_REG_INTF_VSYNC_PERIOD_F0		0x00C
#define MDSS_MDP_REG_INTF_VSYNC_PERIOD_F1		0x010
#define MDSS_MDP_REG_INTF_VSYNC_PULSE_WIDTH_F0		0x014
#define MDSS_MDP_REG_INTF_VSYNC_PULSE_WIDTH_F1		0x018
#define MDSS_MDP_REG_INTF_DISPLAY_V_START_F0		0x01C
#define MDSS_MDP_REG_INTF_DISPLAY_V_START_F1		0x020
#define MDSS_MDP_REG_INTF_DISPLAY_V_END_F0		0x024
#define MDSS_MDP_REG_INTF_DISPLAY_V_END_F1		0x028
#define MDSS_MDP_REG_INTF_ACTIVE_V_START_F0		0x02C
#define MDSS_MDP_REG_INTF_ACTIVE_V_START_F1		0x030
#define MDSS_MDP_REG_INTF_ACTIVE_V_END_F0		0x034
#define MDSS_MDP_REG_INTF_ACTIVE_V_END_F1		0x038
#define MDSS_MDP_REG_INTF_DISPLAY_HCTL			0x03C
#define MDSS_MDP_REG_INTF_ACTIVE_HCTL			0x040
#define MDSS_MDP_REG_INTF_BORDER_COLOR			0x044
#define MDSS_MDP_REG_INTF_UNDERFLOW_COLOR		0x048
#define MDSS_MDP_REG_INTF_HSYNC_SKEW			0x04C
#define MDSS_MDP_REG_INTF_POLARITY_CTL			0x050
#define MDSS_MDP_REG_INTF_TEST_CTL			0x054
#define MDSS_MDP_REG_INTF_TP_COLOR0			0x058
#define MDSS_MDP_REG_INTF_TP_COLOR1			0x05C

#define MDSS_MDP_REG_INTF_DEFLICKER_CONFIG		0x0F0
#define MDSS_MDP_REG_INTF_DEFLICKER_STRNG_COEFF		0x0F4
#define MDSS_MDP_REG_INTF_DEFLICKER_WEAK_COEFF		0x0F8

#define MDSS_MDP_REG_INTF_DSI_CMD_MODE_TRIGGER_EN	0x084
#define MDSS_MDP_REG_INTF_PANEL_FORMAT			0x090
#define MDSS_MDP_REG_INTF_TPG_ENABLE			0x100
#define MDSS_MDP_REG_INTF_TPG_MAIN_CONTROL		0x104
#define MDSS_MDP_REG_INTF_TPG_VIDEO_CONFIG		0x108
#define MDSS_MDP_REG_INTF_TPG_COMPONENT_LIMITS		0x10C
#define MDSS_MDP_REG_INTF_TPG_RECTANGLE			0x110
#define MDSS_MDP_REG_INTF_TPG_INITIAL_VALUE		0x114
#define MDSS_MDP_REG_INTF_TPG_BLK_WHITE_PATTERN_FRAMES	0x118
#define MDSS_MDP_REG_INTF_TPG_RGB_MAPPING		0x11C

#define MDSS_MDP_REG_INTF_FRAME_LINE_COUNT_EN		0x0A8
#define MDSS_MDP_REG_INTF_FRAME_COUNT			0x0AC
#define MDSS_MDP_REG_INTF_LINE_COUNT			0x0B0

enum mdss_mdp_pingpong_index {
	MDSS_MDP_PINGPONG0,
	MDSS_MDP_PINGPONG1,
	MDSS_MDP_PINGPONG2,
	MDSS_MDP_MAX_PINGPONG
};

#define MDSS_MDP_REG_PP_OFFSET(pp)	(0x21B00 + ((pp) * 0x100))

#define MDSS_MDP_REG_PP_TEAR_CHECK_EN			0x000
#define MDSS_MDP_REG_PP_SYNC_CONFIG_VSYNC		0x004
#define MDSS_MDP_REG_PP_SYNC_CONFIG_HEIGHT		0x008
#define MDSS_MDP_REG_PP_SYNC_WRCOUNT			0x00C
#define MDSS_MDP_REG_PP_VSYNC_INIT_VAL			0x010
#define MDSS_MDP_REG_PP_INT_COUNT_VAL			0x014
#define MDSS_MDP_REG_PP_SYNC_THRESH			0x018
#define MDSS_MDP_REG_PP_START_POS			0x01C
#define MDSS_MDP_REG_PP_RD_PTR_IRQ			0x020
#define MDSS_MDP_REG_PP_WR_PTR_IRQ			0x024
#define MDSS_MDP_REG_PP_OUT_LINE_COUNT			0x028
#define MDSS_MDP_REG_PP_LINE_COUNT			0x02C
#define MDSS_MDP_REG_PP_AUTOREFRESH_CONFIG		0x030

#define MDSS_MDP_REG_SMP_ALLOC_W0			0x00180
#define MDSS_MDP_REG_SMP_ALLOC_R0			0x00230

#define MDSS_MDP_SMP_MMB_SIZE		4096
#define MDSS_MDP_SMP_MMB_BLOCKS		22

enum mdss_mdp_smp_client_index {
	MDSS_MDP_SMP_CLIENT_UNUSED,
	MDSS_MDP_SMP_CLIENT_VIG0_FETCH_Y,
	MDSS_MDP_SMP_CLIENT_VIG0_FETCH_CR,
	MDSS_MDP_SMP_CLIENT_VIG0_FETCH_CB,
	MDSS_MDP_SMP_CLIENT_VIG1_FETCH_Y,
	MDSS_MDP_SMP_CLIENT_VIG1_FETCH_CR,
	MDSS_MDP_SMP_CLIENT_VIG1_FETCH_CB,
	MDSS_MDP_SMP_CLIENT_VIG2_FETCH_Y,
	MDSS_MDP_SMP_CLIENT_VIG2_FETCH_CR,
	MDSS_MDP_SMP_CLIENT_VIG2_FETCH_CB,
	MDSS_MDP_SMP_CLIENT_DMA0_FETCH_Y,
	MDSS_MDP_SMP_CLIENT_DMA0_FETCH_CR,
	MDSS_MDP_SMP_CLIENT_DMA0_FETCH_CB,
	MDSS_MDP_SMP_CLIENT_DMA1_FETCH_Y,
	MDSS_MDP_SMP_CLIENT_DMA1_FETCH_CR,
	MDSS_MDP_SMP_CLIENT_DMA1_FETCH_CB,
	MDSS_MDP_SMP_CLIENT_RGB0_FETCH,
	MDSS_MDP_SMP_CLIENT_RGB1_FETCH,
	MDSS_MDP_SMP_CLIENT_RGB2_FETCH,
};

#endif /* MDSS_MDP_HWIO_H */
