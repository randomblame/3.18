/*
 *	OSS handling
 *	Written by Joshua M. Thompson (funaho@jurai.org)
 *
 *
 *	This chip is used in the IIfx in place of VIA #2. It acts like a fancy
 *	VIA chip with prorammable interrupt levels.
 *
 * 990502 (jmt) - Major rewrite for new interrupt architecture as well as some
 *		  recent insights into OSS operational details.
 * 990610 (jmt) - Now taking full advantage of the OSS. Interrupts are mapped
 *		  to mostly match the A/UX interrupt scheme supported on the
 *		  VIA side. Also added support for enabling the ISM irq again
 *		  since we now have a functional IOP manager.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/bootinfo.h>
#include <asm/macintosh.h>
#include <asm/macints.h>
#include <asm/mac_via.h>
#include <asm/mac_oss.h>

int oss_present;
volatile struct mac_oss *oss;

extern void via1_irq(unsigned int irq, struct irq_desc *desc);

/*
 * Initialize the OSS
 *
 * The OSS "detection" code is actually in via_init() which is always called
 * before us. Thus we can count on oss_present being valid on entry.
 */

void __init oss_init(void)
{
	int i;

	if (!oss_present) return;

	oss = (struct mac_oss *) OSS_BASE;

	/* Disable all interrupts. Unlike a VIA it looks like we    */
	/* do this by setting the source's interrupt level to zero. */

	for (i = 0; i <= OSS_NUM_SOURCES; i++) {
		oss->irq_level[i] = OSS_IRQLEV_DISABLED;
	}
	/* If we disable VIA1 here, we never really handle it... */
	oss->irq_level[OSS_VIA1] = OSS_IRQLEV_VIA1;
}

/*
 * Initialize OSS for Nubus access
 */

void __init oss_nubus_init(void)
{
}

/*
 * Handle miscellaneous OSS interrupts. Right now that's just sound
 * and SCSI; everything else is routed to its own autovector IRQ.
 */

static void oss_irq(unsigned int irq, struct irq_desc *desc)
{
	int events;

	events = oss->irq_pending & (OSS_IP_SOUND|OSS_IP_SCSI);
	if (!events)
		return;

#ifdef DEBUG_IRQS
	if ((console_loglevel == 10) && !(events & OSS_IP_SCSI)) {
		printk("oss_irq: irq %u events = 0x%04X\n", irq,
			(int) oss->irq_pending);
	}
#endif
	/* FIXME: how do you clear a pending IRQ?    */

	if (events & OSS_IP_SOUND) {
		oss->irq_pending &= ~OSS_IP_SOUND;
		/* FIXME: call sound handler */
	} else if (events & OSS_IP_SCSI) {
		oss->irq_pending &= ~OSS_IP_SCSI;
		generic_handle_irq(IRQ_MAC_SCSI);
	} else {
		/* FIXME: error check here? */
	}
}

/*
 * Nubus IRQ handler, OSS style
 *
 * Unlike the VIA/RBV this is on its own autovector interrupt level.
 */

static void oss_nubus_irq(unsigned int irq, struct irq_desc *desc)
{
	int events, irq_bit, i;

	events = oss->irq_pending & OSS_IP_NUBUS;
	if (!events)
		return;

#ifdef DEBUG_NUBUS_INT
	if (console_loglevel > 7) {
		printk("oss_nubus_irq: events = 0x%04X\n", events);
	}
#endif
	/* There are only six slots on the OSS, not seven */

	i = 6;
	irq_bit = 0x40;
	do {
		--i;
		irq_bit >>= 1;
		if (events & irq_bit) {
			oss->irq_pending &= ~irq_bit;
			generic_handle_irq(NUBUS_SOURCE_BASE + i);
		}
	} while(events & (irq_bit - 1));
}

/*
 * Register the OSS and NuBus interrupt dispatchers.
 */

void __init oss_register_interrupts(void)
{
	irq_set_chained_handler(OSS_IRQLEV_SCSI, oss_irq);
	irq_set_chained_handler(OSS_IRQLEV_NUBUS, oss_nubus_irq);
	irq_set_chained_handler(OSS_IRQLEV_SOUND, oss_irq);
	irq_set_chained_handler(OSS_IRQLEV_VIA1, via1_irq);
}

/*
 * Enable an OSS interrupt
 *
 * It looks messy but it's rather straightforward. The switch() statement
 * just maps the machspec interrupt numbers to the right OSS interrupt
 * source (if the OSS handles that interrupt) and then sets the interrupt
 * level for that source to nonzero, thus enabling the interrupt.
 */

void oss_irq_enable(int irq) {
#ifdef DEBUG_IRQUSE
	printk("oss_irq_enable(%d)\n", irq);
#endif
	switch(irq) {
		case IRQ_MAC_SCC:
			oss->irq_level[OSS_IOPSCC] = OSS_IRQLEV_IOPSCC;
			break;
		case IRQ_MAC_ADB:
			oss->irq_level[OSS_IOPISM] = OSS_IRQLEV_IOPISM;
			break;
		case IRQ_MAC_SCSI:
			oss->irq_level[OSS_SCSI] = OSS_IRQLEV_SCSI;
			break;
		case IRQ_NUBUS_9:
		case IRQ_NUBUS_A:
		case IRQ_NUBUS_B:
		case IRQ_NUBUS_C:
		case IRQ_NUBUS_D:
		case IRQ_NUBUS_E:
			irq -= NUBUS_SOURCE_BASE;
			oss->irq_level[irq] = OSS_IRQLEV_NUBUS;
			break;
#ifdef DEBUG_IRQUSE
		default:
			printk("%s unknown irq %d\n", __func__, irq);
			break;
#endif
	}
}

/*
 * Disable an OSS interrupt
 *
 * Same as above except we set the source's interrupt level to zero,
 * to disable the interrupt.
 */

void oss_irq_disable(int irq) {
#ifdef DEBUG_IRQUSE
	printk("oss_irq_disable(%d)\n", irq);
#endif
	switch(irq) {
		case IRQ_MAC_SCC:
			oss->irq_level[OSS_IOPSCC] = OSS_IRQLEV_DISABLED;
			break;
		case IRQ_MAC_ADB:
			oss->irq_level[OSS_IOPISM] = OSS_IRQLEV_DISABLED;
			break;
		case IRQ_MAC_SCSI:
			oss->irq_level[OSS_SCSI] = OSS_IRQLEV_DISABLED;
			break;
		case IRQ_NUBUS_9:
		case IRQ_NUBUS_A:
		case IRQ_NUBUS_B:
		case IRQ_NUBUS_C:
		case IRQ_NUBUS_D:
		case IRQ_NUBUS_E:
			irq -= NUBUS_SOURCE_BASE;
			oss->irq_level[irq] = OSS_IRQLEV_DISABLED;
			break;
#ifdef DEBUG_IRQUSE
		default:
			printk("%s unknown irq %d\n", __func__, irq);
			break;
#endif
	}
}

/*
 * Check to see if a specific OSS interrupt is pending
 */

int oss_irq_pending(int irq)
{
	switch(irq) {
		case IRQ_MAC_SCC:
			return oss->irq_pending & OSS_IP_IOPSCC;
			break;
		case IRQ_MAC_ADB:
			return oss->irq_pending & OSS_IP_IOPISM;
			break;
		case IRQ_MAC_SCSI:
			return oss->irq_pending & OSS_IP_SCSI;
			break;
		case IRQ_NUBUS_9:
		case IRQ_NUBUS_A:
		case IRQ_NUBUS_B:
		case IRQ_NUBUS_C:
		case IRQ_NUBUS_D:
		case IRQ_NUBUS_E:
			irq -= NUBUS_SOURCE_BASE;
			return oss->irq_pending & (1 << irq);
			break;
	}
	return 0;
}
