/*
 * dspapi.c
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Common DSP API functions, also includes the wrapper
 * functions called directly by the DeviceIOControl interface.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <linux/types.h>

/*  ----------------------------------- Host OS */
#include <dspbridge/host_os.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dspbridge/dbdefs.h>

/*  ----------------------------------- Trace & Debug */
#include <dspbridge/dbc.h>

/*  ----------------------------------- OS Adaptation Layer */
#include <dspbridge/cfg.h>
#include <dspbridge/ntfy.h>
#include <dspbridge/services.h>

/*  ----------------------------------- Platform Manager */
#include <dspbridge/chnl.h>
#include <dspbridge/dev.h>
#include <dspbridge/drv.h>

#include <dspbridge/proc.h>
#include <dspbridge/strm.h>

/*  ----------------------------------- Resource Manager */
#include <dspbridge/disp.h>
#include <dspbridge/mgr.h>
#include <dspbridge/node.h>
#include <dspbridge/rmm.h>

/*  ----------------------------------- Others */
#include <dspbridge/msg.h>
#include <dspbridge/cmm.h>
#include <dspbridge/io.h>

/*  ----------------------------------- This */
#include <dspbridge/dspapi.h>
#include <dspbridge/dbdcd.h>

#include <dspbridge/resourcecleanup.h>

/*  ----------------------------------- Defines, Data Structures, Typedefs */
#define MAX_TRACEBUFLEN 255
#define MAX_LOADARGS    16
#define MAX_NODES       64
#define MAX_STREAMS     16
#define MAX_BUFS	64

/* Used to get dspbridge ioctl table */
#define DB_GET_IOC_TABLE(cmd)	(DB_GET_MODULE(cmd) >> DB_MODULE_SHIFT)

/* Device IOCtl function pointer */
struct api_cmd {
	u32(*fxn) (union trapped_args *args, void *pr_ctxt);
	u32 dw_index;
};

/*  ----------------------------------- Globals */
static u32 api_c_refs;

/*
 *  Function tables.
 *  The order of these functions MUST be the same as the order of the command
 *  numbers defined in dspapi-ioctl.h  This is how an IOCTL number in user mode
 *  turns into a function call in kernel mode.
 */

/* MGR wrapper functions */
static struct api_cmd mgr_cmd[] = {
	{mgrwrap_enum_node_info},	/* MGR_ENUMNODE_INFO */
	{mgrwrap_enum_proc_info},	/* MGR_ENUMPROC_INFO */
	{mgrwrap_register_object},	/* MGR_REGISTEROBJECT */
	{mgrwrap_unregister_object},	/* MGR_UNREGISTEROBJECT */
	{mgrwrap_wait_for_bridge_events},	/* MGR_WAIT */
	{mgrwrap_get_process_resources_info},	/* MGR_GET_PROC_RES */
};

/* PROC wrapper functions */
static struct api_cmd proc_cmd[] = {
	{procwrap_attach},	/* PROC_ATTACH */
	{procwrap_ctrl},	/* PROC_CTRL */
	{procwrap_detach},	/* PROC_DETACH */
	{procwrap_enum_node_info},	/* PROC_ENUMNODE */
	{procwrap_enum_resources},	/* PROC_ENUMRESOURCES */
	{procwrap_get_state},	/* PROC_GET_STATE */
	{procwrap_get_trace},	/* PROC_GET_TRACE */
	{procwrap_load},	/* PROC_LOAD */
	{procwrap_register_notify},	/* PROC_REGISTERNOTIFY */
	{procwrap_start},	/* PROC_START */
	{procwrap_reserve_memory},	/* PROC_RSVMEM */
	{procwrap_un_reserve_memory},	/* PROC_UNRSVMEM */
	{procwrap_map},		/* PROC_MAPMEM */
	{procwrap_un_map},	/* PROC_UNMAPMEM */
	{procwrap_flush_memory},	/* PROC_FLUSHMEMORY */
	{procwrap_stop},	/* PROC_STOP */
	{procwrap_invalidate_memory},	/* PROC_INVALIDATEMEMORY */
	{procwrap_begin_dma},	/* PROC_BEGINDMA */
	{procwrap_end_dma},	/* PROC_ENDDMA */
};

/* NODE wrapper functions */
static struct api_cmd node_cmd[] = {
	{nodewrap_allocate},	/* NODE_ALLOCATE */
	{nodewrap_alloc_msg_buf},	/* NODE_ALLOCMSGBUF */
	{nodewrap_change_priority},	/* NODE_CHANGEPRIORITY */
	{nodewrap_connect},	/* NODE_CONNECT */
	{nodewrap_create},	/* NODE_CREATE */
	{nodewrap_delete},	/* NODE_DELETE */
	{nodewrap_free_msg_buf},	/* NODE_FREEMSGBUF */
	{nodewrap_get_attr},	/* NODE_GETATTR */
	{nodewrap_get_message},	/* NODE_GETMESSAGE */
	{nodewrap_pause},	/* NODE_PAUSE */
	{nodewrap_put_message},	/* NODE_PUTMESSAGE */
	{nodewrap_register_notify},	/* NODE_REGISTERNOTIFY */
	{nodewrap_run},		/* NODE_RUN */
	{nodewrap_terminate},	/* NODE_TERMINATE */
	{nodewrap_get_uuid_props},	/* NODE_GETUUIDPROPS */
};

/* STRM wrapper functions */
static struct api_cmd strm_cmd[] = {
	{strmwrap_allocate_buffer},	/* STRM_ALLOCATEBUFFER */
	{strmwrap_close},	/* STRM_CLOSE */
	{strmwrap_free_buffer},	/* STRM_FREEBUFFER */
	{strmwrap_get_event_handle},	/* STRM_GETEVENTHANDLE */
	{strmwrap_get_info},	/* STRM_GETINFO */
	{strmwrap_idle},	/* STRM_IDLE */
	{strmwrap_issue},	/* STRM_ISSUE */
	{strmwrap_open},	/* STRM_OPEN */
	{strmwrap_reclaim},	/* STRM_RECLAIM */
	{strmwrap_register_notify},	/* STRM_REGISTERNOTIFY */
	{strmwrap_select},	/* STRM_SELECT */
};

/* CMM wrapper functions */
static struct api_cmd cmm_cmd[] = {
	{cmmwrap_calloc_buf},	/* CMM_ALLOCBUF */
	{cmmwrap_free_buf},	/* CMM_FREEBUF */
	{cmmwrap_get_handle},	/* CMM_GETHANDLE */
	{cmmwrap_get_info},	/* CMM_GETINFO */
};

/* Array used to store ioctl table sizes. It can hold up to 8 entries */
static u8 size_cmd[] = {
	ARRAY_SIZE(mgr_cmd),
	ARRAY_SIZE(proc_cmd),
	ARRAY_SIZE(node_cmd),
	ARRAY_SIZE(strm_cmd),
	ARRAY_SIZE(cmm_cmd),
};

static inline void _cp_fm_usr(void *to, const void __user * from,
			      int *err, unsigned long bytes)
{
	if (DSP_FAILED(*err))
		return;

	if (unlikely(!from)) {
		*err = -EFAULT;
		return;
	}

	if (unlikely(copy_from_user(to, from, bytes)))
		*err = -EFAULT;
}

#define CP_FM_USR(to, from, err, n)				\
	_cp_fm_usr(to, from, &(err), (n) * sizeof(*(to)))

static inline void _cp_to_usr(void __user *to, const void *from,
			      int *err, unsigned long bytes)
{
	if (DSP_FAILED(*err))
		return;

	if (unlikely(!to)) {
		*err = -EFAULT;
		return;
	}

	if (unlikely(copy_to_user(to, from, bytes)))
		*err = -EFAULT;
}

#define CP_TO_USR(to, from, err, n)				\
	_cp_to_usr(to, from, &(err), (n) * sizeof(*(from)))

/*
 *  ======== api_call_dev_ioctl ========
 *  Purpose:
 *      Call the (wrapper) function for the corresponding API IOCTL.
 */
inline int api_call_dev_ioctl(u32 cmd, union trapped_args *args,
				      u32 *result, void *pr_ctxt)
{
	u32(*ioctl_cmd) (union trapped_args *args, void *pr_ctxt) = NULL;
	int i;

	if (_IOC_TYPE(cmd) != DB) {
		pr_err("%s: Incompatible dspbridge ioctl number\n", __func__);
		goto err;
	}

	if (DB_GET_IOC_TABLE(cmd) > ARRAY_SIZE(size_cmd)) {
		pr_err("%s: undefined ioctl module\n", __func__);
		goto err;
	}

	/* Check the size of the required cmd table */
	i = DB_GET_IOC(cmd);
	if (i > size_cmd[DB_GET_IOC_TABLE(cmd)]) {
		pr_err("%s: requested ioctl %d out of bounds for table %d\n",
		       __func__, i, DB_GET_IOC_TABLE(cmd));
		goto err;
	}

	switch (DB_GET_MODULE(cmd)) {
	case DB_MGR:
		ioctl_cmd = mgr_cmd[i].fxn;
		break;
	case DB_PROC:
		ioctl_cmd = proc_cmd[i].fxn;
		break;
	case DB_NODE:
		ioctl_cmd = node_cmd[i].fxn;
		break;
	case DB_STRM:
		ioctl_cmd = strm_cmd[i].fxn;
		break;
	case DB_CMM:
		ioctl_cmd = cmm_cmd[i].fxn;
		break;
	}

	if (!ioctl_cmd) {
		pr_err("%s: requested ioctl not defined\n", __func__);
		goto err;
	} else {
		*result = (*ioctl_cmd) (args, pr_ctxt);
	}

	return 0;

err:
	return -EINVAL;
}

/*
 *  ======== api_exit ========
 */
void api_exit(void)
{
	DBC_REQUIRE(api_c_refs > 0);
	api_c_refs--;

	if (api_c_refs == 0) {
		/* Release all modules initialized in api_init(). */
		cod_exit();
		dev_exit();
		chnl_exit();
		msg_exit();
		io_exit();
		strm_exit();
		disp_exit();
		node_exit();
		proc_exit();
		mgr_exit();
		rmm_exit();
		drv_exit();
	}
	DBC_ENSURE(api_c_refs >= 0);
}

/*
 *  ======== api_init ========
 *  Purpose:
 *      Module initialization used by Bridge API.
 */
bool api_init(void)
{
	bool ret = true;
	bool fdrv, fdev, fcod, fchnl, fmsg, fio;
	bool fmgr, fproc, fnode, fdisp, fstrm, frmm;

	if (api_c_refs == 0) {
		/* initialize driver and other modules */
		fdrv = drv_init();
		fmgr = mgr_init();
		fproc = proc_init();
		fnode = node_init();
		fdisp = disp_init();
		fstrm = strm_init();
		frmm = rmm_init();
		fchnl = chnl_init();
		fmsg = msg_mod_init();
		fio = io_init();
		fdev = dev_init();
		fcod = cod_init();
		ret = fdrv && fdev && fchnl && fcod && fmsg && fio;
		ret = ret && fmgr && fproc && frmm;
		if (!ret) {
			if (fdrv)
				drv_exit();

			if (fmgr)
				mgr_exit();

			if (fstrm)
				strm_exit();

			if (fproc)
				proc_exit();

			if (fnode)
				node_exit();

			if (fdisp)
				disp_exit();

			if (fchnl)
				chnl_exit();

			if (fmsg)
				msg_exit();

			if (fio)
				io_exit();

			if (fdev)
				dev_exit();

			if (fcod)
				cod_exit();

			if (frmm)
				rmm_exit();

		}
	}
	if (ret)
		api_c_refs++;

	return ret;
}

/*
 *  ======== api_init_complete2 ========
 *  Purpose:
 *      Perform any required bridge initialization which cannot
 *      be performed in api_init() or dev_start_device() due
 *      to the fact that some services are not yet
 *      completely initialized.
 *  Parameters:
 *  Returns:
 *      0:	Allow this device to load
 *      -EPERM:      Failure.
 *  Requires:
 *      Bridge API initialized.
 *  Ensures:
 */
int api_init_complete2(void)
{
	int status = 0;
	struct cfg_devnode *dev_node;
	struct dev_object *hdev_obj;
	u8 dev_type;
	u32 tmp;

	DBC_REQUIRE(api_c_refs > 0);

	/*  Walk the list of DevObjects, get each devnode, and attempting to
	 *  autostart the board. Note that this requires COF loading, which
	 *  requires KFILE. */
	for (hdev_obj = dev_get_first(); hdev_obj != NULL;
	     hdev_obj = dev_get_next(hdev_obj)) {
		if (DSP_FAILED(dev_get_dev_node(hdev_obj, &dev_node)))
			continue;

		if (DSP_FAILED(dev_get_dev_type(hdev_obj, &dev_type)))
			continue;

		if ((dev_type == DSP_UNIT) || (dev_type == IVA_UNIT))
			if (cfg_get_auto_start(dev_node, &tmp) == 0
									&& tmp)
				proc_auto_start(dev_node, hdev_obj);
	}

	return status;
}

/* TODO: Remove deprecated and not implemented ioctl wrappers */

/*
 * ======== mgrwrap_enum_node_info ========
 */
u32 mgrwrap_enum_node_info(union trapped_args *args, void *pr_ctxt)
{
	u8 *pndb_props;
	u32 num_nodes;
	int status = 0;
	u32 size = args->args_mgr_enumnode_info.undb_props_size;

	if (size < sizeof(struct dsp_ndbprops))
		return -EINVAL;

	pndb_props = kmalloc(size, GFP_KERNEL);
	if (pndb_props == NULL)
		status = -ENOMEM;

	if (DSP_SUCCEEDED(status)) {
		status =
		    mgr_enum_node_info(args->args_mgr_enumnode_info.node_id,
				       (struct dsp_ndbprops *)pndb_props, size,
				       &num_nodes);
	}
	CP_TO_USR(args->args_mgr_enumnode_info.pndb_props, pndb_props, status,
		  size);
	CP_TO_USR(args->args_mgr_enumnode_info.pu_num_nodes, &num_nodes, status,
		  1);
	kfree(pndb_props);

	return status;
}

/*
 * ======== mgrwrap_enum_proc_info ========
 */
u32 mgrwrap_enum_proc_info(union trapped_args *args, void *pr_ctxt)
{
	u8 *processor_info;
	u8 num_procs;
	int status = 0;
	u32 size = args->args_mgr_enumproc_info.processor_info_size;

	if (size < sizeof(struct dsp_processorinfo))
		return -EINVAL;

	processor_info = kmalloc(size, GFP_KERNEL);
	if (processor_info == NULL)
		status = -ENOMEM;

	if (DSP_SUCCEEDED(status)) {
		status =
		    mgr_enum_processor_info(args->args_mgr_enumproc_info.
					    processor_id,
					    (struct dsp_processorinfo *)
					    processor_info, size, &num_procs);
	}
	CP_TO_USR(args->args_mgr_enumproc_info.processor_info, processor_info,
		  status, size);
	CP_TO_USR(args->args_mgr_enumproc_info.pu_num_procs, &num_procs,
		  status, 1);
	kfree(processor_info);

	return status;
}

#define WRAP_MAP2CALLER(x) x
/*
 * ======== mgrwrap_register_object ========
 */
u32 mgrwrap_register_object(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;
	struct dsp_uuid uuid_obj;
	u32 path_size = 0;
	char *psz_path_name = NULL;
	int status = 0;

	CP_FM_USR(&uuid_obj, args->args_mgr_registerobject.uuid_obj, status, 1);
	if (DSP_FAILED(status))
		goto func_end;
	/* path_size is increased by 1 to accommodate NULL */
	path_size = strlen_user((char *)
				args->args_mgr_registerobject.psz_path_name) +
	    1;
	psz_path_name = kmalloc(path_size, GFP_KERNEL);
	if (!psz_path_name)
		goto func_end;
	ret = strncpy_from_user(psz_path_name,
				(char *)args->args_mgr_registerobject.
				psz_path_name, path_size);
	if (!ret) {
		status = -EFAULT;
		goto func_end;
	}

	if (args->args_mgr_registerobject.obj_type >= DSP_DCDMAXOBJTYPE)
		return -EINVAL;

	status = dcd_register_object(&uuid_obj,
				     args->args_mgr_registerobject.obj_type,
				     (char *)psz_path_name);
func_end:
	kfree(psz_path_name);
	return status;
}

/*
 * ======== mgrwrap_unregister_object ========
 */
u32 mgrwrap_unregister_object(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_uuid uuid_obj;

	CP_FM_USR(&uuid_obj, args->args_mgr_registerobject.uuid_obj, status, 1);
	if (DSP_FAILED(status))
		goto func_end;

	status = dcd_unregister_object(&uuid_obj,
				       args->args_mgr_unregisterobject.
				       obj_type);
func_end:
	return status;

}

/*
 * ======== mgrwrap_wait_for_bridge_events ========
 */
u32 mgrwrap_wait_for_bridge_events(union trapped_args *args, void *pr_ctxt)
{
	int status = 0, real_status = 0;
	struct dsp_notification *anotifications[MAX_EVENTS];
	struct dsp_notification notifications[MAX_EVENTS];
	u32 index, i;
	u32 count = args->args_mgr_wait.count;

	if (count > MAX_EVENTS)
		status = -EINVAL;

	/* get the array of pointers to user structures */
	CP_FM_USR(anotifications, args->args_mgr_wait.anotifications,
		  status, count);
	/* get the events */
	for (i = 0; i < count; i++) {
		CP_FM_USR(&notifications[i], anotifications[i], status, 1);
		if (DSP_SUCCEEDED(status)) {
			/* set the array of pointers to kernel structures */
			anotifications[i] = &notifications[i];
		}
	}
	if (DSP_SUCCEEDED(status)) {
		real_status = mgr_wait_for_bridge_events(anotifications, count,
							 &index,
							 args->args_mgr_wait.
							 utimeout);
	}
	CP_TO_USR(args->args_mgr_wait.pu_index, &index, status, 1);
	return real_status;
}

/*
 * ======== MGRWRAP_GetProcessResourceInfo ========
 */
u32 __deprecated mgrwrap_get_process_resources_info(union trapped_args * args,
						    void *pr_ctxt)
{
	pr_err("%s: deprecated dspbridge ioctl\n", __func__);
	return 0;
}

/*
 * ======== procwrap_attach ========
 */
u32 procwrap_attach(union trapped_args *args, void *pr_ctxt)
{
	void *processor;
	int status = 0;
	struct dsp_processorattrin proc_attr_in, *attr_in = NULL;

	/* Optional argument */
	if (args->args_proc_attach.attr_in) {
		CP_FM_USR(&proc_attr_in, args->args_proc_attach.attr_in, status,
			  1);
		if (DSP_SUCCEEDED(status))
			attr_in = &proc_attr_in;
		else
			goto func_end;

	}
	status = proc_attach(args->args_proc_attach.processor_id, attr_in,
			     &processor, pr_ctxt);
	CP_TO_USR(args->args_proc_attach.ph_processor, &processor, status, 1);
func_end:
	return status;
}

/*
 * ======== procwrap_ctrl ========
 */
u32 procwrap_ctrl(union trapped_args *args, void *pr_ctxt)
{
	u32 cb_data_size, __user * psize = (u32 __user *)
	    args->args_proc_ctrl.pargs;
	u8 *pargs = NULL;
	int status = 0;

	if (psize) {
		if (get_user(cb_data_size, psize)) {
			status = -EPERM;
			goto func_end;
		}
		cb_data_size += sizeof(u32);
		pargs = kmalloc(cb_data_size, GFP_KERNEL);
		if (pargs == NULL) {
			status = -ENOMEM;
			goto func_end;
		}

		CP_FM_USR(pargs, args->args_proc_ctrl.pargs, status,
			  cb_data_size);
	}
	if (DSP_SUCCEEDED(status)) {
		status = proc_ctrl(args->args_proc_ctrl.hprocessor,
				   args->args_proc_ctrl.dw_cmd,
				   (struct dsp_cbdata *)pargs);
	}

	/* CP_TO_USR(args->args_proc_ctrl.pargs, pargs, status, 1); */
	kfree(pargs);
func_end:
	return status;
}

/*
 * ======== procwrap_detach ========
 */
u32 __deprecated procwrap_detach(union trapped_args * args, void *pr_ctxt)
{
	/* proc_detach called at bridge_release only */
	pr_err("%s: deprecated dspbridge ioctl\n", __func__);
	return 0;
}

/*
 * ======== procwrap_enum_node_info ========
 */
u32 procwrap_enum_node_info(union trapped_args *args, void *pr_ctxt)
{
	int status;
	void *node_tab[MAX_NODES];
	u32 num_nodes;
	u32 alloc_cnt;

	if (!args->args_proc_enumnode_info.node_tab_size)
		return -EINVAL;

	status = proc_enum_nodes(args->args_proc_enumnode_info.hprocessor,
				 node_tab,
				 args->args_proc_enumnode_info.node_tab_size,
				 &num_nodes, &alloc_cnt);
	CP_TO_USR(args->args_proc_enumnode_info.node_tab, node_tab, status,
		  num_nodes);
	CP_TO_USR(args->args_proc_enumnode_info.pu_num_nodes, &num_nodes,
		  status, 1);
	CP_TO_USR(args->args_proc_enumnode_info.pu_allocated, &alloc_cnt,
		  status, 1);
	return status;
}

u32 procwrap_end_dma(union trapped_args *args, void *pr_ctxt)
{
	int status;

	if (args->args_proc_dma.dir >= DMA_NONE)
		return -EINVAL;

	status = proc_end_dma(pr_ctxt,
				   args->args_proc_dma.pmpu_addr,
				   args->args_proc_dma.ul_size,
				   args->args_proc_dma.dir);
	return status;
}

u32 procwrap_begin_dma(union trapped_args *args, void *pr_ctxt)
{
	int status;

	if (args->args_proc_dma.dir >= DMA_NONE)
		return -EINVAL;

	status = proc_begin_dma(pr_ctxt,
				   args->args_proc_dma.pmpu_addr,
				   args->args_proc_dma.ul_size,
				   args->args_proc_dma.dir);
	return status;
}

/*
 * ======== procwrap_flush_memory ========
 */
u32 procwrap_flush_memory(union trapped_args *args, void *pr_ctxt)
{
	int status;

	if (args->args_proc_flushmemory.ul_flags >
	    PROC_WRITEBACK_INVALIDATE_MEM)
		return -EINVAL;

	status = proc_flush_memory(pr_ctxt,
				   args->args_proc_flushmemory.pmpu_addr,
				   args->args_proc_flushmemory.ul_size,
				   args->args_proc_flushmemory.ul_flags);
	return status;
}

/*
 * ======== procwrap_invalidate_memory ========
 */
u32 procwrap_invalidate_memory(union trapped_args *args, void *pr_ctxt)
{
	int status;

	status =
	    proc_invalidate_memory(pr_ctxt,
				   args->args_proc_invalidatememory.pmpu_addr,
				   args->args_proc_invalidatememory.ul_size);
	return status;
}

/*
 * ======== procwrap_enum_resources ========
 */
u32 procwrap_enum_resources(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_resourceinfo resource_info;

	if (args->args_proc_enumresources.resource_info_size <
	    sizeof(struct dsp_resourceinfo))
		return -EINVAL;

	status =
	    proc_get_resource_info(args->args_proc_enumresources.hprocessor,
				   args->args_proc_enumresources.resource_type,
				   &resource_info,
				   args->args_proc_enumresources.
				   resource_info_size);

	CP_TO_USR(args->args_proc_enumresources.resource_info, &resource_info,
		  status, 1);

	return status;

}

/*
 * ======== procwrap_get_state ========
 */
u32 procwrap_get_state(union trapped_args *args, void *pr_ctxt)
{
	int status;
	struct dsp_processorstate proc_state;

	if (args->args_proc_getstate.state_info_size <
	    sizeof(struct dsp_processorstate))
		return -EINVAL;

	status =
	    proc_get_state(args->args_proc_getstate.hprocessor, &proc_state,
			   args->args_proc_getstate.state_info_size);
	CP_TO_USR(args->args_proc_getstate.proc_state_obj, &proc_state, status,
		  1);
	return status;

}

/*
 * ======== procwrap_get_trace ========
 */
u32 procwrap_get_trace(union trapped_args *args, void *pr_ctxt)
{
	int status;
	u8 *pbuf;

	if (args->args_proc_gettrace.max_size > MAX_TRACEBUFLEN)
		return -EINVAL;

	pbuf = kzalloc(args->args_proc_gettrace.max_size, GFP_KERNEL);
	if (pbuf != NULL) {
		status = proc_get_trace(args->args_proc_gettrace.hprocessor,
					pbuf,
					args->args_proc_gettrace.max_size);
	} else {
		status = -ENOMEM;
	}
	CP_TO_USR(args->args_proc_gettrace.pbuf, pbuf, status,
		  args->args_proc_gettrace.max_size);
	kfree(pbuf);

	return status;
}

/*
 * ======== procwrap_load ========
 */
u32 procwrap_load(union trapped_args *args, void *pr_ctxt)
{
	s32 i, len;
	int status = 0;
	char *temp;
	s32 count = args->args_proc_load.argc_index;
	u8 **argv = NULL, **envp = NULL;

	if (count <= 0 || count > MAX_LOADARGS) {
		status = -EINVAL;
		goto func_cont;
	}

	argv = kmalloc(count * sizeof(u8 *), GFP_KERNEL);
	if (!argv) {
		status = -ENOMEM;
		goto func_cont;
	}

	CP_FM_USR(argv, args->args_proc_load.user_args, status, count);
	if (DSP_FAILED(status)) {
		kfree(argv);
		argv = NULL;
		goto func_cont;
	}

	for (i = 0; i < count; i++) {
		if (argv[i]) {
			/* User space pointer to argument */
			temp = (char *)argv[i];
			/* len is increased by 1 to accommodate NULL */
			len = strlen_user((char *)temp) + 1;
			/* Kernel space pointer to argument */
			argv[i] = kmalloc(len, GFP_KERNEL);
			if (argv[i]) {
				CP_FM_USR(argv[i], temp, status, len);
				if (DSP_FAILED(status)) {
					kfree(argv[i]);
					argv[i] = NULL;
					goto func_cont;
				}
			} else {
				status = -ENOMEM;
				goto func_cont;
			}
		}
	}
	/* TODO: validate this */
	if (args->args_proc_load.user_envp) {
		/* number of elements in the envp array including NULL */
		count = 0;
		do {
			get_user(temp, args->args_proc_load.user_envp + count);
			count++;
		} while (temp);
		envp = kmalloc(count * sizeof(u8 *), GFP_KERNEL);
		if (!envp) {
			status = -ENOMEM;
			goto func_cont;
		}

		CP_FM_USR(envp, args->args_proc_load.user_envp, status, count);
		if (DSP_FAILED(status)) {
			kfree(envp);
			envp = NULL;
			goto func_cont;
		}
		for (i = 0; envp[i]; i++) {
			/* User space pointer to argument */
			temp = (char *)envp[i];
			/* len is increased by 1 to accommodate NULL */
			len = strlen_user((char *)temp) + 1;
			/* Kernel space pointer to argument */
			envp[i] = kmalloc(len, GFP_KERNEL);
			if (envp[i]) {
				CP_FM_USR(envp[i], temp, status, len);
				if (DSP_FAILED(status)) {
					kfree(envp[i]);
					envp[i] = NULL;
					goto func_cont;
				}
			} else {
				status = -ENOMEM;
				goto func_cont;
			}
		}
	}

	if (DSP_SUCCEEDED(status)) {
		status = proc_load(args->args_proc_load.hprocessor,
				   args->args_proc_load.argc_index,
				   (const char **)argv, (const char **)envp);
	}
func_cont:
	if (envp) {
		i = 0;
		while (envp[i])
			kfree(envp[i++]);

		kfree(envp);
	}

	if (argv) {
		count = args->args_proc_load.argc_index;
		for (i = 0; (i < count) && argv[i]; i++)
			kfree(argv[i]);

		kfree(argv);
	}

	return status;
}

/*
 * ======== procwrap_map ========
 */
u32 procwrap_map(union trapped_args *args, void *pr_ctxt)
{
	int status;
	void *map_addr;

	if (!args->args_proc_mapmem.ul_size)
		return -EINVAL;

	status = proc_map(args->args_proc_mapmem.hprocessor,
			  args->args_proc_mapmem.pmpu_addr,
			  args->args_proc_mapmem.ul_size,
			  args->args_proc_mapmem.req_addr, &map_addr,
			  args->args_proc_mapmem.ul_map_attr, pr_ctxt);
	if (DSP_SUCCEEDED(status)) {
		if (put_user(map_addr, args->args_proc_mapmem.pp_map_addr)) {
			status = -EINVAL;
			proc_un_map(args->args_proc_mapmem.hprocessor,
				    map_addr, pr_ctxt);
		}

	}
	return status;
}

/*
 * ======== procwrap_register_notify ========
 */
u32 procwrap_register_notify(union trapped_args *args, void *pr_ctxt)
{
	int status;
	struct dsp_notification notification;

	/* Initialize the notification data structure */
	notification.ps_name = NULL;
	notification.handle = NULL;

	status =
	    proc_register_notify(args->args_proc_register_notify.hprocessor,
				 args->args_proc_register_notify.event_mask,
				 args->args_proc_register_notify.notify_type,
				 &notification);
	CP_TO_USR(args->args_proc_register_notify.hnotification, &notification,
		  status, 1);
	return status;
}

/*
 * ======== procwrap_reserve_memory ========
 */
u32 procwrap_reserve_memory(union trapped_args *args, void *pr_ctxt)
{
	int status;
	void *prsv_addr;

	if ((args->args_proc_rsvmem.ul_size <= 0) ||
	    (args->args_proc_rsvmem.ul_size & (PG_SIZE4K - 1)) != 0)
		return -EINVAL;

	status = proc_reserve_memory(args->args_proc_rsvmem.hprocessor,
				     args->args_proc_rsvmem.ul_size, &prsv_addr,
				     pr_ctxt);
	if (DSP_SUCCEEDED(status)) {
		if (put_user(prsv_addr, args->args_proc_rsvmem.pp_rsv_addr)) {
			status = -EINVAL;
			proc_un_reserve_memory(args->args_proc_rsvmem.
					       hprocessor, prsv_addr, pr_ctxt);
		}
	}
	return status;
}

/*
 * ======== procwrap_start ========
 */
u32 procwrap_start(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = proc_start(args->args_proc_start.hprocessor);
	return ret;
}

/*
 * ======== procwrap_un_map ========
 */
u32 procwrap_un_map(union trapped_args *args, void *pr_ctxt)
{
	int status;

	status = proc_un_map(args->args_proc_unmapmem.hprocessor,
			     args->args_proc_unmapmem.map_addr, pr_ctxt);
	return status;
}

/*
 * ======== procwrap_un_reserve_memory ========
 */
u32 procwrap_un_reserve_memory(union trapped_args *args, void *pr_ctxt)
{
	int status;

	status = proc_un_reserve_memory(args->args_proc_unrsvmem.hprocessor,
					args->args_proc_unrsvmem.prsv_addr,
					pr_ctxt);
	return status;
}

/*
 * ======== procwrap_stop ========
 */
u32 procwrap_stop(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = proc_stop(args->args_proc_stop.hprocessor);

	return ret;
}

/*
 * ======== nodewrap_allocate ========
 */
u32 nodewrap_allocate(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_uuid node_uuid;
	u32 cb_data_size = 0;
	u32 __user *psize = (u32 __user *) args->args_node_allocate.pargs;
	u8 *pargs = NULL;
	struct dsp_nodeattrin proc_attr_in, *attr_in = NULL;
	struct node_object *hnode;

	/* Optional argument */
	if (psize) {
		if (get_user(cb_data_size, psize))
			status = -EPERM;

		cb_data_size += sizeof(u32);
		if (DSP_SUCCEEDED(status)) {
			pargs = kmalloc(cb_data_size, GFP_KERNEL);
			if (pargs == NULL)
				status = -ENOMEM;

		}
		CP_FM_USR(pargs, args->args_node_allocate.pargs, status,
			  cb_data_size);
	}
	CP_FM_USR(&node_uuid, args->args_node_allocate.node_id_ptr, status, 1);
	if (DSP_FAILED(status))
		goto func_cont;
	/* Optional argument */
	if (args->args_node_allocate.attr_in) {
		CP_FM_USR(&proc_attr_in, args->args_node_allocate.attr_in,
			  status, 1);
		if (DSP_SUCCEEDED(status))
			attr_in = &proc_attr_in;
		else
			status = -ENOMEM;

	}
	if (DSP_SUCCEEDED(status)) {
		status = node_allocate(args->args_node_allocate.hprocessor,
				       &node_uuid, (struct dsp_cbdata *)pargs,
				       attr_in, &hnode, pr_ctxt);
	}
	if (DSP_SUCCEEDED(status)) {
		CP_TO_USR(args->args_node_allocate.ph_node, &hnode, status, 1);
		if (DSP_FAILED(status)) {
			status = -EFAULT;
			node_delete(hnode, pr_ctxt);
		}
	}
func_cont:
	kfree(pargs);

	return status;
}

/*
 *  ======== nodewrap_alloc_msg_buf ========
 */
u32 nodewrap_alloc_msg_buf(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_bufferattr *pattr = NULL;
	struct dsp_bufferattr attr;
	u8 *pbuffer = NULL;

	if (!args->args_node_allocmsgbuf.usize)
		return -EINVAL;

	if (args->args_node_allocmsgbuf.pattr) {	/* Optional argument */
		CP_FM_USR(&attr, args->args_node_allocmsgbuf.pattr, status, 1);
		if (DSP_SUCCEEDED(status))
			pattr = &attr;

	}
	/* argument */
	CP_FM_USR(&pbuffer, args->args_node_allocmsgbuf.pbuffer, status, 1);
	if (DSP_SUCCEEDED(status)) {
		status = node_alloc_msg_buf(args->args_node_allocmsgbuf.hnode,
					    args->args_node_allocmsgbuf.usize,
					    pattr, &pbuffer);
	}
	CP_TO_USR(args->args_node_allocmsgbuf.pbuffer, &pbuffer, status, 1);
	return status;
}

/*
 * ======== nodewrap_change_priority ========
 */
u32 nodewrap_change_priority(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = node_change_priority(args->args_node_changepriority.hnode,
				   args->args_node_changepriority.prio);

	return ret;
}

/*
 * ======== nodewrap_connect ========
 */
u32 nodewrap_connect(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_strmattr attrs;
	struct dsp_strmattr *pattrs = NULL;
	u32 cb_data_size;
	u32 __user *psize = (u32 __user *) args->args_node_connect.conn_param;
	u8 *pargs = NULL;

	/* Optional argument */
	if (psize) {
		if (get_user(cb_data_size, psize))
			status = -EPERM;

		cb_data_size += sizeof(u32);
		if (DSP_SUCCEEDED(status)) {
			pargs = kmalloc(cb_data_size, GFP_KERNEL);
			if (pargs == NULL) {
				status = -ENOMEM;
				goto func_cont;
			}

		}
		CP_FM_USR(pargs, args->args_node_connect.conn_param, status,
			  cb_data_size);
		if (DSP_FAILED(status))
			goto func_cont;
	}
	if (args->args_node_connect.pattrs) {	/* Optional argument */
		CP_FM_USR(&attrs, args->args_node_connect.pattrs, status, 1);
		if (DSP_SUCCEEDED(status))
			pattrs = &attrs;

	}
	if (DSP_SUCCEEDED(status)) {
		status = node_connect(args->args_node_connect.hnode,
				      args->args_node_connect.stream_id,
				      args->args_node_connect.other_node,
				      args->args_node_connect.other_stream,
				      pattrs, (struct dsp_cbdata *)pargs);
	}
func_cont:
	kfree(pargs);

	return status;
}

/*
 * ======== nodewrap_create ========
 */
u32 nodewrap_create(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = node_create(args->args_node_create.hnode);

	return ret;
}

/*
 * ======== nodewrap_delete ========
 */
u32 nodewrap_delete(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = node_delete(args->args_node_delete.hnode, pr_ctxt);

	return ret;
}

/*
 *  ======== nodewrap_free_msg_buf ========
 */
u32 nodewrap_free_msg_buf(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_bufferattr *pattr = NULL;
	struct dsp_bufferattr attr;
	if (args->args_node_freemsgbuf.pattr) {	/* Optional argument */
		CP_FM_USR(&attr, args->args_node_freemsgbuf.pattr, status, 1);
		if (DSP_SUCCEEDED(status))
			pattr = &attr;

	}

	if (!args->args_node_freemsgbuf.pbuffer)
		return -EFAULT;

	if (DSP_SUCCEEDED(status)) {
		status = node_free_msg_buf(args->args_node_freemsgbuf.hnode,
					   args->args_node_freemsgbuf.pbuffer,
					   pattr);
	}

	return status;
}

/*
 * ======== nodewrap_get_attr ========
 */
u32 nodewrap_get_attr(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_nodeattr attr;

	status = node_get_attr(args->args_node_getattr.hnode, &attr,
			       args->args_node_getattr.attr_size);
	CP_TO_USR(args->args_node_getattr.pattr, &attr, status, 1);

	return status;
}

/*
 * ======== nodewrap_get_message ========
 */
u32 nodewrap_get_message(union trapped_args *args, void *pr_ctxt)
{
	int status;
	struct dsp_msg msg;

	status = node_get_message(args->args_node_getmessage.hnode, &msg,
				  args->args_node_getmessage.utimeout);

	CP_TO_USR(args->args_node_getmessage.message, &msg, status, 1);

	return status;
}

/*
 * ======== nodewrap_pause ========
 */
u32 nodewrap_pause(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = node_pause(args->args_node_pause.hnode);

	return ret;
}

/*
 * ======== nodewrap_put_message ========
 */
u32 nodewrap_put_message(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_msg msg;

	CP_FM_USR(&msg, args->args_node_putmessage.message, status, 1);

	if (DSP_SUCCEEDED(status)) {
		status =
		    node_put_message(args->args_node_putmessage.hnode, &msg,
				     args->args_node_putmessage.utimeout);
	}

	return status;
}

/*
 * ======== nodewrap_register_notify ========
 */
u32 nodewrap_register_notify(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_notification notification;

	/* Initialize the notification data structure */
	notification.ps_name = NULL;
	notification.handle = NULL;

	if (!args->args_proc_register_notify.event_mask)
		CP_FM_USR(&notification,
			  args->args_proc_register_notify.hnotification,
			  status, 1);

	status = node_register_notify(args->args_node_registernotify.hnode,
				      args->args_node_registernotify.event_mask,
				      args->args_node_registernotify.
				      notify_type, &notification);
	CP_TO_USR(args->args_node_registernotify.hnotification, &notification,
		  status, 1);
	return status;
}

/*
 * ======== nodewrap_run ========
 */
u32 nodewrap_run(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = node_run(args->args_node_run.hnode);

	return ret;
}

/*
 * ======== nodewrap_terminate ========
 */
u32 nodewrap_terminate(union trapped_args *args, void *pr_ctxt)
{
	int status;
	int tempstatus;

	status = node_terminate(args->args_node_terminate.hnode, &tempstatus);

	CP_TO_USR(args->args_node_terminate.pstatus, &tempstatus, status, 1);

	return status;
}

/*
 * ======== nodewrap_get_uuid_props ========
 */
u32 nodewrap_get_uuid_props(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_uuid node_uuid;
	struct dsp_ndbprops *pnode_props = NULL;

	CP_FM_USR(&node_uuid, args->args_node_getuuidprops.node_id_ptr, status,
		  1);
	if (DSP_FAILED(status))
		goto func_cont;
	pnode_props = kmalloc(sizeof(struct dsp_ndbprops), GFP_KERNEL);
	if (pnode_props != NULL) {
		status =
		    node_get_uuid_props(args->args_node_getuuidprops.hprocessor,
					&node_uuid, pnode_props);
		CP_TO_USR(args->args_node_getuuidprops.node_props, pnode_props,
			  status, 1);
	} else
		status = -ENOMEM;
func_cont:
	kfree(pnode_props);
	return status;
}

/*
 * ======== strmwrap_allocate_buffer ========
 */
u32 strmwrap_allocate_buffer(union trapped_args *args, void *pr_ctxt)
{
	int status;
	u8 **ap_buffer = NULL;
	u32 num_bufs = args->args_strm_allocatebuffer.num_bufs;

	if (num_bufs > MAX_BUFS)
		return -EINVAL;

	ap_buffer = kmalloc((num_bufs * sizeof(u8 *)), GFP_KERNEL);
	if (ap_buffer == NULL)
		return -ENOMEM;

	status = strm_allocate_buffer(args->args_strm_allocatebuffer.hstream,
				      args->args_strm_allocatebuffer.usize,
				      ap_buffer, num_bufs, pr_ctxt);
	if (DSP_SUCCEEDED(status)) {
		CP_TO_USR(args->args_strm_allocatebuffer.ap_buffer, ap_buffer,
			  status, num_bufs);
		if (DSP_FAILED(status)) {
			status = -EFAULT;
			strm_free_buffer(args->args_strm_allocatebuffer.hstream,
					 ap_buffer, num_bufs, pr_ctxt);
		}
	}
	kfree(ap_buffer);

	return status;
}

/*
 * ======== strmwrap_close ========
 */
u32 strmwrap_close(union trapped_args *args, void *pr_ctxt)
{
	return strm_close(args->args_strm_close.hstream, pr_ctxt);
}

/*
 * ======== strmwrap_free_buffer ========
 */
u32 strmwrap_free_buffer(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	u8 **ap_buffer = NULL;
	u32 num_bufs = args->args_strm_freebuffer.num_bufs;

	if (num_bufs > MAX_BUFS)
		return -EINVAL;

	ap_buffer = kmalloc((num_bufs * sizeof(u8 *)), GFP_KERNEL);
	if (ap_buffer == NULL)
		return -ENOMEM;

	CP_FM_USR(ap_buffer, args->args_strm_freebuffer.ap_buffer, status,
		  num_bufs);

	if (DSP_SUCCEEDED(status)) {
		status = strm_free_buffer(args->args_strm_freebuffer.hstream,
					  ap_buffer, num_bufs, pr_ctxt);
	}
	CP_TO_USR(args->args_strm_freebuffer.ap_buffer, ap_buffer, status,
		  num_bufs);
	kfree(ap_buffer);

	return status;
}

/*
 * ======== strmwrap_get_event_handle ========
 */
u32 __deprecated strmwrap_get_event_handle(union trapped_args * args,
					   void *pr_ctxt)
{
	pr_err("%s: deprecated dspbridge ioctl\n", __func__);
	return -ENOSYS;
}

/*
 * ======== strmwrap_get_info ========
 */
u32 strmwrap_get_info(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct stream_info strm_info;
	struct dsp_streaminfo user;
	struct dsp_streaminfo *temp;

	CP_FM_USR(&strm_info, args->args_strm_getinfo.stream_info, status, 1);
	temp = strm_info.user_strm;

	strm_info.user_strm = &user;

	if (DSP_SUCCEEDED(status)) {
		status = strm_get_info(args->args_strm_getinfo.hstream,
				       &strm_info,
				       args->args_strm_getinfo.
				       stream_info_size);
	}
	CP_TO_USR(temp, strm_info.user_strm, status, 1);
	strm_info.user_strm = temp;
	CP_TO_USR(args->args_strm_getinfo.stream_info, &strm_info, status, 1);
	return status;
}

/*
 * ======== strmwrap_idle ========
 */
u32 strmwrap_idle(union trapped_args *args, void *pr_ctxt)
{
	u32 ret;

	ret = strm_idle(args->args_strm_idle.hstream,
			args->args_strm_idle.flush_flag);

	return ret;
}

/*
 * ======== strmwrap_issue ========
 */
u32 strmwrap_issue(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;

	if (!args->args_strm_issue.pbuffer)
		return -EFAULT;

	/* No need of doing CP_FM_USR for the user buffer (pbuffer)
	   as this is done in Bridge internal function bridge_chnl_add_io_req
	   in chnl_sm.c */
	status = strm_issue(args->args_strm_issue.hstream,
			    args->args_strm_issue.pbuffer,
			    args->args_strm_issue.dw_bytes,
			    args->args_strm_issue.dw_buf_size,
			    args->args_strm_issue.dw_arg);

	return status;
}

/*
 * ======== strmwrap_open ========
 */
u32 strmwrap_open(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct strm_attr attr;
	struct strm_object *strm_obj;
	struct dsp_streamattrin strm_attr_in;

	CP_FM_USR(&attr, args->args_strm_open.attr_in, status, 1);

	if (attr.stream_attr_in != NULL) {	/* Optional argument */
		CP_FM_USR(&strm_attr_in, attr.stream_attr_in, status, 1);
		if (DSP_SUCCEEDED(status)) {
			attr.stream_attr_in = &strm_attr_in;
			if (attr.stream_attr_in->strm_mode == STRMMODE_LDMA)
				return -ENOSYS;
		}

	}
	status = strm_open(args->args_strm_open.hnode,
			   args->args_strm_open.direction,
			   args->args_strm_open.index, &attr, &strm_obj,
			   pr_ctxt);
	CP_TO_USR(args->args_strm_open.ph_stream, &strm_obj, status, 1);
	return status;
}

/*
 * ======== strmwrap_reclaim ========
 */
u32 strmwrap_reclaim(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	u8 *buf_ptr;
	u32 ul_bytes;
	u32 dw_arg;
	u32 ul_buf_size;

	status = strm_reclaim(args->args_strm_reclaim.hstream, &buf_ptr,
			      &ul_bytes, &ul_buf_size, &dw_arg);
	CP_TO_USR(args->args_strm_reclaim.buf_ptr, &buf_ptr, status, 1);
	CP_TO_USR(args->args_strm_reclaim.bytes, &ul_bytes, status, 1);
	CP_TO_USR(args->args_strm_reclaim.pdw_arg, &dw_arg, status, 1);

	if (args->args_strm_reclaim.buf_size_ptr != NULL) {
		CP_TO_USR(args->args_strm_reclaim.buf_size_ptr, &ul_buf_size,
			  status, 1);
	}

	return status;
}

/*
 * ======== strmwrap_register_notify ========
 */
u32 strmwrap_register_notify(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct dsp_notification notification;

	/* Initialize the notification data structure */
	notification.ps_name = NULL;
	notification.handle = NULL;

	status = strm_register_notify(args->args_strm_registernotify.hstream,
				      args->args_strm_registernotify.event_mask,
				      args->args_strm_registernotify.
				      notify_type, &notification);
	CP_TO_USR(args->args_strm_registernotify.hnotification, &notification,
		  status, 1);

	return status;
}

/*
 * ======== strmwrap_select ========
 */
u32 strmwrap_select(union trapped_args *args, void *pr_ctxt)
{
	u32 mask;
	struct strm_object *strm_tab[MAX_STREAMS];
	int status = 0;

	if (args->args_strm_select.strm_num > MAX_STREAMS)
		return -EINVAL;

	CP_FM_USR(strm_tab, args->args_strm_select.stream_tab, status,
		  args->args_strm_select.strm_num);
	if (DSP_SUCCEEDED(status)) {
		status = strm_select(strm_tab, args->args_strm_select.strm_num,
				     &mask, args->args_strm_select.utimeout);
	}
	CP_TO_USR(args->args_strm_select.pmask, &mask, status, 1);
	return status;
}

/* CMM */

/*
 * ======== cmmwrap_calloc_buf ========
 */
u32 __deprecated cmmwrap_calloc_buf(union trapped_args * args, void *pr_ctxt)
{
	/* This operation is done in kernel */
	pr_err("%s: deprecated dspbridge ioctl\n", __func__);
	return -ENOSYS;
}

/*
 * ======== cmmwrap_free_buf ========
 */
u32 __deprecated cmmwrap_free_buf(union trapped_args * args, void *pr_ctxt)
{
	/* This operation is done in kernel */
	pr_err("%s: deprecated dspbridge ioctl\n", __func__);
	return -ENOSYS;
}

/*
 * ======== cmmwrap_get_handle ========
 */
u32 cmmwrap_get_handle(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct cmm_object *hcmm_mgr;

	status = cmm_get_handle(args->args_cmm_gethandle.hprocessor, &hcmm_mgr);

	CP_TO_USR(args->args_cmm_gethandle.ph_cmm_mgr, &hcmm_mgr, status, 1);

	return status;
}

/*
 * ======== cmmwrap_get_info ========
 */
u32 cmmwrap_get_info(union trapped_args *args, void *pr_ctxt)
{
	int status = 0;
	struct cmm_info cmm_info_obj;

	status = cmm_get_info(args->args_cmm_getinfo.hcmm_mgr, &cmm_info_obj);

	CP_TO_USR(args->args_cmm_getinfo.cmm_info_obj, &cmm_info_obj, status,
		  1);

	return status;
}
