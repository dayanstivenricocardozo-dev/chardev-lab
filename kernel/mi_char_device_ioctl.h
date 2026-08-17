#ifndef MI_CHAR_DEVICE_IOCTL_H
#define MI_CHAR_DEVICE_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define MI_IOC_MAGIC 'm'

/*
 * MI_IOC_GET_SIZE
 *
 * Devuelve el tamaño actual del buffer privado.
 * Dirección: driver → userspace
 */
#define MI_IOC_GET_SIZE _IOR(MI_IOC_MAGIC, 1, unsigned int)

/*
 * MI_IOC_CLEAR
 *
 * Borra el buffer privado.
 * Dirección: sin datos
 */
#define MI_IOC_CLEAR _IO(MI_IOC_MAGIC, 2)

#endif