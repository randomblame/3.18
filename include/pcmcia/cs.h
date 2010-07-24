/*
 * cs.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
 * are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.
 *
 * (C) 1999             David A. Hinds
 */

#ifndef _LINUX_CS_H
#define _LINUX_CS_H

#ifdef __KERNEL__
#include <linux/interrupt.h>
#endif

/* for AdjustResourceInfo */
/* Action field */
#define REMOVE_MANAGED_RESOURCE		1
#define ADD_MANAGED_RESOURCE		2

/* For CardValues field */
#define CV_OPTION_VALUE		0x01
#define CV_STATUS_VALUE		0x02
#define CV_PIN_REPLACEMENT	0x04
#define CV_COPY_VALUE		0x08
#define CV_EXT_STATUS		0x10

/* ModifyConfiguration */
typedef struct modconf_t {
    u_int	Attributes;
    u_int	Vcc, Vpp1, Vpp2;
} modconf_t;

/* Attributes for ModifyConfiguration */
#define CONF_IRQ_CHANGE_VALID	0x0100
#define CONF_VCC_CHANGE_VALID	0x0200
#define CONF_VPP1_CHANGE_VALID	0x0400
#define CONF_VPP2_CHANGE_VALID	0x0800
#define CONF_IO_CHANGE_WIDTH	0x1000

/* For RequestConfiguration */
typedef struct config_req_t {
    u_int	Attributes;
    u_int	Vpp; /* both Vpp1 and Vpp2 */
    u_int	IntType;
    u_int	ConfigBase;
    u_char	Status, Pin, Copy, ExtStatus;
    u_char	ConfigIndex;
    u_int	Present;
} config_req_t;

/* Attributes for RequestConfiguration */
#define CONF_ENABLE_IRQ		0x01
#define CONF_ENABLE_DMA		0x02
#define CONF_ENABLE_SPKR	0x04
#define CONF_ENABLE_PULSE_IRQ	0x08
#define CONF_VALID_CLIENT	0x100

/* IntType field */
#define INT_MEMORY		0x01
#define INT_MEMORY_AND_IO	0x02
#define INT_CARDBUS		0x04
#define INT_ZOOMED_VIDEO	0x08

/* For RequestIO and ReleaseIO */
typedef struct io_req_t {
    u_int	BasePort1;
    u_int	NumPorts1;
    u_int	Attributes1;
    u_int	BasePort2;
    u_int	NumPorts2;
    u_int	Attributes2;
    u_int	IOAddrLines;
} io_req_t;

/* Attributes for RequestIO and ReleaseIO */
#define IO_SHARED		0x01
#define IO_FIRST_SHARED		0x02
#define IO_FORCE_ALIAS_ACCESS	0x04
#define IO_DATA_PATH_WIDTH	0x18
#define IO_DATA_PATH_WIDTH_8	0x00
#define IO_DATA_PATH_WIDTH_16	0x08
#define IO_DATA_PATH_WIDTH_AUTO	0x10

/* Bits in IRQInfo1 field */
#define IRQ_NMI_ID		0x01
#define IRQ_IOCK_ID		0x02
#define IRQ_BERR_ID		0x04
#define IRQ_VEND_ID		0x08
#define IRQ_INFO2_VALID		0x10
#define IRQ_LEVEL_ID		0x20
#define IRQ_PULSE_ID		0x40
#define IRQ_SHARE_ID		0x80

/* Configuration registers present */
#define PRESENT_OPTION		0x001
#define PRESENT_STATUS		0x002
#define PRESENT_PIN_REPLACE	0x004
#define PRESENT_COPY		0x008
#define PRESENT_EXT_STATUS	0x010
#define PRESENT_IOBASE_0	0x020
#define PRESENT_IOBASE_1	0x040
#define PRESENT_IOBASE_2	0x080
#define PRESENT_IOBASE_3	0x100
#define PRESENT_IOSIZE		0x200

/* For GetMemPage, MapMemPage */
typedef struct memreq_t {
    u_int	CardOffset;
    u_short	Page;
} memreq_t;

/* For ModifyWindow */
typedef struct modwin_t {
    u_int	Attributes;
    u_int	AccessSpeed;
} modwin_t;

/* For RequestWindow */
typedef struct win_req_t {
    u_int	Attributes;
    u_long	Base;
    u_int	Size;
    u_int	AccessSpeed;
} win_req_t;

/* Attributes for RequestWindow */
#define WIN_ADDR_SPACE		0x0001
#define WIN_ADDR_SPACE_MEM	0x0000
#define WIN_ADDR_SPACE_IO	0x0001
#define WIN_MEMORY_TYPE		0x0002
#define WIN_MEMORY_TYPE_CM	0x0000
#define WIN_MEMORY_TYPE_AM	0x0002
#define WIN_ENABLE		0x0004
#define WIN_DATA_WIDTH		0x0018
#define WIN_DATA_WIDTH_8	0x0000
#define WIN_DATA_WIDTH_16	0x0008
#define WIN_DATA_WIDTH_32	0x0010
#define WIN_PAGED		0x0020
#define WIN_SHARED		0x0040
#define WIN_FIRST_SHARED	0x0080
#define WIN_USE_WAIT		0x0100
#define WIN_STRICT_ALIGN	0x0200
#define WIN_MAP_BELOW_1MB	0x0400
#define WIN_PREFETCH		0x0800
#define WIN_CACHEABLE		0x1000
#define WIN_BAR_MASK		0xe000
#define WIN_BAR_SHIFT		13

/* Flag to bind to all functions */
#define BIND_FN_ALL	0xff

#endif /* _LINUX_CS_H */
