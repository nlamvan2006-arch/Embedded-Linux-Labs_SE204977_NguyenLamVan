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
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/jiffies.h>
#include "sms_sensor.h"

#define DEVICE_NAME "sms_sensor"
#define SMS_CSV_MAX 80

static atomic_t read_count = ATOMIC_INIT(0);

static unsigned int last_temp_x100;
static unsigned int last_humid_x100;
static unsigned int interval_ms = 100;

static u64 last_read_ms;
static u64 start_ms;

static DEFINE_MUTEX(sensor_lock);
static struct proc_dir_entry *sensor_proc;

static ssize_t sensor_read(struct file *file, char __user *buf,
                           size_t len, loff_t *ppos)
{
    char output[SMS_CSV_MAX];
    unsigned int temp_x100, humid_x100;
    u64 now;
    int n;
    ssize_t result;

    mutex_lock(&sensor_lock);

    (void)file;
    (void)ppos;

    now = div_u64(ktime_get_ns(), 1000000U);

    if (last_read_ms != 0 && now - last_read_ms < interval_ms) {
        mutex_unlock(&sensor_lock);
        return -EAGAIN;
    }

    temp_x100 = 1500 + get_random_u32() % 3001;
    humid_x100 = 3000 + get_random_u32() % 6001;

    n = scnprintf(output, sizeof(output),
                  "%llu,%u.%02u,%u.%02u\n",
                  (unsigned long long)now,
                  temp_x100 / 100, temp_x100 % 100,
                  humid_x100 / 100, humid_x100 % 100);

    if (len < n) {
        mutex_unlock(&sensor_lock);
        return -EINVAL;
    }

    if (copy_to_user(buf, output, n)) {
        mutex_unlock(&sensor_lock);
        return -EFAULT;
    }

    result = n;
    last_temp_x100 = temp_x100;
    last_humid_x100 = humid_x100;
    last_read_ms = now;
    atomic_inc(&read_count);

    mutex_unlock(&sensor_lock);
    return result;
}

static int sensor_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&sensor_lock);

    seq_printf(m, "read_count: %d\n", atomic_read(&read_count));
    seq_printf(m, "last_temperature: %u.%02u\n",
               last_temp_x100 / 100, last_temp_x100 % 100);
    seq_printf(m, "last_humidity: %u.%02u\n",
               last_humid_x100 / 100, last_humid_x100 % 100);
    seq_printf(m, "uptime_seconds: %llu\n",
               (unsigned long long)(div_u64(div_u64(ktime_get_ns(), 1000000U) - start_ms, 1000U)));

    mutex_unlock(&sensor_lock);
    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sensor_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open = counter_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static long sensor_ioctl(struct file *file,
                         unsigned int cmd, unsigned long arg)
{
    unsigned int value;

    if (cmd != SMS_SET_INTERVAL)
        return -ENOTTY;

    if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
        return -EFAULT;

    if (value < 1 || value > 60000)
        return -EINVAL;

    mutex_lock(&sensor_lock);
    interval_ms = value;
    mutex_unlock(&sensor_lock);

    return 0;
}

static const struct file_operations sensor_fops = {
    .owner = THIS_MODULE,
    .read = sensor_read,
    .unlocked_ioctl = sensor_ioctl,
    .llseek = no_llseek,
};

static struct miscdevice sensor_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &sensor_fops,
    .mode = 0666,
};

static int __init sensor_init(void)
{
    int ret;

    start_ms = div_u64(ktime_get_ns(), 1000000U);

    ret = misc_register(&sensor_misc);
    if (ret)
        return ret;

    sensor_proc = proc_create("sms_stats", 0444, NULL,
                              &counter_proc_ops);
    if (!sensor_proc) {
        misc_deregister(&sensor_misc);
        return -ENOMEM;
    }

    pr_info("sms_sensor_driver: loaded\\n");
    return 0;
}

static void __exit sensor_exit(void)
{
    proc_remove(sensor_proc);
    misc_deregister(&sensor_misc);
    pr_info("sms_sensor_driver: unloaded\\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Assignment 01");
MODULE_DESCRIPTION("Misc counter device driver");
