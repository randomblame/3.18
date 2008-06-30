/*
 *		Pixart PAC7311 library
 *		Copyright (C) 2005 Thomas Kaiser thomas@kaiser-linux.li
 *
 * V4L2 by Jean-Francois Moine <http://moinejf.free.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#define MODULE_NAME "pac7311"

#include "gspca.h"

#define DRIVER_VERSION_NUMBER	KERNEL_VERSION(2, 1, 0)
static const char version[] = "2.1.0";

MODULE_AUTHOR("Thomas Kaiser thomas@kaiser-linux.li");
MODULE_DESCRIPTION("Pixart PAC7311");
MODULE_LICENSE("GPL");

/* specific webcam descriptor */
struct sd {
	struct gspca_dev gspca_dev;		/* !! must be the first item */

	int avg_lum;

	unsigned char brightness;
#define BRIGHTNESS_MAX 0x20
	unsigned char contrast;
	unsigned char colors;
	unsigned char autogain;

	char ffseq;
	signed char ag_cnt;
#define AG_CNT_START 13
};

/* V4L2 controls supported by the driver */
static int sd_setbrightness(struct gspca_dev *gspca_dev, __s32 val);
static int sd_getbrightness(struct gspca_dev *gspca_dev, __s32 *val);
static int sd_setcontrast(struct gspca_dev *gspca_dev, __s32 val);
static int sd_getcontrast(struct gspca_dev *gspca_dev, __s32 *val);
static int sd_setcolors(struct gspca_dev *gspca_dev, __s32 val);
static int sd_getcolors(struct gspca_dev *gspca_dev, __s32 *val);
static int sd_setautogain(struct gspca_dev *gspca_dev, __s32 val);
static int sd_getautogain(struct gspca_dev *gspca_dev, __s32 *val);

static struct ctrl sd_ctrls[] = {
#define SD_BRIGHTNESS 0
	{
	    {
		.id      = V4L2_CID_BRIGHTNESS,
		.type    = V4L2_CTRL_TYPE_INTEGER,
		.name    = "Brightness",
		.minimum = 0,
		.maximum = BRIGHTNESS_MAX,
		.step    = 1,
		.default_value = 0x10,
	    },
	    .set = sd_setbrightness,
	    .get = sd_getbrightness,
	},
#define SD_CONTRAST 1
	{
	    {
		.id      = V4L2_CID_CONTRAST,
		.type    = V4L2_CTRL_TYPE_INTEGER,
		.name    = "Contrast",
		.minimum = 0,
		.maximum = 255,
		.step    = 1,
		.default_value = 127,
	    },
	    .set = sd_setcontrast,
	    .get = sd_getcontrast,
	},
#define SD_COLOR 2
	{
	    {
		.id      = V4L2_CID_SATURATION,
		.type    = V4L2_CTRL_TYPE_INTEGER,
		.name    = "Color",
		.minimum = 0,
		.maximum = 255,
		.step    = 1,
		.default_value = 127,
	    },
	    .set = sd_setcolors,
	    .get = sd_getcolors,
	},
#define SD_AUTOGAIN 3
	{
	    {
		.id      = V4L2_CID_AUTOGAIN,
		.type    = V4L2_CTRL_TYPE_BOOLEAN,
		.name    = "Auto Gain",
		.minimum = 0,
		.maximum = 1,
		.step    = 1,
		.default_value = 1,
	    },
	    .set = sd_setautogain,
	    .get = sd_getautogain,
	},
};

static struct cam_mode vga_mode[] = {
	{V4L2_PIX_FMT_JPEG, 160, 120, 2},
	{V4L2_PIX_FMT_JPEG, 320, 240, 1},
	{V4L2_PIX_FMT_JPEG, 640, 480, 0},
};

#define PAC7311_JPEG_HEADER_SIZE (sizeof pac7311_jpeg_header)	/* (594) */

const unsigned char pac7311_jpeg_header[] = {
	0xff, 0xd8,
	0xff, 0xe0, 0x00, 0x03, 0x20,
	0xff, 0xc0, 0x00, 0x11, 0x08,
		0x01, 0xe0,			/* 12: height */
		0x02, 0x80,			/* 14: width */
		0x03,				/* 16 */
			0x01, 0x21, 0x00,
			0x02, 0x11, 0x01,
			0x03, 0x11, 0x01,
	0xff, 0xdb, 0x00, 0x84,
	0x00, 0x10, 0x0b, 0x0c, 0x0e, 0x0c, 0x0a, 0x10, 0x0e, 0x0d,
	0x0e, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1a, 0x18, 0x16,
	0x16, 0x18, 0x31, 0x23, 0x25, 0x1d, 0x28, 0x3a, 0x33, 0x3d,
	0x3c, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5c, 0x4e, 0x40,
	0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6d, 0x51, 0x57, 0x5f,
	0x62, 0x67, 0x68, 0x67, 0x3e, 0x4d, 0x71, 0x79, 0x70, 0x64,
	0x78, 0x5c, 0x65, 0x67, 0x63, 0x01, 0x11, 0x12, 0x12, 0x18,
	0x15, 0x18, 0x2f, 0x1a, 0x1a, 0x2f, 0x63, 0x42, 0x38, 0x42,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0a, 0x0b, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02,
	0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31,
	0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32,
	0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52,
	0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
	0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
	0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
	0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
	0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
	0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01,
	0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
	0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14,
	0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25,
	0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a,
	0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46,
	0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
	0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
	0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
	0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa,
	0xff, 0xda, 0x00, 0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03,
	0x11, 0x00, 0x3f, 0x00
};

static void reg_w(struct usb_device *dev,
			    __u16 req,
			    __u16 value,
			    __u16 index,
			    __u8 *buffer, __u16 length)
{
	usb_control_msg(dev,
			usb_sndctrlpipe(dev, 0),
			req,
			USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			value, index, buffer, length,
			500);
}

static void pac7311_reg_read(struct usb_device *dev, __u16 index,
			    __u8 *buffer)
{
	usb_control_msg(dev,
			usb_rcvctrlpipe(dev, 0),
			0,			/* request */
			USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			0,			/* value */
			index, buffer, 1,
			500);
}

static void pac7311_reg_write(struct usb_device *dev,
			      __u16 index,
			      __u8 value)
{
	__u8 buf;

	buf = value;
	reg_w(dev, 0x00, value, index, &buf, 1);
}

/* this function is called at probe time */
static int sd_config(struct gspca_dev *gspca_dev,
			const struct usb_device_id *id)
{
	struct sd *sd = (struct sd *) gspca_dev;
	struct usb_device *dev = gspca_dev->dev;
	struct cam *cam;

	PDEBUG(D_CONF, "Find Sensor PAC7311");
	pac7311_reg_write(dev, 0x78, 0x40); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0x78, 0x40); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0x78, 0x44); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0xff, 0x04);
	pac7311_reg_write(dev, 0x27, 0x80);
	pac7311_reg_write(dev, 0x28, 0xca);
	pac7311_reg_write(dev, 0x29, 0x53);
	pac7311_reg_write(dev, 0x2a, 0x0e);
	pac7311_reg_write(dev, 0xff, 0x01);
	pac7311_reg_write(dev, 0x3e, 0x20);

	cam = &gspca_dev->cam;
	cam->dev_name = (char *) id->driver_info;
	cam->epaddr = 0x05;
	cam->cam_mode = vga_mode;
	cam->nmodes = ARRAY_SIZE(vga_mode);

	sd->brightness = sd_ctrls[SD_BRIGHTNESS].qctrl.default_value;
	sd->contrast = sd_ctrls[SD_CONTRAST].qctrl.default_value;
	sd->colors = sd_ctrls[SD_COLOR].qctrl.default_value;
	sd->autogain = sd_ctrls[SD_AUTOGAIN].qctrl.default_value;
	return 0;
}

static void setbrightness(struct gspca_dev *gspca_dev)
{
	struct sd *sd = (struct sd *) gspca_dev;
	int brightness;

/*jfm: inverted?*/
	brightness = BRIGHTNESS_MAX - sd->brightness;
	pac7311_reg_write(gspca_dev->dev, 0xff, 0x04);
	/* pac7311_reg_write(gspca_dev->dev, 0x0e, 0x00); */
	pac7311_reg_write(gspca_dev->dev, 0x0f, brightness);
	/* load registers to sensor (Bit 0, auto clear) */
	pac7311_reg_write(gspca_dev->dev, 0x11, 0x01);
	PDEBUG(D_CONF|D_STREAM, "brightness: %i", brightness);
}

static void setcontrast(struct gspca_dev *gspca_dev)
{
	struct sd *sd = (struct sd *) gspca_dev;

	pac7311_reg_write(gspca_dev->dev, 0xff, 0x01);
	pac7311_reg_write(gspca_dev->dev, 0x80, sd->contrast);
	/* load registers to sensor (Bit 0, auto clear) */
	pac7311_reg_write(gspca_dev->dev, 0x11, 0x01);
	PDEBUG(D_CONF|D_STREAM, "contrast: %i", sd->contrast);
}

static void setcolors(struct gspca_dev *gspca_dev)
{
	struct sd *sd = (struct sd *) gspca_dev;

	pac7311_reg_write(gspca_dev->dev, 0xff, 0x01);
	pac7311_reg_write(gspca_dev->dev, 0x10, sd->colors);
	/* load registers to sensor (Bit 0, auto clear) */
	pac7311_reg_write(gspca_dev->dev, 0x11, 0x01);
	PDEBUG(D_CONF|D_STREAM, "color: %i", sd->colors);
}

/* this function is called at open time */
static int sd_open(struct gspca_dev *gspca_dev)
{
	pac7311_reg_write(gspca_dev->dev, 0x78, 0x00);	/* Turn on LED */
	return 0;
}

static void sd_start(struct gspca_dev *gspca_dev)
{
	struct usb_device *dev = gspca_dev->dev;
	struct sd *sd = (struct sd *) gspca_dev;

	pac7311_reg_write(dev, 0xff, 0x01);
	reg_w(dev, 0x01, 0, 0x0002, "\x48\x0a\x40\x08\x00\x00\x08\x00", 8);
	reg_w(dev, 0x01, 0, 0x000a, "\x06\xff\x11\xff\x5a\x30\x90\x4c", 8);
	reg_w(dev, 0x01, 0, 0x0012, "\x00\x07\x00\x0a\x10\x00\xa0\x10", 8);
	reg_w(dev, 0x01, 0, 0x001a, "\x02\x00\x00\x00\x00\x0b\x01\x00", 8);
	reg_w(dev, 0x01, 0, 0x0022, "\x00\x00\x00\x00\x00\x00\x00\x00", 8);
	reg_w(dev, 0x01, 0, 0x002a, "\x00\x00\x00", 3);
	reg_w(dev, 0x01, 0, 0x003e, "\x00\x00\x78\x52\x4a\x52\x78\x6e", 8);
	reg_w(dev, 0x01, 0, 0x0046, "\x48\x46\x48\x6e\x5f\x49\x42\x49", 8);
	reg_w(dev, 0x01, 0, 0x004e, "\x5f\x5f\x49\x42\x49\x5f\x6e\x48", 8);
	reg_w(dev, 0x01, 0, 0x0056, "\x46\x48\x6e\x78\x52\x4a\x52\x78", 8);
	reg_w(dev, 0x01, 0, 0x005e, "\x00\x00\x09\x1b\x34\x49\x5c\x9b", 8);
	reg_w(dev, 0x01, 0, 0x0066, "\xd0\xff", 2);
	reg_w(dev, 0x01, 0, 0x0078, "\x44\x00\xf2\x01\x01\x80", 6);
	reg_w(dev, 0x01, 0, 0x007f, "\x2a\x1c\x00\xc8\x02\x58\x03\x84", 8);
	reg_w(dev, 0x01, 0, 0x0087, "\x12\x00\x1a\x04\x08\x0c\x10\x14", 8);
	reg_w(dev, 0x01, 0, 0x008f, "\x18\x20", 2);
	reg_w(dev, 0x01, 0, 0x0096, "\x01\x08\x04", 3);
	reg_w(dev, 0x01, 0, 0x00a0, "\x44\x44\x44\x04", 4);
	reg_w(dev, 0x01, 0, 0x00f0, "\x01\x00\x00\x00\x22\x00\x20\x00", 8);
	reg_w(dev, 0x01, 0, 0x00f8, "\x3f\x00\x0a\x01\x00", 5);

	pac7311_reg_write(dev, 0xff, 0x04);
	pac7311_reg_write(dev, 0x02, 0x04);
	pac7311_reg_write(dev, 0x03, 0x54);
	pac7311_reg_write(dev, 0x04, 0x07);
	pac7311_reg_write(dev, 0x05, 0x2b);
	pac7311_reg_write(dev, 0x06, 0x09);
	pac7311_reg_write(dev, 0x07, 0x0f);
	pac7311_reg_write(dev, 0x08, 0x09);
	pac7311_reg_write(dev, 0x09, 0x00);
	pac7311_reg_write(dev, 0x0c, 0x07);
	pac7311_reg_write(dev, 0x0d, 0x00);
	pac7311_reg_write(dev, 0x0e, 0x00);
	pac7311_reg_write(dev, 0x0f, 0x62);
	pac7311_reg_write(dev, 0x10, 0x08);
	pac7311_reg_write(dev, 0x12, 0x07);
	pac7311_reg_write(dev, 0x13, 0x00);
	pac7311_reg_write(dev, 0x14, 0x00);
	pac7311_reg_write(dev, 0x15, 0x00);
	pac7311_reg_write(dev, 0x16, 0x00);
	pac7311_reg_write(dev, 0x17, 0x00);
	pac7311_reg_write(dev, 0x18, 0x00);
	pac7311_reg_write(dev, 0x19, 0x00);
	pac7311_reg_write(dev, 0x1a, 0x00);
	pac7311_reg_write(dev, 0x1b, 0x03);
	pac7311_reg_write(dev, 0x1c, 0xa0);
	pac7311_reg_write(dev, 0x1d, 0x01);
	pac7311_reg_write(dev, 0x1e, 0xf4);
	pac7311_reg_write(dev, 0x21, 0x00);
	pac7311_reg_write(dev, 0x22, 0x08);
	pac7311_reg_write(dev, 0x24, 0x03);
	pac7311_reg_write(dev, 0x26, 0x00);
	pac7311_reg_write(dev, 0x27, 0x01);
	pac7311_reg_write(dev, 0x28, 0xca);
	pac7311_reg_write(dev, 0x29, 0x10);
	pac7311_reg_write(dev, 0x2a, 0x06);
	pac7311_reg_write(dev, 0x2b, 0x78);
	pac7311_reg_write(dev, 0x2c, 0x00);
	pac7311_reg_write(dev, 0x2d, 0x00);
	pac7311_reg_write(dev, 0x2e, 0x00);
	pac7311_reg_write(dev, 0x2f, 0x00);
	pac7311_reg_write(dev, 0x30, 0x23);
	pac7311_reg_write(dev, 0x31, 0x28);
	pac7311_reg_write(dev, 0x32, 0x04);
	pac7311_reg_write(dev, 0x33, 0x11);
	pac7311_reg_write(dev, 0x34, 0x00);
	pac7311_reg_write(dev, 0x35, 0x00);
	pac7311_reg_write(dev, 0x11, 0x01);
	setcontrast(gspca_dev);
	setbrightness(gspca_dev);
	setcolors(gspca_dev);

	/* set correct resolution */
	switch (gspca_dev->cam.cam_mode[(int) gspca_dev->curr_mode].mode) {
	case 2:					/* 160x120 */
		pac7311_reg_write(dev, 0xff, 0x04);
		pac7311_reg_write(dev, 0x02, 0x03);
		pac7311_reg_write(dev, 0xff, 0x01);
		pac7311_reg_write(dev, 0x08, 0x09);
		pac7311_reg_write(dev, 0x17, 0x20);
		pac7311_reg_write(dev, 0x1b, 0x00);
/*		pac7311_reg_write(dev, 0x80, 0x69); */
		pac7311_reg_write(dev, 0x87, 0x10);
		break;
	case 1:					/* 320x240 */
		pac7311_reg_write(dev, 0xff, 0x04);
		pac7311_reg_write(dev, 0x02, 0x03);
		pac7311_reg_write(dev, 0xff, 0x01);
		pac7311_reg_write(dev, 0x08, 0x09);
		pac7311_reg_write(dev, 0x17, 0x30);
/*		pac7311_reg_write(dev, 0x80, 0x3f); */
		pac7311_reg_write(dev, 0x87, 0x11);
		break;
	case 0:					/* 640x480 */
		pac7311_reg_write(dev, 0xff, 0x04);
		pac7311_reg_write(dev, 0x02, 0x03);
		pac7311_reg_write(dev, 0xff, 0x01);
		pac7311_reg_write(dev, 0x08, 0x08);
		pac7311_reg_write(dev, 0x17, 0x00);
/*		pac7311_reg_write(dev, 0x80, 0x1c); */
		pac7311_reg_write(dev, 0x87, 0x12);
		break;
	}

	/* start stream */
	pac7311_reg_write(dev, 0xff, 0x01);
	pac7311_reg_write(dev, 0x78, 0x04);
	pac7311_reg_write(dev, 0x78, 0x05);

	if (sd->autogain) {
		sd->ag_cnt = AG_CNT_START;
		sd->avg_lum = 0;
	} else {
		sd->ag_cnt = -1;
	}
}

static void sd_stopN(struct gspca_dev *gspca_dev)
{
	struct usb_device *dev = gspca_dev->dev;

	pac7311_reg_write(dev, 0xff, 0x04);
	pac7311_reg_write(dev, 0x27, 0x80);
	pac7311_reg_write(dev, 0x28, 0xca);
	pac7311_reg_write(dev, 0x29, 0x53);
	pac7311_reg_write(dev, 0x2a, 0x0e);
	pac7311_reg_write(dev, 0xff, 0x01);
	pac7311_reg_write(dev, 0x3e, 0x20);
	pac7311_reg_write(dev, 0x78, 0x04); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0x78, 0x44); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0x78, 0x44); /* Bit_0=start stream, Bit_7=LED */
}

static void sd_stop0(struct gspca_dev *gspca_dev)
{
}

/* this function is called at close time */
static void sd_close(struct gspca_dev *gspca_dev)
{
	struct usb_device *dev = gspca_dev->dev;

	pac7311_reg_write(dev, 0xff, 0x04);
	pac7311_reg_write(dev, 0x27, 0x80);
	pac7311_reg_write(dev, 0x28, 0xca);
	pac7311_reg_write(dev, 0x29, 0x53);
	pac7311_reg_write(dev, 0x2a, 0x0e);
	pac7311_reg_write(dev, 0xff, 0x01);
	pac7311_reg_write(dev, 0x3e, 0x20);
	pac7311_reg_write(dev, 0x78, 0x04); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0x78, 0x44); /* Bit_0=start stream, Bit_7=LED */
	pac7311_reg_write(dev, 0x78, 0x44); /* Bit_0=start stream, Bit_7=LED */
}

static void setautogain(struct gspca_dev *gspca_dev, int luma)
{
	int luma_mean = 128;
	int luma_delta = 20;
	__u8 spring = 5;
	__u8 Pxclk;
	int Gbright;

	pac7311_reg_read(gspca_dev->dev, 0x02, &Pxclk);
	Gbright = Pxclk;
	PDEBUG(D_FRAM, "luma mean %d", luma);
	if (luma < luma_mean - luma_delta ||
	    luma > luma_mean + luma_delta) {
		Gbright += (luma_mean - luma) >> spring;
		if (Gbright > 0x1a)
			Gbright = 0x1a;
		else if (Gbright < 4)
			Gbright = 4;
		PDEBUG(D_FRAM, "gbright %d", Gbright);
		pac7311_reg_write(gspca_dev->dev, 0xff, 0x04);
		pac7311_reg_write(gspca_dev->dev, 0x0f, Gbright);
		/* load registers to sensor (Bit 0, auto clear) */
		pac7311_reg_write(gspca_dev->dev, 0x11, 0x01);
	}
}

static void sd_pkt_scan(struct gspca_dev *gspca_dev,
			struct gspca_frame *frame,	/* target */
			unsigned char *data,		/* isoc packet */
			int len)			/* iso packet length */
{
	struct sd *sd = (struct sd *) gspca_dev;
	unsigned char tmpbuf[4];
	int i, p, ffseq;

/*	if (len < 5) { */
	if (len < 6) {
/*		gspca_dev->last_packet_type = DISCARD_PACKET; */
		return;
	}

	ffseq = sd->ffseq;

	for (p = 0; p < len - 6; p++) {
		if ((data[0 + p] == 0xff)
		    && (data[1 + p] == 0xff)
		    && (data[2 + p] == 0x00)
		    && (data[3 + p] == 0xff)
		    && (data[4 + p] == 0x96)) {

			/* start of frame */
			if (sd->ag_cnt >= 0 && p > 28) {
				sd->avg_lum += data[p - 23];
				if (--sd->ag_cnt < 0) {
					sd->ag_cnt = AG_CNT_START;
					setautogain(gspca_dev,
						sd->avg_lum / AG_CNT_START);
					sd->avg_lum = 0;
				}
			}

			/* copy the end of data to the current frame */
			frame = gspca_frame_add(gspca_dev, LAST_PACKET, frame,
						data, p);

			/* put the JPEG header in the new frame */
			gspca_frame_add(gspca_dev, FIRST_PACKET, frame,
					(unsigned char *) pac7311_jpeg_header,
					12);
			tmpbuf[0] = gspca_dev->height >> 8;
			tmpbuf[1] = gspca_dev->height & 0xff;
			tmpbuf[2] = gspca_dev->width >> 8;
			tmpbuf[3] = gspca_dev->width & 0xff;
			gspca_frame_add(gspca_dev, INTER_PACKET, frame,
					tmpbuf, 4);
			gspca_frame_add(gspca_dev, INTER_PACKET, frame,
				(unsigned char *) &pac7311_jpeg_header[16],
				PAC7311_JPEG_HEADER_SIZE - 16);

			data += p + 7;
			len -= p + 7;
			ffseq = 0;
			break;
		}
	}

	/* remove the 'ff ff ff xx' sequences */
	switch (ffseq) {
	case 3:
		data += 1;
		len -= 1;
		break;
	case 2:
		if (data[0] == 0xff) {
			data += 2;
			len -= 2;
			frame->data_end -= 2;
		}
		break;
	case 1:
		if (data[0] == 0xff
		    && data[1] == 0xff) {
			data += 3;
			len -= 3;
			frame->data_end -= 1;
		}
		break;
	}
	for (i = 0; i < len - 4; i++) {
		if (data[i] == 0xff
		    && data[i + 1] == 0xff
		    && data[i + 2] == 0xff) {
			memmove(&data[i], &data[i + 4], len - i - 4);
			len -= 4;
		}
	}
	ffseq = 0;
	if (data[len - 4] == 0xff) {
		if (data[len - 3] == 0xff
		    && data[len - 2] == 0xff) {
			len -= 4;
		}
	} else if (data[len - 3] == 0xff) {
		if (data[len - 2] == 0xff
		    && data[len - 1] == 0xff)
			ffseq = 3;
	} else if (data[len - 2] == 0xff) {
		if (data[len - 1] == 0xff)
			ffseq = 2;
	} else if (data[len - 1] == 0xff)
		ffseq = 1;
	sd->ffseq = ffseq;
	gspca_frame_add(gspca_dev, INTER_PACKET, frame, data, len);
}

static void getbrightness(struct gspca_dev *gspca_dev)
{
/*	__u8 brightness = 0;

	pac7311_reg_read(gspca_dev->dev, 0x0008, &brightness);
	spca50x->brightness = brightness;
	return spca50x->brightness;	*/
/*	PDEBUG(D_CONF, "Called pac7311_getbrightness: Not implemented yet"); */
}



static int sd_setbrightness(struct gspca_dev *gspca_dev, __s32 val)
{
	struct sd *sd = (struct sd *) gspca_dev;

	sd->brightness = val;
	if (gspca_dev->streaming)
		setbrightness(gspca_dev);
	return 0;
}

static int sd_getbrightness(struct gspca_dev *gspca_dev, __s32 *val)
{
	struct sd *sd = (struct sd *) gspca_dev;

	getbrightness(gspca_dev);
	*val = sd->brightness;
	return 0;
}

static int sd_setcontrast(struct gspca_dev *gspca_dev, __s32 val)
{
	struct sd *sd = (struct sd *) gspca_dev;

	sd->contrast = val;
	if (gspca_dev->streaming)
		setcontrast(gspca_dev);
	return 0;
}

static int sd_getcontrast(struct gspca_dev *gspca_dev, __s32 *val)
{
	struct sd *sd = (struct sd *) gspca_dev;

/*	getcontrast(gspca_dev); */
	*val = sd->contrast;
	return 0;
}

static int sd_setcolors(struct gspca_dev *gspca_dev, __s32 val)
{
	struct sd *sd = (struct sd *) gspca_dev;

	sd->colors = val;
	if (gspca_dev->streaming)
		setcolors(gspca_dev);
	return 0;
}

static int sd_getcolors(struct gspca_dev *gspca_dev, __s32 *val)
{
	struct sd *sd = (struct sd *) gspca_dev;

/*	getcolors(gspca_dev); */
	*val = sd->colors;
	return 0;
}

static int sd_setautogain(struct gspca_dev *gspca_dev, __s32 val)
{
	struct sd *sd = (struct sd *) gspca_dev;

	sd->autogain = val;
	if (val) {
		sd->ag_cnt = AG_CNT_START;
		sd->avg_lum = 0;
	} else {
		sd->ag_cnt = -1;
	}
	return 0;
}

static int sd_getautogain(struct gspca_dev *gspca_dev, __s32 *val)
{
	struct sd *sd = (struct sd *) gspca_dev;

	*val = sd->autogain;
	return 0;
}

/* sub-driver description */
static struct sd_desc sd_desc = {
	.name = MODULE_NAME,
	.ctrls = sd_ctrls,
	.nctrls = ARRAY_SIZE(sd_ctrls),
	.config = sd_config,
	.open = sd_open,
	.start = sd_start,
	.stopN = sd_stopN,
	.stop0 = sd_stop0,
	.close = sd_close,
	.pkt_scan = sd_pkt_scan,
};

/* -- module initialisation -- */
#define DVNM(name) .driver_info = (kernel_ulong_t) name
static __devinitdata struct usb_device_id device_table[] = {
	{USB_DEVICE(0x093a, 0x2600), DVNM("Typhoon")},
	{USB_DEVICE(0x093a, 0x2601), DVNM("Philips SPC610NC")},
	{USB_DEVICE(0x093a, 0x2603), DVNM("PAC7312")},
	{USB_DEVICE(0x093a, 0x2608), DVNM("Trust WB-3300p")},
	{USB_DEVICE(0x093a, 0x260e), DVNM("Gigaware VGA PC Camera, Trust WB-3350p, SIGMA cam 2350")},
	{USB_DEVICE(0x093a, 0x260f), DVNM("SnakeCam")},
	{USB_DEVICE(0x093a, 0x2621), DVNM("PAC731x")},
	{}
};
MODULE_DEVICE_TABLE(usb, device_table);

/* -- device connect -- */
static int sd_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	return gspca_dev_probe(intf, id, &sd_desc, sizeof(struct sd),
				THIS_MODULE);
}

static struct usb_driver sd_driver = {
	.name = MODULE_NAME,
	.id_table = device_table,
	.probe = sd_probe,
	.disconnect = gspca_disconnect,
};

/* -- module insert / remove -- */
static int __init sd_mod_init(void)
{
	if (usb_register(&sd_driver) < 0)
		return -1;
	PDEBUG(D_PROBE, "v%s registered", version);
	return 0;
}
static void __exit sd_mod_exit(void)
{
	usb_deregister(&sd_driver);
	PDEBUG(D_PROBE, "deregistered");
}

module_init(sd_mod_init);
module_exit(sd_mod_exit);
