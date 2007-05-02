/*
 * Copyright 2004 James Cleverdon, IBM.
 * Subject to the GNU Public License, v.2
 *
 * Generic APIC sub-arch probe layer.
 *
 * Hacked for x86-64 by James Cleverdon from i386 architecture code by
 * Martin Bligh, Andi Kleen, James Bottomley, John Stultz, and
 * James Cleverdon.
 */
#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/module.h>

#include <asm/smp.h>
#include <asm/ipi.h>

#if defined(CONFIG_ACPI)
#include <acpi/acpi_bus.h>
#endif

/* which logical CPU number maps to which CPU (physical APIC ID) */
u8 x86_cpu_to_apicid[NR_CPUS] __read_mostly = { [0 ... NR_CPUS-1] = BAD_APICID };
EXPORT_SYMBOL(x86_cpu_to_apicid);
u8 x86_cpu_to_log_apicid[NR_CPUS] = { [0 ... NR_CPUS-1] = BAD_APICID };

extern struct genapic apic_cluster;
extern struct genapic apic_flat;
extern struct genapic apic_physflat;

struct genapic __read_mostly *genapic = &apic_flat;

/*
 * Check the APIC IDs in bios_cpu_apicid and choose the APIC mode.
 */
void __init clustered_apic_check(void)
{
	int i;
	u8 clusters, max_cluster;
	u8 id;
	u8 cluster_cnt[NUM_APIC_CLUSTERS];
	int max_apic = 0;

#ifdef CONFIG_ACPI
	/*
	 * Some x86_64 machines use physical APIC mode regardless of how many
	 * procs/clusters are present (x86_64 ES7000 is an example).
	 */
	if (acpi_gbl_FADT.header.revision > FADT2_REVISION_ID)
		if (acpi_gbl_FADT.flags & ACPI_FADT_APIC_PHYSICAL) {
			genapic = &apic_cluster;
			goto print;
		}
#endif

	memset(cluster_cnt, 0, sizeof(cluster_cnt));
	for (i = 0; i < NR_CPUS; i++) {
		id = bios_cpu_apicid[i];
		if (id == BAD_APICID)
			continue;
		if (id > max_apic)
			max_apic = id;
		cluster_cnt[APIC_CLUSTERID(id)]++;
	}

	/*
	 * Don't use clustered mode on AMD platforms, default
	 * to flat logical mode.
	 */
 	if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD) {
		/*
		 * Switch to physical flat mode if more than 8 APICs
		 * (In the case of 8 CPUs APIC ID goes from 0 to 7):
		 */
		if (max_apic >= 8)
			genapic = &apic_physflat;
 		goto print;
 	}

	clusters = 0;
	max_cluster = 0;

	for (i = 0; i < NUM_APIC_CLUSTERS; i++) {
		if (cluster_cnt[i] > 0) {
			++clusters;
			if (cluster_cnt[i] > max_cluster)
				max_cluster = cluster_cnt[i];
		}
	}

	/*
	 * If we have clusters <= 1 and CPUs <= 8 in cluster 0, then flat mode,
	 * else if max_cluster <= 4 and cluster_cnt[15] == 0, clustered logical
	 * else physical mode.
	 * (We don't use lowest priority delivery + HW APIC IRQ steering, so
	 * can ignore the clustered logical case and go straight to physical.)
	 */
	if (clusters <= 1 && max_cluster <= 8 && cluster_cnt[0] == max_cluster)
		genapic = &apic_flat;
	else
		genapic = &apic_cluster;

print:
	printk(KERN_INFO "Setting APIC routing to %s\n", genapic->name);
}

/* Same for both flat and clustered. */

void send_IPI_self(int vector)
{
	__send_IPI_shortcut(APIC_DEST_SELF, vector, APIC_DEST_PHYSICAL);
}
