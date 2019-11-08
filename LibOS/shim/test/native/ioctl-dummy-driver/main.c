/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <asm/tlbflush.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/security.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include <linux/eventfd.h>
#include <linux/fdtable.h>
#include <linux/pid.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>

#include "dummy.h"

#define DRV_DESCRIPTION "Dummy Driver"
#define DRV_VERSION "0.1"

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Chia-Che Tsai <chia-che.tsai@intel.com>");
MODULE_AUTHOR("Sudhar Krishna");
MODULE_VERSION(DRV_VERSION);

/* Implemented based on suggestion here->https://stackoverflow.com/a/13615599 */
static int signal_efd_process_context(int efd) {
    struct eventfd_ctx* efd_ctx = NULL;
    // Increment Counter
    static uint64_t counter_val = 1000;

    printk(KERN_ERR "efd=%d\n", efd);

    if (efd == 0) {
        printk(KERN_ERR "Invalid values efd=%d\n", efd);
        return -EFAULT;
    }

    efd_ctx = eventfd_ctx_fdget(efd);
    if (!efd_ctx) {
        printk(KERN_ERR "eventfd_ctx does not exist.\n");
        return -EFAULT;
    }

    printk(KERN_ERR "eventfd's context: %p\n", efd_ctx);
    eventfd_signal(efd_ctx, counter_val);
    printk(KERN_ERR "Incremented eventfd's counter\n");
    eventfd_ctx_put(efd_ctx);

    return 0;
}

static long dummy_efd(struct file* filep, void* arg) {
    int error = 0;
    int ret   = 0;

    struct dummy_efd_info efd_kernel;
    struct dummy_efd_info* efdp = arg;

    efd_kernel.pid = 0;
    efd_kernel.efd = 0;

    if ((error = copy_from_user(&efd_kernel, (void __user*)efdp, sizeof(struct dummy_efd_info)))) {
        printk(KERN_ERR "%s, copy from user, error=%d\n", __func__, error);
        ret = -EFAULT;
    }

    printk(KERN_ERR "dummy_efd pid=%d, efd=%d:\n", efd_kernel.pid, efd_kernel.efd);

    signal_efd_process_context(efd_kernel.efd);

    return ret;
}

static long dummy_print(struct file* filep, void* arg) {
    char data[256];
    char buf[256];

    int error = 0;
    int size  = 0;

    struct dummy_print* print_buffp;

    if (copy_from_user(buf, (void __user*)arg, sizeof(struct dummy_print))) {
        printk(KERN_ERR "copy from user error for dummy_print\n");
        return -EFAULT;
    }

    print_buffp = (struct dummy_print*)buf;
    size        = print_buffp->size;

    // Note: doing memcpy..eventhough buffer is in kernel..causes kernel crash.
    if ((error = copy_from_user(data, (void __user*)print_buffp->str, size))) {
        printk(KERN_ERR "copy from user, error=%d\n", error);
        return -EFAULT;
    }

    data[size] = '\0';
    printk(KERN_ERR "dummy print: %s\n", data);

    return 0;
}

long dummy_ioctl(struct file* filep, unsigned int cmd, unsigned long arg) {
    long (*handler)(struct file * filp, void* arg) = NULL;
    long ret;

    printk(KERN_ERR "dummy_ioctl: cmd=%u\n", cmd);

    switch (cmd) {
        case DUMMY_IOCTL_PRINT:
            handler = dummy_print;
            break;
        case DUMMY_IOCTL_EFD:
            handler = dummy_efd;
            break;

        default:
            return -EINVAL;
    }

    ret = handler(filep, (void*)arg);

    if (!ret) {
        printk(KERN_ERR "ioctl handler PASS for : cmd=%u\n", cmd);
    } else
        printk(KERN_ERR "ioctl handler error for : cmd=%u\n", cmd);

    return ret;
}

int dummy_open(struct inode* inode, struct file* file) {
    return 0;
}

static const struct file_operations dummy_fops = {
    .owner          = THIS_MODULE,
    .open           = dummy_open,
    .unlocked_ioctl = dummy_ioctl,
    .compat_ioctl   = dummy_ioctl,
};

static struct miscdevice dummy_dev = {
    .minor = DUMMY_MINOR, .name = "dummy", .fops = &dummy_fops, .mode = S_IRUGO | S_IWUGO,
};

static int dummy_setup(void) {
    int ret;

    ret = misc_register(&dummy_dev);
    if (ret) {
        pr_err("dummy: misc_register() failed\n");
        dummy_dev.this_device = NULL;
        return ret;
    }

    return 0;
}

static void dummy_teardown(void) {
    if (dummy_dev.this_device)
        misc_deregister(&dummy_dev);
}

static int __init dummy_init(void) {
    int ret;

    pr_info("dummy: " DRV_DESCRIPTION " v" DRV_VERSION "\n");

    ret = dummy_setup();
    if (ret) {
        dummy_teardown();
        return ret;
    }

    return 0;
}

static void __exit dummy_exit(void) {
    dummy_teardown();
}

module_init(dummy_init);
module_exit(dummy_exit);
MODULE_LICENSE("GPL");
