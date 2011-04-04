/*
 * OMAP3/4 Voltage Processor (VP) structure and macro definitions
 *
 * Copyright (C) 2007, 2010 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 * Lesly A M <x0080970@ti.com>
 * Thara Gopinath <thara@ti.com>
 *
 * Copyright (C) 2008, 2011 Nokia Corporation
 * Kalle Jokiniemi
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 */
#ifndef __ARCH_ARM_MACH_OMAP2_VP_H
#define __ARCH_ARM_MACH_OMAP2_VP_H

#include <linux/kernel.h>

struct voltagedomain;

/*
 * Voltage Processor (VP) identifiers
 */
#define OMAP3_VP_VDD_MPU_ID 0
#define OMAP3_VP_VDD_CORE_ID 1
#define OMAP4_VP_VDD_CORE_ID 0
#define OMAP4_VP_VDD_IVA_ID 1
#define OMAP4_VP_VDD_MPU_ID 2

/* XXX document */
#define VP_IDLE_TIMEOUT		200
#define VP_TRANXDONE_TIMEOUT	300

/**
 * struct omap_vp_ops - per-VP operations
 * @check_txdone: check for VP transaction done
 * @clear_txdone: clear VP transaction done status
 */
struct omap_vp_ops {
	u32 (*check_txdone)(u8 vp_id);
	void (*clear_txdone)(u8 vp_id);
};

/**
 * struct omap_vp_common - register data common to all VDDs
 * @vpconfig_errorgain_mask: ERRORGAIN bitmask in the PRM_VP*_CONFIG reg
 * @vpconfig_initvoltage_mask: INITVOLTAGE bitmask in the PRM_VP*_CONFIG reg
 * @vpconfig_timeouten_mask: TIMEOUT bitmask in the PRM_VP*_CONFIG reg
 * @vpconfig_initvdd: INITVDD bitmask in the PRM_VP*_CONFIG reg
 * @vpconfig_forceupdate: FORCEUPDATE bitmask in the PRM_VP*_CONFIG reg
 * @vpconfig_vpenable: VPENABLE bitmask in the PRM_VP*_CONFIG reg
 * @vpconfig_erroroffset_shift: ERROROFFSET field shift in PRM_VP*_CONFIG reg
 * @vpconfig_errorgain_shift: ERRORGAIN field shift in PRM_VP*_CONFIG reg
 * @vpconfig_initvoltage_shift: INITVOLTAGE field shift in PRM_VP*_CONFIG reg
 * @vpconfig_stepmin_shift: VSTEPMIN field shift in the PRM_VP*_VSTEPMIN reg
 * @vpconfig_smpswaittimemin_shift: SMPSWAITTIMEMIN field shift in PRM_VP*_VSTEPMIN reg
 * @vpconfig_stepmax_shift: VSTEPMAX field shift in the PRM_VP*_VSTEPMAX reg
 * @vpconfig_smpswaittimemax_shift: SMPSWAITTIMEMAX field shift in PRM_VP*_VSTEPMAX reg
 * @vpconfig_vlimitto_vddmin_shift: VDDMIN field shift in PRM_VP*_VLIMITTO reg
 * @vpconfig_vlimitto_vddmax_shift: VDDMAX field shift in PRM_VP*_VLIMITTO reg
 * @vpconfig_vlimitto_timeout_shift: TIMEOUT field shift in PRM_VP*_VLIMITTO reg
 *
 * XXX It it not necessary to have both a mask and a shift for the same
 *     bitfield - remove one
 * XXX Many of these fields are wrongly named -- e.g., vpconfig_smps* -- fix!
 */
struct omap_vp_common {
	u32 vpconfig_errorgain_mask;
	u32 vpconfig_initvoltage_mask;
	u32 vpconfig_timeouten;
	u32 vpconfig_initvdd;
	u32 vpconfig_forceupdate;
	u32 vpconfig_vpenable;
	u8 vpconfig_erroroffset_shift;
	u8 vpconfig_errorgain_shift;
	u8 vpconfig_initvoltage_shift;
	u8 vstepmin_stepmin_shift;
	u8 vstepmin_smpswaittimemin_shift;
	u8 vstepmax_stepmax_shift;
	u8 vstepmax_smpswaittimemax_shift;
	u8 vlimitto_vddmin_shift;
	u8 vlimitto_vddmax_shift;
	u8 vlimitto_timeout_shift;

	const struct omap_vp_ops *ops;
};

/**
 * struct omap_vp_instance - VP register offsets (per-VDD)
 * @common: pointer to struct omap_vp_common * for this SoC
 * @vpconfig: PRM_VP*_CONFIG reg offset from PRM start
 * @vstepmin: PRM_VP*_VSTEPMIN reg offset from PRM start
 * @vlimitto: PRM_VP*_VLIMITTO reg offset from PRM start
 * @vstatus: PRM_VP*_VSTATUS reg offset from PRM start
 * @voltage: PRM_VP*_VOLTAGE reg offset from PRM start
 * @id: Unique identifier for VP instance.
 * @enabled: flag to keep track of whether vp is enabled or not
 *
 * XXX vp_common is probably not needed since it is per-SoC
 */
struct omap_vp_instance {
	const struct omap_vp_common *common;
	u8 vpconfig;
	u8 vstepmin;
	u8 vstepmax;
	u8 vlimitto;
	u8 vstatus;
	u8 voltage;
	u8 id;
	bool enabled;
};

/**
 * struct omap_vp_runtime_data - VP data populated at runtime by code
 * @vpconfig_erroroffset: value of ERROROFFSET bitfield in PRM_VP*_CONFIG
 * @vpconfig_errorgain: value of ERRORGAIN bitfield in PRM_VP*_CONFIG
 * @vstepmin_smpswaittimemin: value of SMPSWAITTIMEMIN bitfield in PRM_VP*_VSTEPMIN
 * @vstepmax_smpswaittimemax: value of SMPSWAITTIMEMAX bitfield in PRM_VP*_VSTEPMAX
 * @vlimitto_timeout: value of TIMEOUT bitfield in PRM_VP*_VLIMITTO
 * @vstepmin_stepmin: value of VSTEPMIN bitfield in PRM_VP*_VSTEPMIN
 * @vstepmax_stepmax: value of VSTEPMAX bitfield in PRM_VP*_VSTEPMAX
 * @vlimitto_vddmin: value of VDDMIN bitfield in PRM_VP*_VLIMITTO
 * @vlimitto_vddmax: value of VDDMAX bitfield in PRM_VP*_VLIMITTO
 *
 * XXX Is this structure really needed?  Why not just program the
 * device directly?  They are in PRM space, therefore in the WKUP
 * powerdomain, so register contents should not be lost in off-mode.
 * XXX Some of these fields are incorrectly named, e.g., vstep*
 */
struct omap_vp_runtime_data {
	u32 vpconfig_erroroffset;
	u16 vpconfig_errorgain;
	u16 vstepmin_smpswaittimemin;
	u16 vstepmax_smpswaittimemax;
	u16 vlimitto_timeout;
	u8 vstepmin_stepmin;
	u8 vstepmax_stepmax;
	u8 vlimitto_vddmin;
	u8 vlimitto_vddmax;
};

extern struct omap_vp_instance omap3_vp_mpu;
extern struct omap_vp_instance omap3_vp_core;

extern struct omap_vp_instance omap4_vp_mpu;
extern struct omap_vp_instance omap4_vp_iva;
extern struct omap_vp_instance omap4_vp_core;

void omap_vp_init(struct voltagedomain *voltdm);
void omap_vp_enable(struct voltagedomain *voltdm);
void omap_vp_disable(struct voltagedomain *voltdm);
unsigned long omap_vp_get_curr_volt(struct voltagedomain *voltdm);
int omap_vp_forceupdate_scale(struct voltagedomain *voltdm,
			      unsigned long target_volt);

#endif
