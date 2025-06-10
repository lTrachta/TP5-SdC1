#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUF_LEN 80

static int major;
static char message[BUF_LEN] = "Hola desde el kernel!\n";
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
};

static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "drv1: Dispositivo abierto\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "drv1: Dispositivo cerrado\n");
    return 0;
}

static ssize_t dev_read(struct file *filp, char *buffer, size_t len, loff_t *offset) {
    int bytes_read = 0;
    char *msg_ptr = message;

    while (len && *msg_ptr) {
        put_user(*msg_ptr++, buffer++);
        len--;
        bytes_read++;
    }
    
    return bytes_read;
}

static int __init drv1_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Fallo al registrar el dispositivo\n");
        return major;
    }
    printk(KERN_INFO "drv1: registrado con major %d\n", major);
    return 0;
}

static void __exit drv1_exit(void) {
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "drv1: descargado\n");
}

module_init(drv1_init);
module_exit(drv1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TuNombre");
MODULE_DESCRIPTION("Un simple driver de caracter");
