#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "mi_char_device_ioctl.h"

#define DEVICE_NAME "mi_char_device"
#define CLASS_NAME  "mi_char"
#define BUFFER_SIZE 1024

static dev_t dev_number;
static struct cdev mi_cdev;
static struct class *mi_class;
static struct device *mi_device;

/*
 * Datos privados por apertura.
 *
 * Cada proceso que abre el dispositivo recibe
 * su propio buffer, su propio tamaño y su propio mutex.
 *
 * Esto elimina la fuga de información entre procesos.
 */
struct mi_dev_data {
    char buffer[BUFFER_SIZE];
    size_t size;
    struct mutex lock;
};

/*
 * open()
 *
 * Reserva memoria para los datos privados de esta apertura.
 */
static int mi_open(struct inode *inode, struct file *file)
{
    struct mi_dev_data *data;

    data = kmalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    mutex_init(&data->lock);
    data->size = 0;
    memset(data->buffer, 0, BUFFER_SIZE);

    file->private_data = data;

    pr_info("mi_char_device: open()\n");
    return 0;
}

/*
 * release()
 *
 * Libera la memoria reservada en open().
 */
static int mi_release(struct inode *inode, struct file *file)
{
    struct mi_dev_data *data = file->private_data;

    kfree(data);

    pr_info("mi_char_device: release()\n");
    return 0;
}

/*
 * read()
 *
 * Lee del buffer privado de esta apertura.
 * La verificación de offset va DENTRO del mutex
 * para evitar condiciones de carrera.
 */
static ssize_t mi_read(struct file *file,
                       char __user *user_buffer,
                       size_t count,
                       loff_t *offset)
{
    struct mi_dev_data *data = file->private_data;
    size_t bytes_to_read;

    mutex_lock(&data->lock);

    if (*offset >= data->size) {
        mutex_unlock(&data->lock);
        return 0;
    }

    if (count > data->size - *offset)
        count = data->size - *offset;

    bytes_to_read = count;

    if (copy_to_user(user_buffer,
                     data->buffer + *offset,
                     bytes_to_read)) {
        mutex_unlock(&data->lock);
        return -EFAULT;
    }

    *offset += bytes_to_read;
    mutex_unlock(&data->lock);

    pr_info("mi_char_device: read() -> %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

/*
 * write()
 *
 * Escribe en el buffer privado de esta apertura.
 */
static ssize_t mi_write(struct file *file,
                        const char __user *user_buffer,
                        size_t count,
                        loff_t *offset)
{
    struct mi_dev_data *data = file->private_data;
    size_t bytes_to_write;

    if (count > BUFFER_SIZE - 1)
        bytes_to_write = BUFFER_SIZE - 1;
    else
        bytes_to_write = count;

    mutex_lock(&data->lock);

    if (copy_from_user(data->buffer,
                       user_buffer,
                       bytes_to_write)) {
        mutex_unlock(&data->lock);
        return -EFAULT;
    }

    data->buffer[bytes_to_write] = '\0';
    data->size = bytes_to_write;

    mutex_unlock(&data->lock);

    pr_info("mi_char_device: write() <- %zu bytes\n", bytes_to_write);
    return bytes_to_write;
}

/*
 * mi_ioctl()
 *
 * Comandos personalizados del dispositivo.
 */
static long mi_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    struct mi_dev_data *data = file->private_data;

    switch (cmd) {

    case MI_IOC_GET_SIZE: {
        unsigned int size;

        mutex_lock(&data->lock);
        size = (unsigned int)data->size;
        mutex_unlock(&data->lock);

        if (copy_to_user((unsigned int __user *)arg,
                         &size,
                         sizeof(size))) {
            return -EFAULT;
        }

        return 0;
    }

    case MI_IOC_CLEAR:
        mutex_lock(&data->lock);
        data->size = 0;
        data->buffer[0] = '\0';
        mutex_unlock(&data->lock);
        return 0;

    default:
        return -ENOTTY;
    }
}


/*
 * Operaciones del char device
 */
static const struct file_operations mi_fops = {
    .owner          = THIS_MODULE,
    .open           = mi_open,
    .read           = mi_read,
    .write          = mi_write,
    .release        = mi_release,
    .llseek         = default_llseek,
    .unlocked_ioctl = mi_ioctl,
};

/*
 * Inicialización del módulo
 */
static int __init mi_init(void)
{
    int ret;

    pr_info("mi_char_device: inicializando\n");

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("No se pudo reservar el número de dispositivo\n");
        return ret;
    }

    pr_info("Major: %d, Minor: %d\n",
            MAJOR(dev_number), MINOR(dev_number));

    cdev_init(&mi_cdev, &mi_fops);
    mi_cdev.owner = THIS_MODULE;

    ret = cdev_add(&mi_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("No se pudo registrar cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    mi_class = class_create(CLASS_NAME);
    if (IS_ERR(mi_class)) {
        pr_err("No se pudo crear la clase\n");
        cdev_del(&mi_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(mi_class);
    }

    mi_device = device_create(mi_class,
                              NULL,
                              dev_number,
                              NULL,
                              DEVICE_NAME);
    if (IS_ERR(mi_device)) {
        pr_err("No se pudo crear el dispositivo\n");
        class_destroy(mi_class);
        cdev_del(&mi_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(mi_device);
    }

    pr_info("mi_char_device: creado correctamente\n");
    return 0;
}

/*
 * Limpieza del módulo
 */
static void __exit mi_exit(void)
{
    device_destroy(mi_class, dev_number);
    class_destroy(mi_class);
    cdev_del(&mi_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("mi_char_device: eliminado\n");
}

module_init(mi_init);
module_exit(mi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ejemplo");
MODULE_DESCRIPTION("Char device con private_data para aislamiento por proceso");
MODULE_VERSION("2.0");