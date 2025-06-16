// cdd.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/device.h>

static int signal_values[2] = {0, 0};
static int selected_signal = 0;
static struct task_struct *sensor_thread;
static DEFINE_MUTEX(signal_mutex);

// Hilo del kernel: simula señales
static int sensor_thread_fn(void *data) {
    int counter = 0;
    while (!kthread_should_stop()) {
        mutex_lock(&signal_mutex);
        signal_values[0] = counter % 2;
        signal_values[1] = (counter / 2) % 2;
        mutex_unlock(&signal_mutex);
        counter++;
        ssleep(1);
    }
    return 0;
}

// Función de lectura para user space
static ssize_t cdd_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) {
    char buffer[8];
    int value;

    mutex_lock(&signal_mutex);
    value = signal_values[selected_signal];
    mutex_unlock(&signal_mutex);

    snprintf(buffer, sizeof(buffer), "%d\n", value);
    return simple_read_from_buffer(buf, len, ppos, buffer, strlen(buffer));
}

// Sysfs para seleccionar señal
static ssize_t select_signal_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count) {
    int val;
    if (kstrtoint(buf, 10, &val) == 0 && (val == 0 || val == 1)) {
        mutex_lock(&signal_mutex);
        selected_signal = val;
        mutex_unlock(&signal_mutex);
    }
    return count;
}

static ssize_t select_signal_show(struct class *class, struct class_attribute *attr, char *buf) {
    return scnprintf(buf, PAGE_SIZE, "%d\n", selected_signal);
}

CLASS_ATTR_RW(select_signal);

// Dispositivo de caracteres
static struct file_operations cdd_fops = {
    .owner = THIS_MODULE,
    .read = cdd_read,
};

static struct miscdevice cdd_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "cdd_sensor",
    .fops = &cdd_fops,
    .mode = 0666
};

static struct class *cdd_class;

static int __init cdd_init(void) {
    int ret;

    ret = misc_register(&cdd_misc);
    if (ret)
        return ret;

    cdd_class = class_create(THIS_MODULE, "cdd");
    if (IS_ERR(cdd_class)) {
        misc_deregister(&cdd_misc);
        return PTR_ERR(cdd_class);
    }

    class_create_file(cdd_class, &class_attr_select_signal);

    sensor_thread = kthread_run(sensor_thread_fn, NULL, "cdd_sensor_thread");
    if (IS_ERR(sensor_thread)) {
        class_remove_file(cdd_class, &class_attr_select_signal);
        class_destroy(cdd_class);
        misc_deregister(&cdd_misc);
        return PTR_ERR(sensor_thread);
    }

    pr_info("CDD iniciado correctamente\n");
    return 0;
}

static void __exit cdd_exit(void) {
    kthread_stop(sensor_thread);
    class_remove_file(cdd_class, &class_attr_select_signal);
    class_destroy(cdd_class);
    misc_deregister(&cdd_misc);
    pr_info("CDD removido\n");
}

module_init(cdd_init);
module_exit(cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TP Sistemas de Computación");
MODULE_DESCRIPTION("CDD para lectura de señales simuladas en QEMU");