#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <linux/capability.h>

#include "privesc_ioctl.h"

#define DEVICE_NAME "privesc_lab"
#define CLASS_NAME  "privesc"

/*
 * Tipo de puntero a función.
 */
typedef void (*handler_t)(void);

/*
 * Función normal.
 *
 * Handler apunta aquí cuando el módulo está sano.
 */
static void normal_handler(void)
{
    pr_info("privesc: normal handler ejecutado\n");
}

static void root_shell(void)
{
    struct cred *new_creds;
    int ret;

    pr_info("privesc: ejecutando root_shell\n");

    /*
     * prepare_creds() crea una copia de las credenciales
     * del proceso actual.
     */
    new_creds = prepare_creds();
    if (!new_creds) {
        pr_err("privesc: fallo en prepare_creds\n");
        return;
    }

    /*
     * Cambiar todos los IDs a 0.
     */
    new_creds->uid.val = 0;
    new_creds->gid.val = 0;
    new_creds->suid.val = 0;
    new_creds->sgid.val = 0;
    new_creds->euid.val = 0;
    new_creds->egid.val = 0;
    new_creds->fsuid.val = 0;
    new_creds->fsgid.val = 0;

    /*
     * Dar capacidades completas.
     *
     * Sin capacidades, UID 0 puede no ser suficiente
     * en kernels modernos.
     */
    new_creds->cap_inheritable = CAP_FULL_SET;
    new_creds->cap_permitted = CAP_FULL_SET;
    new_creds->cap_effective = CAP_FULL_SET;
    new_creds->cap_bset = CAP_FULL_SET;
    new_creds->cap_ambient = CAP_FULL_SET;

    /*
     * Aplicar las credenciales al proceso actual.
     */
    ret = commit_creds(new_creds);
    if (ret < 0) {
        pr_err("privesc: fallo en commit_creds: %d\n", ret);
        return;
    }

    pr_info("privesc: credenciales cambiadas a root\n");
}
/*
 * Estado vulnerable.
 *
 * buf[64] es el buffer pequeño.
 * handler es el puntero de función adyacente.
 * guard es espacio extra.
 */
struct privesc_state {
    char buf[64];
    handler_t handler;
    char guard[128];
};

static struct privesc_state state;

static dev_t dev_number;
static struct cdev privesc_cdev;
static struct class *privesc_class;
static struct device *privesc_device;

/*
 * open()
 */
static int privesc_open(struct inode *inode, struct file *file)
{
    pr_info("privesc: open()\n");
    return 0;
}

/*
 * release()
 */
static int privesc_release(struct inode *inode, struct file *file)
{
    pr_info("privesc: release()\n");
    return 0;
}

/*
 * read()
 *
 * Filtra las direcciones de handler y root_shell.
 * Simula una vulnerabilidad de fuga de información.
 */
static ssize_t privesc_read(struct file *file,
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
                   (unsigned long)root_shell);

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
 * Copia count bytes SIN validar tamaño.
 */
static ssize_t privesc_write(struct file *file,
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
 * PRIVESC_CALL ejecuta el puntero de función handler.
 */
static long privesc_ioctl(struct file *file,
                          unsigned int cmd,
                          unsigned long arg)
{
    switch (cmd) {

    case PRIVESC_CALL:
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
static const struct file_operations privesc_fops = {
    .owner          = THIS_MODULE,
    .open           = privesc_open,
    .read           = privesc_read,
    .write          = privesc_write,
    .release        = privesc_release,
    .unlocked_ioctl = privesc_ioctl,
};

/*
 * Inicialización
 */
static int __init privesc_init(void)
{
    int ret;

    pr_info("privesc: inicializando\n");

    state.handler = normal_handler;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("privesc: no se pudo reservar chrdev region\n");
        return ret;
    }

    pr_info("privesc: Major: %d, Minor: %d\n",
            MAJOR(dev_number), MINOR(dev_number));

    cdev_init(&privesc_cdev, &privesc_fops);
    privesc_cdev.owner = THIS_MODULE;

    ret = cdev_add(&privesc_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("privesc: no se pudo agregar cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    privesc_class = class_create(CLASS_NAME);
    if (IS_ERR(privesc_class)) {
        pr_err("privesc: no se pudo crear clase\n");
        cdev_del(&privesc_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(privesc_class);
    }

    privesc_device = device_create(privesc_class,
                                   NULL,
                                   dev_number,
                                   NULL,
                                   DEVICE_NAME);
    if (IS_ERR(privesc_device)) {
        pr_err("privesc: no se pudo crear dispositivo\n");
        class_destroy(privesc_class);
        cdev_del(&privesc_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(privesc_device);
    }

    pr_info("privesc: creado correctamente\n");
    pr_info("privesc: handler=0x%lx root_shell=0x%lx\n",
            (unsigned long)state.handler,
            (unsigned long)root_shell);

    return 0;
}

/*
 * Limpieza
 */
static void __exit privesc_exit(void)
{
    device_destroy(privesc_class, dev_number);
    class_destroy(privesc_class);
    cdev_del(&privesc_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("privesc: eliminado\n");
}

module_init(privesc_init);
module_exit(privesc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab local");
MODULE_DESCRIPTION("Lab educativo de escalada de privilegios");
MODULE_VERSION("0.1");