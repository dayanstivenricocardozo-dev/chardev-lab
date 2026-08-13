#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "mi_char_device"
#define CLASS_NAME  "mi_char"

#define BUFFER_SIZE 1024

static dev_t dev_number;
static struct cdev mi_cdev;
static struct class *mi_class;
static struct device *mi_device;

static char device_buffer[BUFFER_SIZE];
static size_t buffer_size;

static DEFINE_MUTEX(device_mutex);

/*
 * open()
 */
static int mi_open(struct inode *inode, struct file *file)
{
    pr_info("mi_char_device: open()\n");
    return 0;
}

/*
 * release()
 */
static int mi_release(struct inode *inode, struct file *file)
{
    pr_info("mi_char_device: release()\n");
    return 0;
}

/*
 * read()
 */
static ssize_t mi_read(struct file *file,
                       char __user *user_buffer,
                       size_t count,
                       loff_t *offset)
{
    size_t bytes_to_read;

    if (*offset >= buffer_size)
        return 0;

    if (count > buffer_size - *offset)
        count = buffer_size - *offset;

    mutex_lock(&device_mutex);

    bytes_to_read = count;

    if (copy_to_user(user_buffer,
                     device_buffer + *offset,
                     bytes_to_read)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    *offset += bytes_to_read;

    mutex_unlock(&device_mutex);

    pr_info("mi_char_device: read() -> %zu bytes\n",
            bytes_to_read);

    return bytes_to_read;
}

/*
 * write()
 */
static ssize_t mi_write(struct file *file,
                        const char __user *user_buffer,
                        size_t count,
                        loff_t *offset)
{
    size_t bytes_to_write;

    if (count > BUFFER_SIZE - 1)
        bytes_to_write = BUFFER_SIZE - 1;
    else
        bytes_to_write = count;

    mutex_lock(&device_mutex);

    if (copy_from_user(device_buffer,
                       user_buffer,
                       bytes_to_write)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    device_buffer[bytes_to_write] = '\0';
    buffer_size = bytes_to_write;

    mutex_unlock(&device_mutex);

    pr_info("mi_char_device: write() <- %zu bytes\n",
            bytes_to_write);

    return bytes_to_write;
}

/*
 * Operaciones del char device
 */
static const struct file_operations mi_fops = {
    .owner   = THIS_MODULE,
    .open    = mi_open,
    .read    = mi_read,
    .write   = mi_write,
    .release = mi_release,
};

/*
 * Inicialización del módulo
 */
static int __init mi_init(void)
{
    int ret;

    pr_info("mi_char_device: inicializando\n");

    /*
     * 1. Reservar número mayor/menor dinámicamente
     */
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("No se pudo reservar el número de dispositivo\n");
        return ret;
    }

    pr_info("Major: %d, Minor: %d\n",
            MAJOR(dev_number),
            MINOR(dev_number));

    /*
     * 2. Inicializar cdev
     */
    cdev_init(&mi_cdev, &mi_fops);
    mi_cdev.owner = THIS_MODULE;

    /*
     * 3. Registrar cdev
     */
    ret = cdev_add(&mi_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("No se pudo registrar cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    /*
     * 4. Crear class
     */
    mi_class = class_create(CLASS_NAME);

    if (IS_ERR(mi_class)) {
        pr_err("No se pudo crear la clase\n");

        cdev_del(&mi_cdev);
        unregister_chrdev_region(dev_number, 1);

        return PTR_ERR(mi_class);
    }

    /*
     * 5. Crear dispositivo
     *
     * Normalmente udev creará:
     *
     * /dev/mi_char_device
     */
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
MODULE_DESCRIPTION("Char device Linux con read/write");
MODULE_VERSION("1.0");
