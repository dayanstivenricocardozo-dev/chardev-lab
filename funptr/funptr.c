#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "funptr_ioctl.h"

#define DEVICE_NAME "funptr_lab"
#define CLASS_NAME  "funptr"

/*
 * Tipo de puntero a función.
 *
 * handler_t es un puntero a una función que no recibe
 * argumentos y no devuelve nada.
 */
typedef void (*handler_t)(void);

/*
 * Función normal.
 *
 * Esta es la función que handler apunta cuando el módulo está sano.
 */
static void normal_handler(void)
{
    pr_info("funptr: normal handler ejecutado\n");
}

/*
 * Función objetivo del exploit.
 *
 * Si el atacante logra que handler apunte aquí,
 * demuestra que controla el flujo de ejecución.
 */
static void win_handler(void)
{
    pr_info("funptr: *** WIN handler ejecutado ***\n");
}

/*
 * Estado vulnerable.
 *
 * buf[64] es el buffer pequeño.
 * handler es el puntero de función que está al lado del buffer.
 * guard es espacio extra para reducir riesgo de corrupción fuera del struct.
 */
struct funptr_state {
    char buf[64];
    handler_t handler;
    char guard[128];
};

static struct funptr_state state;

static dev_t dev_number;
static struct cdev funptr_cdev;
static struct class *funptr_class;
static struct device *funptr_device;

/*
 * open()
 */
static int funptr_open(struct inode *inode, struct file *file)
{
    pr_info("funptr: open()\n");
    return 0;
}

/*
 * release()
 */
static int funptr_release(struct inode *inode, struct file *file)
{
    pr_info("funptr: release()\n");
    return 0;
}

/*
 * read()
 *
 * Filtra las direcciones de handler y win_handler.
 *
 * Esto simula una vulnerabilidad de fuga de información.
 * En un sistema real, estas direcciones estarían protegidas.
 */
static ssize_t funptr_read(struct file *file,
                           char __user *user_buffer,
                           size_t count,
                           loff_t *offset)
{
    char tmp[128];
    int len;
    size_t remaining;

    len = snprintf(tmp, sizeof(tmp),
                   "handler=0x%lx win=0x%lx\n",
                   (unsigned long)state.handler,
                   (unsigned long)win_handler);

    if (len < 0)
        return -EINVAL;

    if (*offset >= (loff_t)len)
        return 0;

    remaining = (size_t)(len - *offset);

    if (count > remaining)
        count = remaining;

    if (copy_to_user(user_buffer, tmp + *offset, count))
        return -EFAULT;

    *offset += count;

    return count;
}

/*
 * write()
 *
 * VULNERABILIDAD INTENCIONAL.
 *
 * Copia count bytes desde userspace hacia state.buf
 * SIN validar si count cabe dentro de state.buf.
 *
 * Si count > 64, el exceso sobrescribe state.handler.
 */
static ssize_t funptr_write(struct file *file,
                            const char __user *user_buffer,
                            size_t count,
                            loff_t *offset)
{
    size_t i;

    for (i = 0; i < count; i++) {
        char c;

        if (get_user(c, user_buffer + i))
            return -EFAULT;

        state.buf[i] = c;
    }

    return count;
}

/*
 * ioctl()
 *
 * FUNPTR_CALL ejecuta el puntero de función handler.
 *
 * Si handler apunta a normal_handler, se ejecuta normal_handler.
 * Si el atacante sobrescribió handler con la dirección de win_handler,
 * se ejecuta win_handler.
 */
static long funptr_ioctl(struct file *file,
                         unsigned int cmd,
                         unsigned long arg)
{
    switch (cmd) {

    case FUNPTR_CALL:
        if (state.handler) {
            state.handler();
        }
        return 0;

    default:
        return -ENOTTY;
    }
}

/*
 * Operaciones del char device
 */
static const struct file_operations funptr_fops = {
    .owner          = THIS_MODULE,
    .open           = funptr_open,
    .read           = funptr_read,
    .write          = funptr_write,
    .release        = funptr_release,
    .unlocked_ioctl = funptr_ioctl,
};

/*
 * Inicialización del módulo
 */
static int __init funptr_init(void)
{
    int ret;

    pr_info("funptr: inicializando\n");

    /*
     * Inicializar handler con la función normal.
     */
    state.handler = normal_handler;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("funptr: no se pudo reservar chrdev region\n");
        return ret;
    }

    pr_info("funptr: Major: %d, Minor: %d\n",
            MAJOR(dev_number), MINOR(dev_number));

    cdev_init(&funptr_cdev, &funptr_fops);
    funptr_cdev.owner = THIS_MODULE;

    ret = cdev_add(&funptr_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("funptr: no se pudo agregar cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    funptr_class = class_create(CLASS_NAME);
    if (IS_ERR(funptr_class)) {
        pr_err("funptr: no se pudo crear clase\n");
        cdev_del(&funptr_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(funptr_class);
    }

    funptr_device = device_create(funptr_class,
                                  NULL,
                                  dev_number,
                                  NULL,
                                  DEVICE_NAME);
    if (IS_ERR(funptr_device)) {
        pr_err("funptr: no se pudo crear dispositivo\n");
        class_destroy(funptr_class);
        cdev_del(&funptr_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(funptr_device);
    }

    pr_info("funptr: creado correctamente\n");
    pr_info("funptr: handler=0x%lx win=0x%lx\n",
            (unsigned long)state.handler,
            (unsigned long)win_handler);

    return 0;
}

/*
 * Limpieza del módulo
 */
static void __exit funptr_exit(void)
{
    device_destroy(funptr_class, dev_number);
    class_destroy(funptr_class);
    cdev_del(&funptr_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("funptr: eliminado\n");
}

module_init(funptr_init);
module_exit(funptr_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab local");
MODULE_DESCRIPTION("Lab educativo de function pointer hijack");
MODULE_VERSION("0.1");