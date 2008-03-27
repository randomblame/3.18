/*
 * asm-arm/arch-orion/platform.h
 *
 * Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_PLATFORM_H__
#define __ASM_ARCH_PLATFORM_H__

/*
 * Orion EHCI platform driver data.
 */
struct orion_ehci_data {
	struct mbus_dram_target_info	*dram;
};


/*
 * Device bus NAND private data
 */
struct orion_nand_data {
	struct mtd_partition *parts;
	u32 nr_parts;
	u8 ale;		/* address line number connected to ALE */
	u8 cle;		/* address line number connected to CLE */
	u8 width;	/* buswidth */
};


#endif
