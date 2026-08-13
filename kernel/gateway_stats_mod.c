/*
 * gateway_stats_mod.c — High-Performance C++ Linux Network Gateway
 * Educational Loadable Linux Kernel Module (LKM)
 *
 * Demonstrates:
 *  - Kernel space (Ring 0) vs User space (Ring 3) privilege separation
 *  - LKM Entry & Exit points (init_module / cleanup_module)
 *  - Procfs virtual filesystem interface (/proc/gateway_stats)
 *  - Kernel 64-bit atomic counters (atomic64_t)
 *  - Kernel logging infrastructure (printk / pr_info)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qualcomm Software Engineering Intern Project");
MODULE_DESCRIPTION("High-Performance Gateway Kernel-Level Packet Telemetry LKM");
MODULE_VERSION("1.0");

#define PROC_FILENAME "gateway_stats"

// Kernel Atomic Counters for Gateway Telemetry
static atomic64_t g_rx_packets   = ATOMIC64_INIT(1048576); // Initial mock 1M packets
static atomic64_t g_tx_packets   = ATOMIC64_INIT(1048000);
static atomic64_t g_drop_packets = ATOMIC64_INIT(576);

// Procfs Sequence File Output Callback
static int gateway_stats_show(struct seq_file *m, void *v) {
    seq_printf(m, "========================================\n");
    seq_printf(m, " LINUX KERNEL GATEWAY TELEMETRY (/proc/%s)\n", PROC_FILENAME);
    seq_printf(m, "========================================\n");
    seq_printf(m, "  Kernel Ring Privilege : Ring 0 (LKM Driver)\n");
    seq_printf(m, "  Kernel RX Packets     : %lld\n", (long long)atomic64_read(&g_rx_packets));
    seq_printf(m, "  Kernel TX Packets     : %lld\n", (long long)atomic64_read(&g_tx_packets));
    seq_printf(m, "  Kernel Dropped Packets: %lld\n", (long long)atomic64_read(&g_drop_packets));
    seq_printf(m, "========================================\n");
    return 0;
}

static int gateway_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, gateway_stats_show, NULL);
}

// Proc Operations Interface (Linux 5.6+ proc_ops compatibility)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops gateway_proc_ops = {
    .proc_open    = gateway_stats_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations gateway_proc_ops = {
    .owner   = THIS_MODULE,
    .open    = gateway_stats_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif

// Kernel Module Initialization Entry Point (Ring 0 Setup)
static int __init gateway_stats_init(void) {
    struct proc_dir_entry *entry;

    pr_info("[Gateway Kernel LKM] Initializing Gateway Telemetry Driver (Ring 0)\n");

    entry = proc_create(PROC_FILENAME, 0444, NULL, &gateway_proc_ops);
    if (!entry) {
        pr_err("[Gateway Kernel LKM] Failed to create /proc/%s entry\n", PROC_FILENAME);
        return -ENOMEM;
    }

    pr_info("[Gateway Kernel LKM] Registered virtual file /proc/%s successfully\n", PROC_FILENAME);
    return 0; // Success
}

// Kernel Module Exit Entry Point (Ring 0 Teardown)
static void __exit gateway_stats_exit(void) {
    pr_info("[Gateway Kernel LKM] Unloading Gateway Telemetry Driver...\n");
    remove_proc_entry(PROC_FILENAME, NULL);
    pr_info("[Gateway Kernel LKM] Driver unloaded cleanly.\n");
}

module_init(gateway_stats_init);
module_exit(gateway_stats_exit);
