/*
 * Coherency fabric (Aurora) support for Armada 370 and XP platforms.
 *
 * Copyright (C) 2012 Marvell
 *
 * Yehuda Yitschak <yehuday@marvell.com>
 * Gregory Clement <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * The Armada 370 and Armada XP SOCs have a coherency fabric which is
 * responsible for ensuring hardware coherency between all CPUs and between
 * CPUs and I/O masters. This file initializes the coherency fabric and
 * supplies basic routines for configuring and controlling hardware coherency
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/smp.h>
#include <asm/smp_plat.h>
#include "armada-370-xp.h"

/*
 * Some functions in this file are called very early during SMP
 * initialization. At that time the device tree framework is not yet
 * ready, and it is not possible to get the register address to
 * ioremap it. That's why the pointer below is given with an initial
 * value matching its virtual mapping
 */
static void __iomem *coherency_base = ARMADA_370_XP_REGS_VIRT_BASE + 0x20200;

/* Coherency fabric registers */
#define COHERENCY_FABRIC_CFG_OFFSET		   0x4

static struct of_device_id of_coherency_table[] = {
	{.compatible = "marvell,coherency-fabric"},
	{ /* end of list */ },
};

#ifdef CONFIG_SMP
int coherency_get_cpu_count(void)
{
	int reg, cnt;

	reg = readl(coherency_base + COHERENCY_FABRIC_CFG_OFFSET);
	cnt = (reg & 0xF) + 1;

	return cnt;
}
#endif

/* Function defined in coherency_ll.S */
int ll_set_cpu_coherent(void __iomem *base_addr, unsigned int hw_cpu_id);

int set_cpu_coherent(unsigned int hw_cpu_id, int smp_group_id)
{
	if (!coherency_base) {
		pr_warn("Can't make CPU %d cache coherent.\n", hw_cpu_id);
		pr_warn("Coherency fabric is not initialized\n");
		return 1;
	}

	return ll_set_cpu_coherent(coherency_base, hw_cpu_id);
}

int __init coherency_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_coherency_table);
	if (np) {
		pr_info("Initializing Coherency fabric\n");
		coherency_base = of_iomap(np, 0);
	}

	return 0;
}
