/*
 *  PS3 'Other OS' area data.
 *
 *  Copyright (C) 2006 Sony Computer Entertainment Inc.
 *  Copyright 2006 Sony Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/io.h>

#include <asm/lmb.h>
#include <asm/ps3.h>

#include "platform.h"

enum {
	OS_AREA_SEGMENT_SIZE = 0X200,
};

enum {
	HEADER_LDR_FORMAT_RAW = 0,
	HEADER_LDR_FORMAT_GZIP = 1,
};

/**
 * struct os_area_header - os area header segment.
 * @magic_num: Always 'cell_ext_os_area'.
 * @hdr_version: Header format version number.
 * @os_area_offset: Starting segment number of os image area.
 * @ldr_area_offset: Starting segment number of bootloader image area.
 * @ldr_format: HEADER_LDR_FORMAT flag.
 * @ldr_size: Size of bootloader image in bytes.
 *
 * Note that the docs refer to area offsets.  These are offsets in units of
 * segments from the start of the os area (top of the header).  These are
 * better thought of as segment numbers.  The os area of the os area is
 * reserved for the os image.
 */

struct os_area_header {
	s8 magic_num[16];
	u32 hdr_version;
	u32 os_area_offset;
	u32 ldr_area_offset;
	u32 _reserved_1;
	u32 ldr_format;
	u32 ldr_size;
	u32 _reserved_2[6];
};

enum {
	PARAM_BOOT_FLAG_GAME_OS = 0,
	PARAM_BOOT_FLAG_OTHER_OS = 1,
};

enum {
	PARAM_AV_MULTI_OUT_NTSC = 0,
	PARAM_AV_MULTI_OUT_PAL_RGB = 1,
	PARAM_AV_MULTI_OUT_PAL_YCBCR = 2,
	PARAM_AV_MULTI_OUT_SECAM = 3,
};

enum {
	PARAM_CTRL_BUTTON_O_IS_YES = 0,
	PARAM_CTRL_BUTTON_X_IS_YES = 1,
};

/**
 * struct os_area_params - os area params segment.
 * @boot_flag: User preference of operating system, PARAM_BOOT_FLAG flag.
 * @num_params: Number of params in this (params) segment.
 * @rtc_diff: Difference in seconds between 1970 and the ps3 rtc value.
 * @av_multi_out: User preference of AV output, PARAM_AV_MULTI_OUT flag.
 * @ctrl_button: User preference of controller button config, PARAM_CTRL_BUTTON
 *	flag.
 * @static_ip_addr: User preference of static IP address.
 * @network_mask: User preference of static network mask.
 * @default_gateway: User preference of static default gateway.
 * @dns_primary: User preference of static primary dns server.
 * @dns_secondary: User preference of static secondary dns server.
 *
 * User preference of zero for static_ip_addr means use dhcp.
 */

struct os_area_params {
	u32 boot_flag;
	u32 _reserved_1[3];
	u32 num_params;
	u32 _reserved_2[3];
	/* param 0 */
	s64 rtc_diff;
	u8 av_multi_out;
	u8 ctrl_button;
	u8 _reserved_3[6];
	/* param 1 */
	u8 static_ip_addr[4];
	u8 network_mask[4];
	u8 default_gateway[4];
	u8 _reserved_4[4];
	/* param 2 */
	u8 dns_primary[4];
	u8 dns_secondary[4];
	u8 _reserved_5[8];
};

/**
 * struct saved_params - Static working copies of data from the 'Other OS' area.
 *
 * For the convinience of the guest, the HV makes a copy of the 'Other OS' area
 * in flash to a high address in the boot memory region and then puts that RAM
 * address and the byte count into the repository for retreval by the guest.
 * We copy the data we want into a static variable and allow the memory setup
 * by the HV to be claimed by the lmb manager.
 */

struct saved_params {
	/* param 0 */
	s64 rtc_diff;
	unsigned int av_multi_out;
	unsigned int ctrl_button;
	/* param 1 */
	u8 static_ip_addr[4];
	u8 network_mask[4];
	u8 default_gateway[4];
	/* param 2 */
	u8 dns_primary[4];
	u8 dns_secondary[4];
} static saved_params;

#define dump_header(_a) _dump_header(_a, __func__, __LINE__)
static void _dump_header(const struct os_area_header __iomem *h, const char* func,
	int line)
{
	pr_debug("%s:%d: h.magic_num:         '%s'\n", func, line,
		h->magic_num);
	pr_debug("%s:%d: h.hdr_version:       %u\n", func, line,
		h->hdr_version);
	pr_debug("%s:%d: h.os_area_offset:   %u\n", func, line,
		h->os_area_offset);
	pr_debug("%s:%d: h.ldr_area_offset: %u\n", func, line,
		h->ldr_area_offset);
	pr_debug("%s:%d: h.ldr_format:        %u\n", func, line,
		h->ldr_format);
	pr_debug("%s:%d: h.ldr_size:          %xh\n", func, line,
		h->ldr_size);
}

#define dump_params(_a) _dump_params(_a, __func__, __LINE__)
static void _dump_params(const struct os_area_params __iomem *p, const char* func,
	int line)
{
	pr_debug("%s:%d: p.boot_flag:       %u\n", func, line, p->boot_flag);
	pr_debug("%s:%d: p.num_params:      %u\n", func, line, p->num_params);
	pr_debug("%s:%d: p.rtc_diff         %ld\n", func, line, p->rtc_diff);
	pr_debug("%s:%d: p.av_multi_out     %u\n", func, line, p->av_multi_out);
	pr_debug("%s:%d: p.ctrl_button:     %u\n", func, line, p->ctrl_button);
	pr_debug("%s:%d: p.static_ip_addr:  %u.%u.%u.%u\n", func, line,
		p->static_ip_addr[0], p->static_ip_addr[1],
		p->static_ip_addr[2], p->static_ip_addr[3]);
	pr_debug("%s:%d: p.network_mask:    %u.%u.%u.%u\n", func, line,
		p->network_mask[0], p->network_mask[1],
		p->network_mask[2], p->network_mask[3]);
	pr_debug("%s:%d: p.default_gateway: %u.%u.%u.%u\n", func, line,
		p->default_gateway[0], p->default_gateway[1],
		p->default_gateway[2], p->default_gateway[3]);
	pr_debug("%s:%d: p.dns_primary:     %u.%u.%u.%u\n", func, line,
		p->dns_primary[0], p->dns_primary[1],
		p->dns_primary[2], p->dns_primary[3]);
	pr_debug("%s:%d: p.dns_secondary:   %u.%u.%u.%u\n", func, line,
		p->dns_secondary[0], p->dns_secondary[1],
		p->dns_secondary[2], p->dns_secondary[3]);
}

static int __init verify_header(const struct os_area_header *header)
{
	if (memcmp(header->magic_num, "cell_ext_os_area", 16)) {
		pr_debug("%s:%d magic_num failed\n", __func__, __LINE__);
		return -1;
	}

	if (header->hdr_version < 1) {
		pr_debug("%s:%d hdr_version failed\n", __func__, __LINE__);
		return -1;
	}

	if (header->os_area_offset > header->ldr_area_offset) {
		pr_debug("%s:%d offsets failed\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int __init ps3_os_area_init(void)
{
	int result;
	u64 lpar_addr;
	unsigned int size;
	struct os_area_header *header;
	struct os_area_params *params;

	result = ps3_repository_read_boot_dat_info(&lpar_addr, &size);

	if (result) {
		pr_debug("%s:%d ps3_repository_read_boot_dat_info failed\n",
			__func__, __LINE__);
		return result;
	}

	header = (struct os_area_header *)__va(lpar_addr);
	params = (struct os_area_params *)__va(lpar_addr + OS_AREA_SEGMENT_SIZE);

	result = verify_header(header);

	if (result) {
		pr_debug("%s:%d verify_header failed\n", __func__, __LINE__);
		dump_header(header);
		return -EIO;
	}

	dump_header(header);
	dump_params(params);

	saved_params.rtc_diff = params->rtc_diff;
	saved_params.av_multi_out = params->av_multi_out;
	saved_params.ctrl_button = params->ctrl_button;
	memcpy(saved_params.static_ip_addr, params->static_ip_addr, 4);
	memcpy(saved_params.network_mask, params->network_mask, 4);
	memcpy(saved_params.default_gateway, params->default_gateway, 4);
	memcpy(saved_params.dns_secondary, params->dns_secondary, 4);

	return result;
}

/**
 * ps3_os_area_rtc_diff - Returns the ps3 rtc diff value.
 *
 * The ps3 rtc maintains a value that approximates seconds since
 * 2000-01-01 00:00:00 UTC.  Returns the exact number of seconds from 1970 to
 * 2000 when saved_params.rtc_diff has not been properly set up.
 */

u64 ps3_os_area_rtc_diff(void)
{
	return saved_params.rtc_diff ? saved_params.rtc_diff : 946684800UL;
}
