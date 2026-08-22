#ifndef FUNPTR_IOCTL_H
#define FUNPTR_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define FUNPTR_MAGIC 'f'

/*
 * FUNPTR_CALL
 *
 * Ejecuta el puntero de función handler.
 * No transmite datos.
 */
#define FUNPTR_CALL _IO(FUNPTR_MAGIC, 1)

#endif