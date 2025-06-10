#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "clipboard"
#define BUF_SIZE  256

static char buffer[BUF_SIZE];
static size_t buf_len = 0;
static struct proc_dir_entry *proc_entry;

// Write callback: copia del usuario al buffer
static ssize_t clipboard_write(struct file *file, const char __user *ubuf,
                               size_t count, loff_t *ppos)
{
    size_t to_copy = min(count, (size_t)(BUF_SIZE - 1));
    if (copy_from_user(buffer, ubuf, to_copy))
        return -EFAULT;
    buffer[to_copy] = '\0';
    buf_len = to_copy;
    pr_info("clipboard: almacenado %zu bytes\n", to_copy);
    return to_copy;
}

// Read callback: copia del buffer al usuario
static ssize_t clipboard_read(struct file *file, char __user *ubuf,
                              size_t count, loff_t *ppos)
{
    size_t to_copy = min(count, buf_len - (size_t)(*ppos));
    if (to_copy == 0)
        return 0;
    if (copy_to_user(ubuf, buffer + *ppos, to_copy))
        return -EFAULT;
    *ppos += to_copy;
    pr_info("clipboard: leído %zu bytes\n", to_copy);
    return to_copy;
}

// Para kernels modernos usamos proc_ops
static const struct proc_ops clipboard_ops = {
    .proc_read  = clipboard_read,
    .proc_write = clipboard_write,
};

static int __init clipboard_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0666, NULL, &clipboard_ops);
    if (!proc_entry) {
        pr_err("clipboard: fallo al crear /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    pr_info("clipboard: módulo cargado, /proc/%s creado\n", PROC_NAME);
    return 0;
}

static void __exit clipboard_exit(void)
{
    proc_remove(proc_entry);
    pr_info("clipboard: módulo descargado, /proc/%s eliminado\n", PROC_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TuNombre");
MODULE_DESCRIPTION("Módulo /proc/clipboard para copiar/pegar texto");
module_init(clipboard_init);
module_exit(clipboard_exit);
