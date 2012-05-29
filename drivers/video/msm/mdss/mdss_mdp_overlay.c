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

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "mdss_fb.h"
#include "mdss_mdp.h"

#define CHECK_BOUNDS(offset, size, max_size) \
	(((size) > (max_size)) || ((offset) > ((max_size) - (size))))

static int mdss_mdp_overlay_req_check(struct msm_fb_data_type *mfd,
				      struct mdp_overlay *req,
				      struct mdss_mdp_format_params *fmt)
{
	u32 xres, yres;
	u32 dst_w, dst_h;

	xres = mfd->fbi->var.xres;
	yres = mfd->fbi->var.yres;

	if (req->z_order >= MDSS_MDP_MAX_STAGE) {
		pr_err("zorder %d out of range\n", req->z_order);
		return -ERANGE;
	}

	if (req->src.width > MAX_IMG_WIDTH ||
	    req->src.height > MAX_IMG_HEIGHT ||
	    req->src_rect.w == 0 || req->src_rect.h == 0 ||
	    req->dst_rect.w < MIN_DST_W || req->dst_rect.h < MIN_DST_H ||
	    req->dst_rect.w > MAX_DST_W || req->dst_rect.h > MAX_DST_H ||
	    CHECK_BOUNDS(req->src_rect.x, req->src_rect.w, req->src.width) ||
	    CHECK_BOUNDS(req->src_rect.y, req->src_rect.h, req->src.height) ||
	    CHECK_BOUNDS(req->dst_rect.x, req->dst_rect.w, xres) ||
	    CHECK_BOUNDS(req->dst_rect.y, req->dst_rect.h, yres)) {
		pr_err("invalid image img_w=%d img_h=%d\n",
				req->src.width, req->src.height);

		pr_err("\tsrc_rect=%d,%d,%d,%d dst_rect=%d,%d,%d,%d\n",
		       req->src_rect.x, req->src_rect.y,
		       req->src_rect.w, req->src_rect.h,
		       req->dst_rect.x, req->dst_rect.y,
		       req->dst_rect.w, req->dst_rect.h);
		return -EINVAL;
	}

	if (req->flags & MDP_ROT_90) {
		dst_h = req->dst_rect.w;
		dst_w = req->dst_rect.h;
	} else {
		dst_w = req->dst_rect.w;
		dst_h = req->dst_rect.h;
	}

	if ((req->src_rect.w * MAX_UPSCALE_RATIO) < dst_w) {
		pr_err("too much upscaling Width %d->%d\n",
		       req->src_rect.w, req->dst_rect.w);
		return -EINVAL;
	}

	if ((req->src_rect.h * MAX_UPSCALE_RATIO) < dst_h) {
		pr_err("too much upscaling. Height %d->%d\n",
		       req->src_rect.h, req->dst_rect.h);
		return -EINVAL;
	}

	if (req->src_rect.w > (dst_w * MAX_DOWNSCALE_RATIO)) {
		pr_err("too much downscaling. Width %d->%d\n",
		       req->src_rect.w, req->dst_rect.w);
		return -EINVAL;
	}

	if (req->src_rect.h > (dst_h * MAX_DOWNSCALE_RATIO)) {
		pr_err("too much downscaling. Height %d->%d\n",
		       req->src_rect.h, req->dst_rect.h);
		return -EINVAL;
	}

	if (fmt->is_yuv) {
		if ((req->src_rect.x & 0x1) || (req->src_rect.y & 0x1) ||
		    (req->src_rect.w & 0x1) || (req->src_rect.h & 0x1)) {
			pr_err("invalid odd src resolution\n");
			return -EINVAL;
		}
		if ((req->dst_rect.x & 0x1) || (req->dst_rect.y & 0x1) ||
		    (req->dst_rect.w & 0x1) || (req->dst_rect.h & 0x1)) {
			pr_err("invalid odd dst resolution\n");
			return -EINVAL;
		}

		if (((req->src_rect.w * (MAX_UPSCALE_RATIO / 2)) < dst_w) &&
		    (fmt->chroma_sample == MDSS_MDP_CHROMA_420 ||
		     fmt->chroma_sample == MDSS_MDP_CHROMA_H2V1)) {
			pr_err("too much YUV upscaling Width %d->%d\n",
			       req->src_rect.w, req->dst_rect.w);
			return -EINVAL;
		}

		if (((req->src_rect.h * (MAX_UPSCALE_RATIO / 2)) < dst_h) &&
		    (fmt->chroma_sample == MDSS_MDP_CHROMA_420 ||
		     fmt->chroma_sample == MDSS_MDP_CHROMA_H1V2)) {
			pr_err("too much YUV upscaling Height %d->%d\n",
			       req->src_rect.h, req->dst_rect.h);
			return -EINVAL;
		}
	}

	return 0;
}

static int mdss_mdp_overlay_pipe_setup(struct msm_fb_data_type *mfd,
				       struct mdp_overlay *req,
				       struct mdss_mdp_pipe **ppipe)
{
	struct mdss_mdp_format_params *fmt;
	struct mdss_mdp_pipe *pipe;
	struct mdss_mdp_mixer *mixer = NULL;
	u32 pipe_type, mixer_mux;
	int ret;

	if (mfd == NULL || mfd->ctl == NULL)
		return -ENODEV;

	if (req->flags & MDSS_MDP_RIGHT_MIXER)
		mixer_mux = MDSS_MDP_MIXER_MUX_RIGHT;
	else
		mixer_mux = MDSS_MDP_MIXER_MUX_LEFT;

	pr_debug("pipe ctl=%u req id=%x mux=%d\n", mfd->ctl->num, req->id,
			mixer_mux);

	if (req->flags & MDP_ROT_90) {
		pr_err("unsupported inline rotation\n");
		return -ENOTSUPP;
	}

	fmt = mdss_mdp_get_format_params(req->src.format);
	if (!fmt) {
		pr_err("invalid pipe format %d\n", req->src.format);
		return -EINVAL;
	}

	ret = mdss_mdp_overlay_req_check(mfd, req, fmt);
	if (ret)
		return ret;

	pipe = mdss_mdp_mixer_stage_pipe(mfd->ctl, mixer_mux, req->z_order);
	if (pipe && pipe->ndx != req->id) {
		pr_err("stage %d taken by pnum=%d\n", req->z_order, pipe->num);
		return -EBUSY;
	}


	if (req->id == MSMFB_NEW_REQUEST) {
		mixer = mdss_mdp_mixer_get(mfd->ctl, mixer_mux);
		if (!mixer) {
			pr_err("unable to get mixer\n");
			return -ENODEV;
		}

		if (fmt->is_yuv || (req->flags & MDP_OV_PIPE_SHARE))
			pipe_type = MDSS_MDP_PIPE_TYPE_VIG;
		else
			pipe_type = MDSS_MDP_PIPE_TYPE_RGB;

		pipe = mdss_mdp_pipe_alloc_locked(pipe_type);

		/* VIG pipes can also support RGB format */
		if (!pipe && pipe_type == MDSS_MDP_PIPE_TYPE_RGB) {
			pipe_type = MDSS_MDP_PIPE_TYPE_VIG;
			pipe = mdss_mdp_pipe_alloc_locked(pipe_type);
		}

		if (pipe == NULL) {
			pr_err("error allocating pipe\n");
			return -ENOMEM;
		}

		pipe->mixer = mixer;
		pipe->mfd = mfd;
	} else {
		pipe = mdss_mdp_pipe_get_locked(req->id);
		if (pipe == NULL) {
			pr_err("invalid pipe ndx=%x\n", req->id);
			return -ENODEV;
		}
	}

	pipe->flags = req->flags;

	pipe->img_width = req->src.width & 0x3fff;
	pipe->img_height = req->src.height & 0x3fff;
	pipe->src.x = req->src_rect.x;
	pipe->src.y = req->src_rect.y;
	pipe->src.w = req->src_rect.w;
	pipe->src.h = req->src_rect.h;
	pipe->dst.x = req->dst_rect.x;
	pipe->dst.y = req->dst_rect.y;
	pipe->dst.w = req->dst_rect.w;
	pipe->dst.h = req->dst_rect.h;

	pipe->src_fmt = fmt;

	pipe->mixer_stage = req->z_order;
	pipe->is_fg = req->is_fg;
	pipe->alpha = req->alpha;
	pipe->transp = req->transp_mask;

	pipe->req_data = *req;

	pipe->params_changed++;

	req->id = pipe->ndx;

	*ppipe = pipe;

	mdss_mdp_pipe_unlock(pipe);

	return ret;
}

static int mdss_mdp_overlay_get_fb_pipe(struct msm_fb_data_type *mfd,
					struct mdss_mdp_pipe **ppipe,
					int mixer_mux)
{
	struct mdss_mdp_pipe *pipe;

	pipe = mdss_mdp_mixer_stage_pipe(mfd->ctl, mixer_mux,
					 MDSS_MDP_STAGE_BASE);
	if (pipe == NULL) {
		struct mdp_overlay req;
		int ret;

		memset(&req, 0, sizeof(req));

		req.id = MSMFB_NEW_REQUEST;
		req.src.format = mfd->fb_imgType;
		req.src.height = mfd->fbi->var.yres;
		req.src.width = mfd->fbi->var.xres;
		if (mixer_mux == MDSS_MDP_MIXER_MUX_RIGHT) {
			if (req.src.width <= MAX_MIXER_WIDTH)
				return -ENODEV;

			req.flags |= MDSS_MDP_RIGHT_MIXER;
			req.src_rect.x = MAX_MIXER_WIDTH;
			req.src_rect.w = req.src.width - MAX_MIXER_WIDTH;
		} else {
			req.src_rect.x = 0;
			req.src_rect.w = MIN(req.src.width, MAX_MIXER_WIDTH);
		}

		req.src_rect.y = 0;
		req.src_rect.h = req.src.height;
		req.dst_rect.x = req.src_rect.x;
		req.dst_rect.y = 0;
		req.dst_rect.w = req.src_rect.w;
		req.dst_rect.h = req.src_rect.h;
		req.z_order = MDSS_MDP_STAGE_BASE;

		pr_debug("allocating base pipe mux=%d\n", mixer_mux);

		ret = mdss_mdp_overlay_pipe_setup(mfd, &req, &pipe);
		if (ret)
			return ret;

		pr_debug("ctl=%d pnum=%d\n", mfd->ctl->num, pipe->num);
	}

	*ppipe = pipe;
	return 0;
}

static void mdss_mdp_overlay_pan_display(struct msm_fb_data_type *mfd)
{
	struct mdss_mdp_data data;
	struct mdss_mdp_pipe *pipe;
	struct fb_info *fbi;
	u32 offset;
	int bpp, ret;

	if (!mfd)
		return;

	if (!mfd->ctl || !mfd->panel_power_on)
		return;

	fbi = mfd->fbi;

	if (fbi->fix.smem_len == 0) {
		pr_warn("fb memory not allocated\n");
		return;
	}

	memset(&data, 0, sizeof(data));

	bpp = fbi->var.bits_per_pixel / 8;
	offset = fbi->var.xoffset * bpp +
		 fbi->var.yoffset * fbi->fix.line_length;

	data.p[0].addr = fbi->fix.smem_start + offset;
	data.p[0].len = fbi->fix.smem_len - offset;
	data.num_planes = 1;

	ret = mdss_mdp_overlay_get_fb_pipe(mfd, &pipe, MDSS_MDP_MIXER_MUX_LEFT);
	if (ret) {
		pr_err("unable to allocate base pipe\n");
		return;
	}

	mdss_mdp_pipe_lock(pipe);
	ret = mdss_mdp_pipe_queue_data(pipe, &data);
	mdss_mdp_pipe_unlock(pipe);
	if (ret) {
		pr_err("unable to queue data\n");
		return;
	}

	if (fbi->var.xres > MAX_MIXER_WIDTH) {
		ret = mdss_mdp_overlay_get_fb_pipe(mfd, &pipe,
						   MDSS_MDP_MIXER_MUX_RIGHT);
		if (ret) {
			pr_err("unable to allocate right base pipe\n");
			return;
		}
		mdss_mdp_pipe_lock(pipe);
		ret = mdss_mdp_pipe_queue_data(pipe, &data);
		mdss_mdp_pipe_unlock(pipe);
		if (ret) {
			pr_err("unable to queue right data\n");
			return;
		}
	}

	mdss_mdp_display_commit(mfd->ctl, NULL);
}

static int mdss_mdp_hw_cursor_update(struct fb_info *info,
				     struct fb_cursor *cursor)
{
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)info->par;
	struct mdss_mdp_mixer *mixer;
	struct fb_image *img = &cursor->image;
	int calpha_en, transp_en, blendcfg, alpha;
	int off, ret = 0;

	mixer = mdss_mdp_mixer_get(mfd->ctl, MDSS_MDP_MIXER_MUX_DEFAULT);
	off = MDSS_MDP_REG_LM_OFFSET(mixer->num);

	if ((img->width > MDSS_MDP_CURSOR_WIDTH) ||
	    (img->height > MDSS_MDP_CURSOR_HEIGHT) ||
	    (img->depth != 32))
		return -EINVAL;

	pr_debug("mixer=%d enable=%x set=%x\n", mixer->num, cursor->enable,
			cursor->set);

	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON, false);
	blendcfg = MDSS_MDP_REG_READ(off + MDSS_MDP_REG_LM_CURSOR_BLEND_CONFIG);

	if (cursor->set & FB_CUR_SETPOS)
		MDSS_MDP_REG_WRITE(off + MDSS_MDP_REG_LM_CURSOR_START_XY,
				   (img->dy << 16) | img->dx);

	if (cursor->set & FB_CUR_SETIMAGE) {
		ret = copy_from_user(mfd->cursor_buf, img->data,
				     img->width * img->height * 4);
		if (ret)
			return ret;

		if (img->bg_color == 0xffffffff)
			transp_en = 0;
		else
			transp_en = 1;

		alpha = (img->fg_color & 0xff000000) >> 24;

		if (alpha)
			calpha_en = 0x0; /* xrgb */
		else
			calpha_en = 0x2; /* argb */

		MDSS_MDP_REG_WRITE(off + MDSS_MDP_REG_LM_CURSOR_SIZE,
				   (img->height << 16) | img->width);
		MDSS_MDP_REG_WRITE(off + MDSS_MDP_REG_LM_CURSOR_STRIDE,
				   img->width * 4);
		MDSS_MDP_REG_WRITE(off + MDSS_MDP_REG_LM_CURSOR_BASE_ADDR,
				   mfd->cursor_buf_phys);

		wmb();

		blendcfg &= ~0x1;
		blendcfg |= (transp_en << 3) | (calpha_en << 1);
		MDSS_MDP_REG_WRITE(off + MDSS_MDP_REG_LM_CURSOR_BLEND_CONFIG,
				   blendcfg);
		if (calpha_en)
			MDSS_MDP_REG_WRITE(off +
					   MDSS_MDP_REG_LM_CURSOR_BLEND_PARAM,
					   alpha);

		if (transp_en) {
			MDSS_MDP_REG_WRITE(off +
				   MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_LOW0,
				   ((img->bg_color & 0xff00) << 8) |
				   (img->bg_color & 0xff));
			MDSS_MDP_REG_WRITE(off +
				   MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_LOW1,
				   ((img->bg_color & 0xff0000) >> 16));
			MDSS_MDP_REG_WRITE(off +
				   MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_HIGH0,
				   ((img->bg_color & 0xff00) << 8) |
				   (img->bg_color & 0xff));
			MDSS_MDP_REG_WRITE(off +
				   MDSS_MDP_REG_LM_CURSOR_BLEND_TRANSP_HIGH1,
				   ((img->bg_color & 0xff0000) >> 16));
		}
	}

	if (!cursor->enable != !(blendcfg & 0x1)) {
		if (cursor->enable)
			blendcfg |= 0x1;
		else
			blendcfg &= ~0x1;

		MDSS_MDP_REG_WRITE(off + MDSS_MDP_REG_LM_CURSOR_BLEND_CONFIG,
				   blendcfg);

		mixer->cursor_enabled = cursor->enable;
		mixer->params_changed++;
	}

	mixer->ctl->flush_bits |= BIT(6) << mixer->num;
	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF, false);

	return 0;
}

int mdss_mdp_overlay_init(struct msm_fb_data_type *mfd)
{
	mfd->on_fnc = mdss_mdp_ctl_on;
	mfd->off_fnc = mdss_mdp_ctl_off;
	mfd->hw_refresh = true;
	mfd->lut_update = NULL;
	mfd->do_histogram = NULL;
	mfd->overlay_play_enable = false;
	mfd->cursor_update = mdss_mdp_hw_cursor_update;
	mfd->dma_fnc = mdss_mdp_overlay_pan_display;

	return 0;
}
