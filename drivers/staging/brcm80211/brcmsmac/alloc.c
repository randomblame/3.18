#/*
 * Copyright (c) 2010 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <brcmu_utils.h>
#include "types.h"
#include "pub.h"
#include "main.h"
#include "alloc.h"

static void brcms_c_bsscfg_mfree(struct brcms_bss_cfg *cfg)
{
	if (cfg == NULL)
		return;

	kfree(cfg->current_bss);
	kfree(cfg);
}

static struct brcms_bss_cfg *brcms_c_bsscfg_malloc(uint unit)
{
	struct brcms_bss_cfg *cfg;

	cfg = kzalloc(sizeof(struct brcms_bss_cfg), GFP_ATOMIC);
	if (cfg == NULL)
		goto fail;

	cfg->current_bss = kzalloc(sizeof(struct brcms_bss_info), GFP_ATOMIC);
	if (cfg->current_bss == NULL)
		goto fail;

	return cfg;

 fail:
	brcms_c_bsscfg_mfree(cfg);
	return NULL;
}

/*
 * The common driver entry routine. Error codes should be unique
 */
struct brcms_c_info *brcms_c_attach_malloc(uint unit, uint *err, uint devid)
{
	struct brcms_c_info *wlc;

	wlc = kzalloc(sizeof(struct brcms_c_info), GFP_ATOMIC);
	if (wlc == NULL) {
		*err = 1002;
		goto fail;
	}

	/* allocate struct brcms_c_pub state structure */
	wlc->pub = kzalloc(sizeof(struct brcms_pub), GFP_ATOMIC);
	if (wlc->pub == NULL) {
		*err = 1003;
		goto fail;
	}
	wlc->pub->wlc = wlc;

	/* allocate struct brcms_hardware state structure */

	wlc->hw = kzalloc(sizeof(struct brcms_hardware), GFP_ATOMIC);
	if (wlc->hw == NULL) {
		*err = 1005;
		goto fail;
	}
	wlc->hw->wlc = wlc;

	wlc->hw->bandstate[0] =
		kzalloc(sizeof(struct brcms_hw_band) * MAXBANDS, GFP_ATOMIC);
	if (wlc->hw->bandstate[0] == NULL) {
		*err = 1006;
		goto fail;
	} else {
		int i;

		for (i = 1; i < MAXBANDS; i++)
			wlc->hw->bandstate[i] = (struct brcms_hw_band *)
			    ((unsigned long)wlc->hw->bandstate[0] +
			     (sizeof(struct brcms_hw_band) * i));
	}

	wlc->modulecb =
		kzalloc(sizeof(struct modulecb) * BRCMS_MAXMODULES, GFP_ATOMIC);
	if (wlc->modulecb == NULL) {
		*err = 1009;
		goto fail;
	}

	wlc->default_bss = kzalloc(sizeof(struct brcms_bss_info), GFP_ATOMIC);
	if (wlc->default_bss == NULL) {
		*err = 1010;
		goto fail;
	}

	wlc->cfg = brcms_c_bsscfg_malloc(unit);
	if (wlc->cfg == NULL) {
		*err = 1011;
		goto fail;
	}

	wlc->protection = kzalloc(sizeof(struct brcms_protection),
				  GFP_ATOMIC);
	if (wlc->protection == NULL) {
		*err = 1016;
		goto fail;
	}

	wlc->stf = kzalloc(sizeof(struct brcms_stf), GFP_ATOMIC);
	if (wlc->stf == NULL) {
		*err = 1017;
		goto fail;
	}

	wlc->bandstate[0] =
		kzalloc(sizeof(struct brcms_band)*MAXBANDS, GFP_ATOMIC);
	if (wlc->bandstate[0] == NULL) {
		*err = 1025;
		goto fail;
	} else {
		int i;

		for (i = 1; i < MAXBANDS; i++)
			wlc->bandstate[i] = (struct brcms_band *)
				((unsigned long)wlc->bandstate[0]
				+ (sizeof(struct brcms_band)*i));
	}

	wlc->corestate = kzalloc(sizeof(struct brcms_core), GFP_ATOMIC);
	if (wlc->corestate == NULL) {
		*err = 1026;
		goto fail;
	}

	wlc->corestate->macstat_snapshot =
		kzalloc(sizeof(struct macstat), GFP_ATOMIC);
	if (wlc->corestate->macstat_snapshot == NULL) {
		*err = 1027;
		goto fail;
	}

	return wlc;

 fail:
	brcms_c_detach_mfree(wlc);
	return NULL;
}

void brcms_c_detach_mfree(struct brcms_c_info *wlc)
{
	if (wlc == NULL)
		return;

	brcms_c_bsscfg_mfree(wlc->cfg);
	kfree(wlc->pub);
	kfree(wlc->modulecb);
	kfree(wlc->default_bss);
	kfree(wlc->protection);
	kfree(wlc->stf);
	kfree(wlc->bandstate[0]);
	kfree(wlc->corestate->macstat_snapshot);
	kfree(wlc->corestate);
	kfree(wlc->hw->bandstate[0]);
	kfree(wlc->hw);

	/* free the wlc */
	kfree(wlc);
	wlc = NULL;
}
