/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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

#ifndef __MSM_EP_PCIE_H
#define __MSM_EP_PCIE_H

#include <linux/types.h>

enum ep_pcie_link_status {
	EP_PCIE_LINK_DISABLED,
	EP_PCIE_LINK_UP,
	EP_PCIE_LINK_ENABLED,
};

enum ep_pcie_event {
	EP_PCIE_EVENT_INVALID = 0,
	EP_PCIE_EVENT_PM_D0 = 0x1,
	EP_PCIE_EVENT_PM_D3_HOT = 0x2,
	EP_PCIE_EVENT_PM_D3_COLD = 0x4,
	EP_PCIE_EVENT_PM_RST_DEAST = 0x8,
	EP_PCIE_EVENT_LINKDOWN = 0x10,
	EP_PCIE_EVENT_LINKUP = 0x20,
};

enum ep_pcie_trigger {
	EP_PCIE_TRIGGER_CALLBACK,
	EP_PCIE_TRIGGER_COMPLETION,
};

enum ep_pcie_options {
	EP_PCIE_OPT_NULL = 0,
	EP_PCIE_OPT_AST_WAKE = 0x1,
	EP_PCIE_OPT_POWER_ON = 0x2,
	EP_PCIE_OPT_ENUM = 0x4,
	EP_PCIE_OPT_ALL = 0xFFFFFFFF,
};

struct ep_pcie_notify {
	enum ep_pcie_event event;
	void *user;
	void *data;
	u32 options;
};

struct ep_pcie_register_event {
	u32 events;
	void *user;
	enum ep_pcie_trigger mode;
	void (*callback)(struct ep_pcie_notify *notify);
	struct ep_pcie_notify notify;
	struct completion *completion;
	u32 options;
};

struct ep_pcie_iatu {
	u32 start;
	u32 end;
	u32 tgt_lower;
	u32 tgt_upper;
};

struct ep_pcie_msi_config {
	u32 lower;
	u32 upper;
	u32 data;
	u32 msg_num;
};

struct ep_pcie_db_config {
	u8 base;
	u8 end;
	u32 tgt_addr;
};

struct ep_pcie_hw {
	struct list_head node;
	u32 device_id;
	void **private_data;
	int (*register_event)(struct ep_pcie_register_event *reg);
	int (*deregister_event)(void);
	enum ep_pcie_link_status (*get_linkstatus)(void);
	int (*config_outbound_iatu)(struct ep_pcie_iatu entries[],
				u32 num_entries);
	int (*get_msi_config)(struct ep_pcie_msi_config *cfg);
	int (*trigger_msi)(u32 idx);
	int (*wakeup_host)(void);
	int (*enable_endpoint)(enum ep_pcie_options opt);
	int (*disable_endpoint)(void);
	int (*config_db_routing)(struct ep_pcie_db_config chdb_cfg,
				struct ep_pcie_db_config erdb_cfg);
};

/*
 * ep_pcie_register_drv - register HW driver.
 * @phandle:	PCIe endpoint HW driver handle
 *
 * This function registers PCIe HW driver to PCIe endpoint service
 * layer.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_register_drv(struct ep_pcie_hw *phandle);

 /*
 * ep_pcie_deregister_drv - deregister HW driver.
 * @phandle:	PCIe endpoint HW driver handle
 *
 * This function deregisters PCIe HW driver to PCIe endpoint service
 * layer.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_deregister_drv(struct ep_pcie_hw *phandle);

/*
 * ep_pcie_get_phandle - get PCIe endpoint HW driver handle.
 * @id:	PCIe endpoint device ID
 *
 * This function deregisters PCIe HW driver from PCIe endpoint service
 * layer.
 *
 * Return: PCIe endpoint HW driver handle
 */
struct ep_pcie_hw *ep_pcie_get_phandle(u32 id);

/*
 * ep_pcie_register_event - register event with PCIe driver.
 * @phandle:	PCIe endpoint HW driver handle
 * @reg:	event structure
 *
 * This function gives PCIe client driver an option to register
 * event with PCIe driver.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_register_event(struct ep_pcie_hw *phandle,
	struct ep_pcie_register_event *reg);

/*
 * ep_pcie_deregister_event - deregister event with PCIe driver.
 * @phandle:	PCIe endpoint HW driver handle
 *
 * This function gives PCIe client driver an option to deregister
 * existing event with PCIe driver.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_deregister_event(struct ep_pcie_hw *phandle);

/*
 * ep_pcie_get_linkstatus - indicate the status of PCIe link.
 * @phandle:	PCIe endpoint HW driver handle
 *
 * This function tells PCIe client about the status of PCIe link.
 *
 * Return: status of PCIe link
 */
enum ep_pcie_link_status ep_pcie_get_linkstatus(struct ep_pcie_hw *phandle);

/*
 * ep_pcie_config_outbound_iatu - configure outbound iATU.
 * @entries:	iatu entries
 * @num_entries:	number of iatu entries
 *
 * This function configures the outbound iATU for PCIe
 * client's access to the regions in the host memory which
 * are specified by the SW on host side.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_config_outbound_iatu(struct ep_pcie_hw *phandle,
				struct ep_pcie_iatu entries[],
				u32 num_entries);

/*
 * ep_pcie_get_msi_config - get MSI config info.
 * @phandle:	PCIe endpoint HW driver handle
 * @cfg:	pointer to MSI config
 *
 * This function returns MSI config info.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_get_msi_config(struct ep_pcie_hw *phandle,
				struct ep_pcie_msi_config *cfg);

/*
 * ep_pcie_trigger_msi - trigger an MSI.
 * @phandle:	PCIe endpoint HW driver handle
 * @idx:	MSI index number
 *
 * This function allows PCIe client to trigger an MSI
 * on host side.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_trigger_msi(struct ep_pcie_hw *phandle, u32 idx);

/*
 * ep_pcie_wakeup_host - wake up the host.
 * @phandle:	PCIe endpoint HW driver handle
 *
 * This function asserts WAKE GPIO to wake up the host.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_wakeup_host(struct ep_pcie_hw *phandle);

/*
 * ep_pcie_enable_endpoint - enable PCIe endpoint.
 * @phandle:	PCIe endpoint HW driver handle
 * @opt:	endpoint enable options
 *
 * This function is to enable the PCIe endpoint device.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_enable_endpoint(struct ep_pcie_hw *phandle,
				enum ep_pcie_options opt);

/*
 * ep_pcie_disable_endpoint - disable PCIe endpoint.
 * @phandle:	PCIe endpoint HW driver handle
 *
 * This function is to disable the PCIe endpoint device.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_disable_endpoint(struct ep_pcie_hw *phandle);

/*
 * ep_pcie_config_db_routing - Configure routing of doorbells to another block.
 * @phandle:	PCIe endpoint HW driver handle
 * @chdb_cfg:	channel doorbell config
 * @erdb_cfg:	event ring doorbell config
 *
 * This function allows PCIe core to route the doorbells intended
 * for another entity via a target address.
 *
 * Return: 0 on success, negative value on error
 */
int ep_pcie_config_db_routing(struct ep_pcie_hw *phandle,
				struct ep_pcie_db_config chdb_cfg,
				struct ep_pcie_db_config erdb_cfg);
#endif
