#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME    "SdeC_drv3"
#define DEVICE_COUNT   1
#define BUF_SIZE       128

static dev_t          first_dev;
static struct cdev    drv3_cdev;
static struct class  *drv3_class;
static char           read_buf[BUF_SIZE]  = "DRV3 initial data\n";
static size_t         read_len;
static char           write_buf[BUF_SIZE];
static size_t         write_len;

// open
static int drv3_open(struct inode *inode, struct file *file) {
    read_len = strlen(read_buf);
    printk(KERN_INFO DRIVER_NAME ": open()\n");
    return 0;
}

// release
static int drv3_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO DRIVER_NAME ": release()\n");
    return 0;
}

// read
static ssize_t drv3_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    size_t to_copy = min(count, read_len - (size_t)(*ppos));
    if (to_copy == 0)
        return 0;
    if (copy_to_user(buf, read_buf + *ppos, to_copy))
        return -EFAULT;
    *ppos += to_copy;
    printk(KERN_INFO DRIVER_NAME ": read() returned %zu bytes\n", to_copy);
    return to_copy;
}

// write
static ssize_t drv3_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    size_t to_copy = min(count, (size_t)(BUF_SIZE - 1));
    if (copy_from_user(write_buf, buf, to_copy))
        return -EFAULT;
    write_buf[to_copy] = '\0';
    write_len = to_copy;
    // Update read_buf so subsequent reads see the written data
    strncpy(read_buf, write_buf, BUF_SIZE);
    read_len = write_len;
    printk(KERN_INFO DRIVER_NAME ": write() received %zu bytes: '%s'\n", to_copy, write_buf);
    return to_copy;
}

static const struct file_operations drv3_fops = {
    .owner   = THIS_MODULE,
    .open    = drv3_open,
    .release = drv3_release,
    .read    = drv3_read,
    .write   = drv3_write,
};

static int __init drv3_init(void) {
    int ret;
    // 1) reservar major/minor
    ret = alloc_chrdev_region(&first_dev, 0, DEVICE_COUNT, DRIVER_NAME);
    if (ret) {
        pr_err(DRIVER_NAME ": alloc_chrdev_region failed (%d)\n", ret);
        return ret;
    }
    // 2) registrar cdev
    cdev_init(&drv3_cdev, &drv3_fops);
    drv3_cdev.owner = THIS_MODULE;
    ret = cdev_add(&drv3_cdev, first_dev, DEVICE_COUNT);
    if (ret) {
        unregister_chrdev_region(first_dev, DEVICE_COUNT);
        pr_err(DRIVER_NAME ": cdev_add failed (%d)\n", ret);
        return ret;
    }
    // 3) crear clase para udev
    drv3_class = class_create(DRIVER_NAME);
    if (IS_ERR(drv3_class)) {
        cdev_del(&drv3_cdev);
        unregister_chrdev_region(first_dev, DEVICE_COUNT);
        return PTR_ERR(drv3_class);
    }
    // 4) device_create genera /dev/SdeC_drv3
    device_create(drv3_class, NULL, first_dev, NULL, DRIVER_NAME);
    pr_info(DRIVER_NAME ": loaded with major %d\n", MAJOR(first_dev));
    return 0;
}

static void __exit drv3_exit(void) {
    device_destroy(drv3_class, first_dev);
    class_destroy(drv3_class);
    cdev_del(&drv3_cdev);
    unregister_chrdev_region(first_dev, DEVICE_COUNT);
    pr_info(DRIVER_NAME ": unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TuNombre");
MODULE_DESCRIPTION("drv3: demo read/write with byte counts");
module_init(drv3_init);
module_exit(drv3_exit);
