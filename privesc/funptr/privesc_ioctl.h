#ifndef PRIVESC_IOCTL_H
#define PRIVESC_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define PRIVESC_MAGIC 'p'
#define PRIVESC_CALL  _IO(PRIVESC_MAGIC, 1)

#endif
