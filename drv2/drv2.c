#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "drv2"
#define DEVICE_COUNT 2
#define BUF_SIZE 128

static dev_t     first_dev;           // major/minor inicial
static struct cdev drv2_cdev;
static struct class *drv2_class;
static char      message[BUF_SIZE] = "Mensaje inicial desde drv2\n";
static size_t    msg_len;

// fops:
static ssize_t drv2_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    if (*ppos >= msg_len) return 0;
    if (count > msg_len - *ppos) count = msg_len - *ppos;
    if (copy_to_user(buf, message + *ppos, count)) return -EFAULT;
    *ppos += count;
    return count;
}

static ssize_t drv2_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    if (count >= BUF_SIZE) return -EINVAL;
    if (copy_from_user(message, buf, count)) return -EFAULT;
    msg_len = count;
    return count;
}

static int drv2_open(struct inode *inode, struct file *file) {  
    msg_len = strlen(message);
    return 0;
}
static int drv2_release(struct inode *inode, struct file *file) {
    return 0;
}

static const struct file_operations drv2_fops = {
    .owner   = THIS_MODULE,
    .read    = drv2_read,
    .write   = drv2_write,
    .open    = drv2_open,
    .release = drv2_release,
};

static int __init drv2_init(void) {
    int ret, i;
    // 1) Reserva major/minor automáticamente para 2 dispositivos
    ret = alloc_chrdev_region(&first_dev, 0, DEVICE_COUNT, DRIVER_NAME);
    if (ret) {
        pr_err(DRIVER_NAME ": no puedo alloc_chrdev_region (%d)\n", ret);
        return ret;
    }
    // 2) cdev init + add
    cdev_init(&drv2_cdev, &drv2_fops);
    drv2_cdev.owner = THIS_MODULE;
    ret = cdev_add(&drv2_cdev, first_dev, DEVICE_COUNT);
    if (ret) {
        unregister_chrdev_region(first_dev, DEVICE_COUNT);
        pr_err(DRIVER_NAME ": cdev_add fallo (%d)\n", ret);
        return ret;
    }
    // 3) crea la clase para udev
    drv2_class = class_create(DRIVER_NAME);
    if (IS_ERR(drv2_class)) {
        cdev_del(&drv2_cdev);
        unregister_chrdev_region(first_dev, DEVICE_COUNT);
        return PTR_ERR(drv2_class);
    }
    // 4) crea dos dispositivos /dev/drv2_0 y /dev/drv2_1
    for (i = 0; i < DEVICE_COUNT; i++) {
        device_create(drv2_class, NULL, MKDEV(MAJOR(first_dev), MINOR(first_dev)+i),
                      NULL, DRIVER_NAME "_%d", i);
    }
    pr_info(DRIVER_NAME ": cargado, major=%d\n", MAJOR(first_dev));
    return 0;
}

static void __exit drv2_exit(void) {
    int i;
    // destruye dispositivos y clase
    for (i = DEVICE_COUNT-1; i >= 0; i--) {
        device_destroy(drv2_class, MKDEV(MAJOR(first_dev), MINOR(first_dev)+i));
    }
    class_destroy(drv2_class);
    cdev_del(&drv2_cdev);
    unregister_chrdev_region(first_dev, DEVICE_COUNT);
    pr_info(DRIVER_NAME ": descargado\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TuNombre");
MODULE_DESCRIPTION("Ejemplo drv2 con dos minor y udev");
module_init(drv2_init);
module_exit(drv2_exit);
