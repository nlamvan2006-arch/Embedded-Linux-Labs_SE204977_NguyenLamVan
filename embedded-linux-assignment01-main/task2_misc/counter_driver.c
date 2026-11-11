#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/string.h>

#define DEVICE_NAME "counter"
#define COUNTER_MAX_INPUT 32

static atomic_t counter = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);
static atomic_t total_writes = ATOMIC_INIT(0);
static DEFINE_MUTEX(counter_lock);
static struct proc_dir_entry *counter_proc;

static ssize_t counter_read(struct file *file, char __user *buf,
                            size_t len, loff_t *ppos)
{
    char output[32];
    int n, value;
    ssize_t result;

    mutex_lock(&counter_lock);

    if (*ppos != 0) {
        mutex_unlock(&counter_lock);
        return 0;
    }

    value = atomic_read(&counter);
    n = scnprintf(output, sizeof(output), "%d\n", value);

    result = simple_read_from_buffer(buf, len, ppos, output, n);

    if (result > 0) {
        atomic_inc(&counter);
        atomic_inc(&total_reads);
    }

    mutex_unlock(&counter_lock);
    return result;
}

static ssize_t counter_write(struct file *file, const char __user *buf,
                             size_t len, loff_t *ppos)
{
    char input[COUNTER_MAX_INPUT];
    int value;
    int ret;

    if (len == 0 || len >= sizeof(input))
        return -EINVAL;

    if (copy_from_user(input, buf, len))
        return -EFAULT;

    input[len] = '\0';

    ret = kstrtoint(strim(input), 10, &value);
    if (ret || value < 0)
        return -EINVAL;

    mutex_lock(&counter_lock);
    atomic_set(&counter, value);
    atomic_inc(&total_writes);
    mutex_unlock(&counter_lock);

    return len;
}

static int counter_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&counter_lock);

    seq_printf(m, "Counter: %d\n", atomic_read(&counter));
    seq_printf(m, "Total reads: %d\n", atomic_read(&total_reads));
    seq_printf(m, "Total writes: %d\n", atomic_read(&total_writes));

    mutex_unlock(&counter_lock);
    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open = counter_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .read = counter_read,
    .write = counter_write,
    .llseek = no_llseek,
};

static struct miscdevice counter_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &counter_fops,
    .mode = 0666,
};

static int __init counter_init(void)
{
    int ret;

    ret = misc_register(&counter_misc);
    if (ret)
        return ret;

    counter_proc = proc_create("counter_info", 0444, NULL,
                               &counter_proc_ops);
    if (!counter_proc) {
        misc_deregister(&counter_misc);
        return -ENOMEM;
    }

    pr_info("counter_driver: loaded\n");
    return 0;
}

static void __exit counter_exit(void)
{
    proc_remove(counter_proc);
    misc_deregister(&counter_misc);
    pr_info("counter_driver: unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Assignment 01");
MODULE_DESCRIPTION("Misc counter device driver");
