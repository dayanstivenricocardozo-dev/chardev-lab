#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEVICE_PATH "/dev/mi_char_device"
#define NUM_CHILDREN 2

int main(void)
{
    /* Paso 1: crear un pipe */
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    /* Paso 2: crear NUM_CHILDREN hijos con fork() */
    for (int i = 0; i < NUM_CHILDREN; i++) {

        int resultado_fork = fork();

        if (resultado_fork == -1) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            return 1;
        }

        if (resultado_fork == 0) {

            /* El hijo solo escribe en el pipe */
            close(pipefd[0]);

            char mensaje[6];

            if (i == 0) {
                strcpy(mensaje, "hola");
            } else {
                strcpy(mensaje, "adios");
            }

            /* Paso 3: abrir el dispositivo */
            int fd = open(DEVICE_PATH, O_RDWR);

            if (fd == -1) {
                perror("open");

                const char *error = "ERROR: no se pudo abrir el dispositivo\n";
                write(pipefd[1], error, strlen(error));

                close(pipefd[1]);
                exit(1);
            }

            /* Escribir en el dispositivo */
            ssize_t resultado_write = write(
                fd,
                mensaje,
                strlen(mensaje)
            );

            if (resultado_write == -1) {
                perror("write");

                const char *error = "ERROR: fallo en write\n";
                write(pipefd[1], error, strlen(error));

                close(fd);
                close(pipefd[1]);
                exit(1);
            }

            /* Comprobar que se escribieron todos los bytes */
            if (resultado_write != (ssize_t)strlen(mensaje)) {

                const char *error = "ERROR: escritura incompleta\n";
                write(pipefd[1], error, strlen(error));

                close(fd);
                close(pipefd[1]);
                exit(1);
            }

            /* Leer del dispositivo */
            char buffer[6];

            ssize_t resultado_read = read(
                fd,
                buffer,
                sizeof(buffer) - 1
            );

            if (resultado_read == -1) {
                perror("read");

                const char *error = "ERROR: fallo en read\n";
                write(pipefd[1], error, strlen(error));

                close(fd);
                close(pipefd[1]);
                exit(1);
            }

            /* Añadir terminador '\0' */
            buffer[resultado_read] = '\0';

            /* Comprobar que se leyó lo esperado */
            size_t tamaño_esperado = strlen(mensaje);

            if ((size_t)resultado_read != tamaño_esperado) {

                char resultado[100];

                snprintf(
                    resultado,
                    sizeof(resultado),
                    "Hijo %d: ERROR, esperaba %zu bytes y leyó %zd\n",
                    i,
                    tamaño_esperado,
                    resultado_read
                );

                write(pipefd[1], resultado, strlen(resultado));

                close(fd);
                close(pipefd[1]);
                exit(1);
            }

            /* Paso 4: cada hijo escribe su resultado en el pipe */
            char resultado[100];

            snprintf(
                resultado,
                sizeof(resultado),
                "Hijo %d leyó: %s\n",
                i,
                buffer
            );

            if (write(pipefd[1], resultado, strlen(resultado)) == -1) {
                perror("write pipe");
                close(fd);
                close(pipefd[1]);
                exit(1);
            }

            close(fd);
            close(pipefd[1]);

            exit(0);
        }
    }

    /* El padre no escribe en el pipe */
    close(pipefd[1]);

    /* Paso 5: el padre espera a los hijos */
    for (int i = 0; i < NUM_CHILDREN; i++) {

        int status;

        pid_t pid = wait(&status);

        if (pid == -1) {
            perror("wait");
            close(pipefd[0]);
            return 1;
        }

        if (WIFEXITED(status)) {
            printf(
                "Padre: hijo con PID %d terminó con código %d\n",
                pid,
                WEXITSTATUS(status)
            );
        } else {
            printf(
                "Padre: hijo con PID %d terminó de forma anormal\n",
                pid
            );
        }
    }

    /* Paso 6: el padre lee los resultados del pipe */
    printf("\n--- Resultados de los hijos ---\n");

    char buffer_padre[256];
    ssize_t bytes_leidos;

    while ((bytes_leidos = read(
                pipefd[0],
                buffer_padre,
                sizeof(buffer_padre) - 1)) > 0) {

        buffer_padre[bytes_leidos] = '\0';

        printf("%s", buffer_padre);
    }

    if (bytes_leidos == -1) {
        perror("read pipe");
        close(pipefd[0]);
        return 1;
    }

    close(pipefd[0]);

    /* Paso 7: resumen final */
    printf("\n--- Resumen final ---\n");
    printf("Se crearon %d hijos.\n", NUM_CHILDREN);
    printf("Todos los hijos terminaron.\n");
    printf("El padre recibió los resultados mediante el pipe.\n");

    return 0;
}
