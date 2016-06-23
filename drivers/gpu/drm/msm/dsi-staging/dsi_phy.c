/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt)	"msm-dsi-phy:[%s] " fmt, __func__

#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/msm-bus.h>
#include <linux/list.h>

#include "msm_drv.h"
#include "msm_kms.h"
#include "msm_gpu.h"
#include "dsi_phy.h"
#include "dsi_phy_hw.h"
#include "dsi_clk_pwr.h"
#include "dsi_catalog.h"

#define DSI_PHY_DEFAULT_LABEL "MDSS PHY CTRL"

struct dsi_phy_list_item {
	struct msm_dsi_phy *phy;
	struct list_head list;
};

static LIST_HEAD(dsi_phy_list);
static DEFINE_MUTEX(dsi_phy_list_lock);

static const struct dsi_ver_spec_info dsi_phy_v1_0 = {
	.version = DSI_PHY_VERSION_1_0,
	.lane_cfg_count = 4,
	.strength_cfg_count = 2,
	.regulator_cfg_count = 1,
	.timing_cfg_count = 8,
};
static const struct dsi_ver_spec_info dsi_phy_v2_0 = {
	.version = DSI_PHY_VERSION_2_0,
	.lane_cfg_count = 4,
	.strength_cfg_count = 2,
	.regulator_cfg_count = 1,
	.timing_cfg_count = 8,
};
static const struct dsi_ver_spec_info dsi_phy_v3_0 = {
	.version = DSI_PHY_VERSION_3_0,
	.lane_cfg_count = 4,
	.strength_cfg_count = 2,
	.regulator_cfg_count = 1,
	.timing_cfg_count = 8,
};
static const struct dsi_ver_spec_info dsi_phy_v4_0 = {
	.version = DSI_PHY_VERSION_4_0,
	.lane_cfg_count = 4,
	.strength_cfg_count = 2,
	.regulator_cfg_count = 1,
	.timing_cfg_count = 8,
};

static const struct of_device_id msm_dsi_phy_of_match[] = {
	{ .compatible = "qcom,dsi-phy-v1.0",
	  .data = &dsi_phy_v1_0,},
	{ .compatible = "qcom,dsi-phy-v2.0",
	  .data = &dsi_phy_v2_0,},
	{ .compatible = "qcom,dsi-phy-v3.0",
	  .data = &dsi_phy_v3_0,},
	{ .compatible = "qcom,dsi-phy-v4.0",
	  .data = &dsi_phy_v4_0,},
	{}
};

static int dsi_phy_regmap_init(struct platform_device *pdev,
			       struct msm_dsi_phy *phy)
{
	int rc = 0;
	void __iomem *ptr;

	ptr = msm_ioremap(pdev, "dsi_phy", phy->name);
	if (IS_ERR(ptr)) {
		rc = PTR_ERR(ptr);
		return rc;
	}

	phy->hw.base = ptr;

	pr_debug("[%s] map dsi_phy registers to %p\n", phy->name, phy->hw.base);

	return rc;
}

static int dsi_phy_regmap_deinit(struct msm_dsi_phy *phy)
{
	pr_debug("[%s] unmap registers\n", phy->name);
	return 0;
}

static int dsi_phy_clocks_deinit(struct msm_dsi_phy *phy)
{
	int rc = 0;
	struct dsi_core_clk_info *core = &phy->clks.core_clks;

	if (core->mdp_core_clk)
		devm_clk_put(&phy->pdev->dev, core->mdp_core_clk);
	if (core->iface_clk)
		devm_clk_put(&phy->pdev->dev, core->iface_clk);
	if (core->core_mmss_clk)
		devm_clk_put(&phy->pdev->dev, core->core_mmss_clk);
	if (core->bus_clk)
		devm_clk_put(&phy->pdev->dev, core->bus_clk);

	memset(core, 0x0, sizeof(*core));

	return rc;
}

static int dsi_phy_clocks_init(struct platform_device *pdev,
			       struct msm_dsi_phy *phy)
{
	int rc = 0;
	struct dsi_core_clk_info *core = &phy->clks.core_clks;

	core->mdp_core_clk = devm_clk_get(&pdev->dev, "mdp_core_clk");
	if (IS_ERR(core->mdp_core_clk)) {
		rc = PTR_ERR(core->mdp_core_clk);
		pr_err("failed to get mdp_core_clk, rc=%d\n", rc);
		goto fail;
	}

	core->iface_clk = devm_clk_get(&pdev->dev, "iface_clk");
	if (IS_ERR(core->iface_clk)) {
		rc = PTR_ERR(core->iface_clk);
		pr_err("failed to get iface_clk, rc=%d\n", rc);
		goto fail;
	}

	core->core_mmss_clk = devm_clk_get(&pdev->dev, "core_mmss_clk");
	if (IS_ERR(core->core_mmss_clk)) {
		rc = PTR_ERR(core->core_mmss_clk);
		pr_err("failed to get core_mmss_clk, rc=%d\n", rc);
		goto fail;
	}

	core->bus_clk = devm_clk_get(&pdev->dev, "bus_clk");
	if (IS_ERR(core->bus_clk)) {
		rc = PTR_ERR(core->bus_clk);
		pr_err("failed to get bus_clk, rc=%d\n", rc);
		goto fail;
	}

	return rc;
fail:
	dsi_phy_clocks_deinit(phy);
	return rc;
}

static int dsi_phy_supplies_init(struct platform_device *pdev,
				 struct msm_dsi_phy *phy)
{
	int rc = 0;
	int i = 0;
	struct dsi_regulator_info *regs;
	struct regulator *vreg = NULL;

	regs = &phy->pwr_info.digital;
	regs->vregs = devm_kzalloc(&pdev->dev, sizeof(struct dsi_vreg),
				   GFP_KERNEL);
	if (!regs->vregs)
		goto error;

	regs->count = 1;
	snprintf(regs->vregs->vreg_name,
		 ARRAY_SIZE(regs->vregs[i].vreg_name),
		 "%s", "gdsc");

	rc = dsi_clk_pwr_get_dt_vreg_data(&pdev->dev,
					  &phy->pwr_info.phy_pwr,
					  "qcom,phy-supply-entries");
	if (rc) {
		pr_err("failed to get host power supplies, rc = %d\n", rc);
		goto error_digital;
	}

	regs = &phy->pwr_info.digital;
	for (i = 0; i < regs->count; i++) {
		vreg = devm_regulator_get(&pdev->dev, regs->vregs[i].vreg_name);
		rc = PTR_RET(vreg);
		if (rc) {
			pr_err("failed to get %s regulator\n",
			       regs->vregs[i].vreg_name);
			goto error_host_pwr;
		}
		regs->vregs[i].vreg = vreg;
	}

	regs = &phy->pwr_info.phy_pwr;
	for (i = 0; i < regs->count; i++) {
		vreg = devm_regulator_get(&pdev->dev, regs->vregs[i].vreg_name);
		rc = PTR_RET(vreg);
		if (rc) {
			pr_err("failed to get %s regulator\n",
			       regs->vregs[i].vreg_name);
			for (--i; i >= 0; i--)
				devm_regulator_put(regs->vregs[i].vreg);
			goto error_digital_put;
		}
		regs->vregs[i].vreg = vreg;
	}

	return rc;

error_digital_put:
	regs = &phy->pwr_info.digital;
	for (i = 0; i < regs->count; i++)
		devm_regulator_put(regs->vregs[i].vreg);
error_host_pwr:
	devm_kfree(&pdev->dev, phy->pwr_info.phy_pwr.vregs);
	phy->pwr_info.phy_pwr.vregs = NULL;
	phy->pwr_info.phy_pwr.count = 0;
error_digital:
	devm_kfree(&pdev->dev, phy->pwr_info.digital.vregs);
	phy->pwr_info.digital.vregs = NULL;
	phy->pwr_info.digital.count = 0;
error:
	return rc;
}

static int dsi_phy_supplies_deinit(struct msm_dsi_phy *phy)
{
	int i = 0;
	int rc = 0;
	struct dsi_regulator_info *regs;

	regs = &phy->pwr_info.digital;
	for (i = 0; i < regs->count; i++) {
		if (!regs->vregs[i].vreg)
			pr_err("vreg is NULL, should not reach here\n");
		else
			devm_regulator_put(regs->vregs[i].vreg);
	}

	regs = &phy->pwr_info.phy_pwr;
	for (i = 0; i < regs->count; i++) {
		if (!regs->vregs[i].vreg)
			pr_err("vreg is NULL, should not reach here\n");
		else
			devm_regulator_put(regs->vregs[i].vreg);
	}

	if (phy->pwr_info.phy_pwr.vregs) {
		devm_kfree(&phy->pdev->dev, phy->pwr_info.phy_pwr.vregs);
		phy->pwr_info.phy_pwr.vregs = NULL;
		phy->pwr_info.phy_pwr.count = 0;
	}
	if (phy->pwr_info.digital.vregs) {
		devm_kfree(&phy->pdev->dev, phy->pwr_info.digital.vregs);
		phy->pwr_info.digital.vregs = NULL;
		phy->pwr_info.digital.count = 0;
	}

	return rc;
}

static int dsi_phy_parse_dt_per_lane_cfgs(struct platform_device *pdev,
					  struct dsi_phy_per_lane_cfgs *cfg,
					  char *property)
{
	int rc = 0, i = 0, j = 0;
	const u8 *data;
	u32 len = 0;

	data = of_get_property(pdev->dev.of_node, property, &len);
	if (!data) {
		pr_err("Unable to read Phy %s settings\n", property);
		return -EINVAL;
	}

	if (len != DSI_LANE_MAX * cfg->count_per_lane) {
		pr_err("incorrect phy %s settings, exp=%d, act=%d\n",
		       property, (DSI_LANE_MAX * cfg->count_per_lane), len);
		return -EINVAL;
	}

	for (i = DSI_LOGICAL_LANE_0; i < DSI_LANE_MAX; i++) {
		for (j = 0; j < cfg->count_per_lane; j++) {
			cfg->lane[i][j] = *data;
			data++;
		}
	}

	return rc;
}

static int dsi_phy_settings_init(struct platform_device *pdev,
				 struct msm_dsi_phy *phy)
{
	int rc = 0;
	struct dsi_phy_per_lane_cfgs *lane = &phy->cfg.lanecfg;
	struct dsi_phy_per_lane_cfgs *strength = &phy->cfg.strength;
	struct dsi_phy_per_lane_cfgs *timing = &phy->cfg.timing;
	struct dsi_phy_per_lane_cfgs *regs = &phy->cfg.regulators;

	lane->count_per_lane = phy->ver_info->lane_cfg_count;
	rc = dsi_phy_parse_dt_per_lane_cfgs(pdev, lane,
					    "qcom,platform-lane-config");
	if (rc) {
		pr_err("failed to parse lane cfgs, rc=%d\n", rc);
		goto err;
	}

	strength->count_per_lane = phy->ver_info->strength_cfg_count;
	rc = dsi_phy_parse_dt_per_lane_cfgs(pdev, strength,
					    "qcom,platform-strength-ctrl");
	if (rc) {
		pr_err("failed to parse lane cfgs, rc=%d\n", rc);
		goto err;
	}

	regs->count_per_lane = phy->ver_info->regulator_cfg_count;
	rc = dsi_phy_parse_dt_per_lane_cfgs(pdev, regs,
					    "qcom,platform-regulator-settings");
	if (rc) {
		pr_err("failed to parse lane cfgs, rc=%d\n", rc);
		goto err;
	}

	/* Actual timing values are dependent on panel */
	timing->count_per_lane = phy->ver_info->timing_cfg_count;

err:
	lane->count_per_lane = 0;
	strength->count_per_lane = 0;
	regs->count_per_lane = 0;
	timing->count_per_lane = 0;
	return rc;
}

static int dsi_phy_settings_deinit(struct msm_dsi_phy *phy)
{
	memset(&phy->cfg.lanecfg, 0x0, sizeof(phy->cfg.lanecfg));
	memset(&phy->cfg.strength, 0x0, sizeof(phy->cfg.strength));
	memset(&phy->cfg.timing, 0x0, sizeof(phy->cfg.timing));
	memset(&phy->cfg.regulators, 0x0, sizeof(phy->cfg.regulators));
	return 0;
}

static int dsi_phy_driver_probe(struct platform_device *pdev)
{
	struct msm_dsi_phy *dsi_phy;
	struct dsi_phy_list_item *item;
	const struct of_device_id *id;
	const struct dsi_ver_spec_info *ver_info;
	int rc = 0;
	u32 index = 0;

	if (!pdev || !pdev->dev.of_node) {
		pr_err("pdev not found\n");
		return -ENODEV;
	}

	id = of_match_node(msm_dsi_phy_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	ver_info = id->data;

	item = devm_kzalloc(&pdev->dev, sizeof(*item), GFP_KERNEL);
	if (!item)
		return -ENOMEM;


	dsi_phy = devm_kzalloc(&pdev->dev, sizeof(*dsi_phy), GFP_KERNEL);
	if (!dsi_phy) {
		devm_kfree(&pdev->dev, item);
		return -ENOMEM;
	}

	rc = of_property_read_u32(pdev->dev.of_node, "cell-index", &index);
	if (rc) {
		pr_debug("cell index not set, default to 0\n");
		index = 0;
	}

	dsi_phy->index = index;

	dsi_phy->name = of_get_property(pdev->dev.of_node, "label", NULL);
	if (!dsi_phy->name)
		dsi_phy->name = DSI_PHY_DEFAULT_LABEL;

	pr_debug("Probing %s device\n", dsi_phy->name);

	rc = dsi_phy_regmap_init(pdev, dsi_phy);
	if (rc) {
		pr_err("Failed to parse register information, rc=%d\n", rc);
		goto fail;
	}

	rc = dsi_phy_clocks_init(pdev, dsi_phy);
	if (rc) {
		pr_err("failed to parse clock information, rc = %d\n", rc);
		goto fail_regmap;
	}

	rc = dsi_phy_supplies_init(pdev, dsi_phy);
	if (rc) {
		pr_err("failed to parse voltage supplies, rc = %d\n", rc);
		goto fail_clks;
	}

	rc = dsi_catalog_phy_setup(&dsi_phy->hw, ver_info->version,
				   dsi_phy->index);
	if (rc) {
		pr_err("Catalog does not support version (%d)\n",
		       ver_info->version);
		goto fail_supplies;
	}

	dsi_phy->ver_info = ver_info;
	rc = dsi_phy_settings_init(pdev, dsi_phy);
	if (rc) {
		pr_err("Failed to parse phy setting, rc=%d\n", rc);
		goto fail_supplies;
	}

	item->phy = dsi_phy;

	mutex_lock(&dsi_phy_list_lock);
	list_add(&item->list, &dsi_phy_list);
	mutex_unlock(&dsi_phy_list_lock);

	mutex_init(&dsi_phy->phy_lock);
	/** TODO: initialize debugfs */
	dsi_phy->pdev = pdev;
	platform_set_drvdata(pdev, dsi_phy);
	pr_debug("Probe successful for %s\n", dsi_phy->name);
	return 0;

fail_supplies:
	(void)dsi_phy_supplies_deinit(dsi_phy);
fail_clks:
	(void)dsi_phy_clocks_deinit(dsi_phy);
fail_regmap:
	(void)dsi_phy_regmap_deinit(dsi_phy);
fail:
	devm_kfree(&pdev->dev, dsi_phy);
	devm_kfree(&pdev->dev, item);
	return rc;
}

static int dsi_phy_driver_remove(struct platform_device *pdev)
{
	int rc = 0;
	struct msm_dsi_phy *phy = platform_get_drvdata(pdev);
	struct list_head *pos, *tmp;

	if (!pdev || !phy) {
		pr_err("Invalid device\n");
		return -EINVAL;
	}

	mutex_lock(&dsi_phy_list_lock);
	list_for_each_safe(pos, tmp, &dsi_phy_list) {
		struct dsi_phy_list_item *n;

		n = list_entry(pos, struct dsi_phy_list_item, list);
		if (n->phy == phy) {
			list_del(&n->list);
			devm_kfree(&pdev->dev, n);
			break;
		}
	}
	mutex_unlock(&dsi_phy_list_lock);

	mutex_lock(&phy->phy_lock);
	rc = dsi_phy_settings_deinit(phy);
	if (rc)
		pr_err("failed to deinitialize phy settings, rc=%d\n", rc);

	rc = dsi_phy_supplies_deinit(phy);
	if (rc)
		pr_err("failed to deinitialize voltage supplies, rc=%d\n", rc);

	rc = dsi_phy_clocks_deinit(phy);
	if (rc)
		pr_err("failed to deinitialize clocks, rc=%d\n", rc);

	rc = dsi_phy_regmap_deinit(phy);
	if (rc)
		pr_err("failed to deinitialize regmap, rc=%d\n", rc);
	mutex_unlock(&phy->phy_lock);

	mutex_destroy(&phy->phy_lock);
	devm_kfree(&pdev->dev, phy);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver dsi_phy_platform_driver = {
	.probe      = dsi_phy_driver_probe,
	.remove     = dsi_phy_driver_remove,
	.driver     = {
		.name   = "msm_dsi_phy",
		.of_match_table = msm_dsi_phy_of_match,
	},
};

static void dsi_phy_enable_hw(struct msm_dsi_phy *phy)
{
	if (phy->hw.ops.regulator_enable)
		phy->hw.ops.regulator_enable(&phy->hw, &phy->cfg.regulators);

	if (phy->hw.ops.enable)
		phy->hw.ops.enable(&phy->hw, &phy->cfg);
}

static void dsi_phy_disable_hw(struct msm_dsi_phy *phy)
{
	if (phy->hw.ops.disable)
		phy->hw.ops.disable(&phy->hw);

	if (phy->hw.ops.regulator_disable)
		phy->hw.ops.regulator_disable(&phy->hw);
}

/**
 * dsi_phy_get() - get a dsi phy handle from device node
 * @of_node:           device node for dsi phy controller
 *
 * Gets the DSI PHY handle for the corresponding of_node. The ref count is
 * incremented to one all subsequents get will fail until the original client
 * calls a put.
 *
 * Return: DSI PHY handle or an error code.
 */
struct msm_dsi_phy *dsi_phy_get(struct device_node *of_node)
{
	struct list_head *pos, *tmp;
	struct msm_dsi_phy *phy = NULL;

	mutex_lock(&dsi_phy_list_lock);
	list_for_each_safe(pos, tmp, &dsi_phy_list) {
		struct dsi_phy_list_item *n;

		n = list_entry(pos, struct dsi_phy_list_item, list);
		if (n->phy->pdev->dev.of_node == of_node) {
			phy = n->phy;
			break;
		}
	}
	mutex_unlock(&dsi_phy_list_lock);

	if (!phy) {
		pr_err("Device with of node not found\n");
		phy = ERR_PTR(-EPROBE_DEFER);
		return phy;
	}

	mutex_lock(&phy->phy_lock);
	if (phy->refcount > 0) {
		pr_err("[PHY_%d] Device under use\n", phy->index);
		phy = ERR_PTR(-EINVAL);
	} else {
		phy->refcount++;
	}
	mutex_unlock(&phy->phy_lock);
	return phy;
}

/**
 * dsi_phy_put() - release dsi phy handle
 * @dsi_phy:              DSI PHY handle.
 *
 * Release the DSI PHY hardware. Driver will clean up all resources and puts
 * back the DSI PHY into reset state.
 */
void dsi_phy_put(struct msm_dsi_phy *dsi_phy)
{
	mutex_lock(&dsi_phy->phy_lock);

	if (dsi_phy->refcount == 0)
		pr_err("Unbalanced dsi_phy_put call\n");
	else
		dsi_phy->refcount--;

	mutex_unlock(&dsi_phy->phy_lock);
}

/**
 * dsi_phy_drv_init() - initialize dsi phy driver
 * @dsi_phy:         DSI PHY handle.
 *
 * Initializes DSI PHY driver. Should be called after dsi_phy_get().
 *
 * Return: error code.
 */
int dsi_phy_drv_init(struct msm_dsi_phy *dsi_phy)
{
	return 0;
}

/**
 * dsi_phy_drv_deinit() - de-initialize dsi phy driver
 * @dsi_phy:          DSI PHY handle.
 *
 * Release all resources acquired by dsi_phy_drv_init().
 *
 * Return: error code.
 */
int dsi_phy_drv_deinit(struct msm_dsi_phy *dsi_phy)
{
	return 0;
}

/**
 * dsi_phy_validate_mode() - validate a display mode
 * @dsi_phy:            DSI PHY handle.
 * @mode:               Mode information.
 *
 * Validation will fail if the mode cannot be supported by the PHY driver or
 * hardware.
 *
 * Return: error code.
 */
int dsi_phy_validate_mode(struct msm_dsi_phy *dsi_phy,
			  struct dsi_mode_info *mode)
{
	int rc = 0;

	if (!dsi_phy || !mode) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&dsi_phy->phy_lock);

	pr_debug("[PHY_%d] Skipping validation\n", dsi_phy->index);

	mutex_unlock(&dsi_phy->phy_lock);
	return rc;
}

/**
 * dsi_phy_set_power_state() - enable/disable dsi phy power supplies
 * @dsi_phy:               DSI PHY handle.
 * @enable:                Boolean flag to enable/disable.
 *
 * Return: error code.
 */
int dsi_phy_set_power_state(struct msm_dsi_phy *dsi_phy, bool enable)
{
	int rc = 0;

	if (!dsi_phy) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&dsi_phy->phy_lock);

	if (enable == dsi_phy->power_state) {
		pr_err("[PHY_%d] No state change\n", dsi_phy->index);
		goto error;
	}

	if (enable) {
		rc = dsi_pwr_enable_regulator(&dsi_phy->pwr_info.digital, true);
		if (rc) {
			pr_err("failed to enable digital regulator\n");
			goto error;
		}
		rc = dsi_pwr_enable_regulator(&dsi_phy->pwr_info.phy_pwr, true);
		if (rc) {
			pr_err("failed to enable phy power\n");
			(void)dsi_pwr_enable_regulator(
						&dsi_phy->pwr_info.digital,
						false
						);
			goto error;
		}
	} else {
		rc = dsi_pwr_enable_regulator(&dsi_phy->pwr_info.phy_pwr,
					      false);
		if (rc) {
			pr_err("failed to enable digital regulator\n");
			goto error;
		}
		rc = dsi_pwr_enable_regulator(&dsi_phy->pwr_info.digital,
					      false);
		if (rc) {
			pr_err("failed to enable phy power\n");
			goto error;
		}
	}

	dsi_phy->power_state = enable;
error:
	mutex_unlock(&dsi_phy->phy_lock);
	return rc;
}

/**
 * dsi_phy_enable() - enable DSI PHY hardware
 * @dsi_phy:            DSI PHY handle.
 * @config:             DSI host configuration.
 * @pll_source:         Source PLL for PHY clock.
 * @skip_validation:    Validation will not be performed on parameters.
 *
 * Validates and enables DSI PHY.
 *
 * Return: error code.
 */
int dsi_phy_enable(struct msm_dsi_phy *phy,
		   struct dsi_host_config *config,
		   enum dsi_phy_pll_source pll_source,
		   bool skip_validation)
{
	int rc = 0;

	if (!phy || !config) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&phy->phy_lock);

	if (!skip_validation)
		pr_debug("[PHY_%d] TODO: perform validation\n", phy->index);

	rc = dsi_clk_enable_core_clks(&phy->clks.core_clks, true);
	if (rc) {
		pr_err("failed to enable core clocks, rc=%d\n", rc);
		goto error;
	}

	memcpy(&phy->mode, &config->video_timing, sizeof(phy->mode));
	phy->data_lanes = config->common_config.data_lanes;
	phy->dst_format = config->common_config.dst_format;
	phy->lane_map = config->lane_map;
	phy->cfg.pll_source = pll_source;

	rc = phy->hw.ops.calculate_timing_params(&phy->hw,
						 &phy->mode,
						 &config->common_config,
						 &phy->cfg.timing);
	if (rc) {
		pr_err("[%s] failed to set timing, rc=%d\n", phy->name, rc);
		goto error_disable_clks;
	}

	dsi_phy_enable_hw(phy);

error_disable_clks:
	rc = dsi_clk_enable_core_clks(&phy->clks.core_clks, false);
	if (rc) {
		pr_err("failed to disable clocks, skip phy disable\n");
		goto error;
	}
error:
	mutex_unlock(&phy->phy_lock);
	return rc;
}

/**
 * dsi_phy_disable() - disable DSI PHY hardware.
 * @phy:        DSI PHY handle.
 *
 * Return: error code.
 */
int dsi_phy_disable(struct msm_dsi_phy *phy)
{
	int rc = 0;

	if (!phy) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&phy->phy_lock);

	rc = dsi_clk_enable_core_clks(&phy->clks.core_clks, true);
	if (rc) {
		pr_err("failed to enable core clocks, rc=%d\n", rc);
		goto error;
	}

	dsi_phy_disable_hw(phy);

	rc = dsi_clk_enable_core_clks(&phy->clks.core_clks, false);
	if (rc) {
		pr_err("failed to disable core clocks, rc=%d\n", rc);
		goto error;
	}

error:
	mutex_unlock(&phy->phy_lock);
	return rc;
}

/**
 * dsi_phy_set_timing_params() - timing parameters for the panel
 * @phy:          DSI PHY handle
 * @timing:       array holding timing params.
 * @size:         size of the array.
 *
 * When PHY timing calculator is not implemented, this array will be used to
 * pass PHY timing information.
 *
 * Return: error code.
 */
int dsi_phy_set_timing_params(struct msm_dsi_phy *phy,
			      u8 *timing, u32 size)
{
	int rc = 0;
	int i, j;
	struct dsi_phy_per_lane_cfgs *timing_cfg;

	if (!phy || !timing || !size) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&phy->phy_lock);

	if (size != (DSI_LANE_MAX * phy->cfg.timing.count_per_lane)) {
		pr_err("Unexpected timing array size %d\n", size);
		rc = -EINVAL;
	} else {
		timing_cfg = &phy->cfg.timing;
		for (i = DSI_LOGICAL_LANE_0; i < DSI_LANE_MAX; i++) {
			for (j = 0; j < timing_cfg->count_per_lane; j++) {
				timing_cfg->lane[i][j] = *timing;
				timing++;
			}
		}
	}
	mutex_unlock(&phy->phy_lock);
	return rc;
}

void __init dsi_phy_drv_register(void)
{
	platform_driver_register(&dsi_phy_platform_driver);
}

void __exit dsi_phy_drv_unregister(void)
{
	platform_driver_unregister(&dsi_phy_platform_driver);
}
