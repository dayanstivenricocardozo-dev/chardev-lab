#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "vuln_device"
#define CLASS_NAME  "vuln"

/*
 * Estado vulnerable.
 *
 * buf[64] es el buffer pequeño.
 * admin es la variable privilegiada simulada.
 * guard es espacio extra para reducir el riesgo de corrupción fuera del struct.
 */
struct vuln_state {
    char buf[64];
    int admin;
    char guard[128];
};

static struct vuln_state state;

static dev_t dev_number;
static struct cdev vuln_cdev;
static struct class *vuln_class;
static struct device *vuln_device;

/*
 * open()
 */
static int vuln_open(struct inode *inode, struct file *file)
{
    pr_info("vuln_device: open()\n");
    return 0;
}

/*
 * release()
 */
static int vuln_release(struct inode *inode, struct file *file)
{
    pr_info("vuln_device: release()\n");
    return 0;
}

/*
 * read()
 *
 * Devuelve el valor actual de admin.
 */
static ssize_t vuln_read(struct file *file,
                         char __user *user_buffer,
                         size_t count,
                         loff_t *offset)
{
    char tmp[64];
    int len;
    size_t remaining;

    len = snprintf(tmp, sizeof(tmp), "admin=%d\n", state.admin);
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
 * AQUÍ VA LA VULNERABILIDAD.
 *
 * Tu tarea:
 * Copiar count bytes desde userspace hacia state.buf,
 * SIN validar si count cabe dentro de state.buf.
 *
 * Objetivo del laboratorio:
 * Si count > 64, el exceso debe sobrescribir state.admin.
 */



static ssize_t vuln_write(struct file *file,
                          const char __user *user_buffer,
                          size_t count,
                          loff_t *offset)
{
    /*
     * TODO: implementar copia insegura.
     *
     * Pista:
     * Un bucle for desde i = 0 hasta count - 1.
     * Usar get_user() para leer cada byte desde user_buffer + i.
     * Guardar ese byte en state.buf[i].
     *
     * No agregar ninguna validación de tamaño.
     */
    if (count > sizeof(state.buf)){
        count = sizeof(state.buf);

    }

    for (int i = 0; i < count; i++ ){
        int user;
        char var_destino;
        user = get_user(var_destino, user_buffer + i);
        if (user != 0){
            return -EFAULT;
        }
        
        state.buf[i] = var_destino;
        
    }


    return count;
}

/*
 * Operaciones del char device
 */
static const struct file_operations vuln_fops = {
    .owner   = THIS_MODULE,
    .open    = vuln_open,
    .read    = vuln_read,
    .write   = vuln_write,
    .release = vuln_release,
};

/*
 * Inicialización del módulo
 */
static int __init vuln_init(void)
{
    int ret;

    pr_info("vuln_device: inicializando\n");

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("vuln_device: no se pudo reservar chrdev region\n");
        return ret;
    }

    pr_info("vuln_device: Major: %d, Minor: %d\n",
            MAJOR(dev_number), MINOR(dev_number));

    cdev_init(&vuln_cdev, &vuln_fops);
    vuln_cdev.owner = THIS_MODULE;

    ret = cdev_add(&vuln_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("vuln_device: no se pudo agregar cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    vuln_class = class_create(CLASS_NAME);
    if (IS_ERR(vuln_class)) {
        pr_err("vuln_device: no se pudo crear clase\n");
        cdev_del(&vuln_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(vuln_class);
    }

    vuln_device = device_create(vuln_class,
                                NULL,
                                dev_number,
                                NULL,
                                DEVICE_NAME);
    if (IS_ERR(vuln_device)) {
        pr_err("vuln_device: no se pudo crear dispositivo\n");
        class_destroy(vuln_class);
        cdev_del(&vuln_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(vuln_device);
    }

    state.admin = 0;

    pr_info("vuln_device: creado correctamente\n");

    return 0;
}

/*
 * Limpieza del módulo
 */
static void __exit vuln_exit(void)
{
    device_destroy(vuln_class, dev_number);
    class_destroy(vuln_class);
    cdev_del(&vuln_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("vuln_device: eliminado\n");
}

module_init(vuln_init);
module_exit(vuln_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab local");
MODULE_DESCRIPTION("Driver vulnerable educativo");
MODULE_VERSION("0.1");