#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEVICE_PATH "/dev/mi_char_device"
#define TEST_MSG "hola driver"

int main(void)
{
    /* Paso 1: abrir el dispositivo con open() */

    int fd = open(DEVICE_PATH, O_RDWR);
        if (fd < 0) {
        perror("open");
        return 1;
}


    /* Paso 2: escribir TEST_MSG con write() */

    size_t tamaño = strlen(TEST_MSG);
    ssize_t bytes_escritos = write(fd, TEST_MSG, tamaño);
        if (bytes_escritos == -1){
            perror("write");
            return 1;
        }
        
        if (bytes_escritos != (ssize_t)tamaño){
            printf("no se escribieron todos los bytes\n");
            return 1;
        }
        

    /* Paso 3: leer el buffer con read() */
    char buffer [sizeof(TEST_MSG)];

    size_t tamaño_buffer = sizeof(buffer) - 1;
    ssize_t bytes_leidos = read(fd, buffer, tamaño_buffer);
        if (bytes_leidos == -1){
            perror("read");
            return 1;
        }
        if (bytes_leidos != (ssize_t)tamaño){
            return 1;
        }
    /* Paso 4: comparar lo leído con TEST_MSG */
    buffer[bytes_leidos] = '\0';
        if (strcmp(buffer, TEST_MSG) != 0) {
        printf("el contenido leido no coincide con TEST_MSG\n");
        return 1;
}
    

    /* Paso 5: cerrar el dispositivo con close() */

    int resultado_close = close(fd);
        if (resultado_close == -1){
            return 1;
        }

    printf("PASS\n");

    return 0;
}