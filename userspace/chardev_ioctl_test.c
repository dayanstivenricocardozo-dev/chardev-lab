#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

/*
 * Incluimos el header compartido con el driver.
 * Ese header define MI_IOC_GET_SIZE y MI_IOC_CLEAR.
 */
#include "../kernel/mi_char_device_ioctl.h"

#define DEVICE_PATH "/dev/mi_char_device"
#define TEST_MSG "hola ioctl"

int main(void)
{
    /* Paso 1: abrir el dispositivo */
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* Paso 2: escribir un mensaje */
    ssize_t w = write(fd, TEST_MSG, strlen(TEST_MSG));
    if (w == -1) {
        perror("write");
        close(fd);
        return 1;
    }
    printf("Escritos %zd bytes: \"%s\"\n", w, TEST_MSG);

    /* Paso 3: ioctl GET_SIZE */
    unsigned int size = 0;
    if (ioctl(fd, MI_IOC_GET_SIZE, &size) == -1) {
        perror("ioctl GET_SIZE");
        close(fd);
        return 1;
    }
    printf("GET_SIZE: %u bytes\n", size);

    /* Paso 4: ioctl CLEAR */
    if (ioctl(fd, MI_IOC_CLEAR) == -1) {
        perror("ioctl CLEAR");
        close(fd);
        return 1;
    }
    printf("CLEAR: buffer borrado\n");

    /* Paso 5: ioctl GET_SIZE de nuevo */
    size = 999;
    if (ioctl(fd, MI_IOC_GET_SIZE, &size) == -1) {
        perror("ioctl GET_SIZE after CLEAR");
        close(fd);
        return 1;
    }
    printf("GET_SIZE after CLEAR: %u bytes\n", size);

    /* Paso 6: cerrar */
    close(fd);

    /* Paso 7: verificar resultado */
    if (size == 0) {
        printf("PASS\n");
        return 0;
    } else {
        printf("FAIL: se esperaba 0 después de CLEAR\n");
        return 1;
    }
}