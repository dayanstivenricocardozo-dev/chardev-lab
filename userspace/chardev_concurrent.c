#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define DEVICE_PATH "/dev/mi_char_device"
#define NUM_CHILDREN 2
#define ITERATIONS 100

int main(void)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            return 1;
        }

        if (pid == 0) {
            /* Hijo: solo escribe en el pipe */
            close(pipefd[0]);

            char mensaje[16];
            if (i == 0) {
                strcpy(mensaje, "hola");
            } else {
                strcpy(mensaje, "adios");
            }

            int fd = open(DEVICE_PATH, O_RDWR);
            if (fd == -1) {
                perror("open");
                close(pipefd[1]);
                exit(1);
            }

            int lecturas_propias = 0;
            int lecturas_ajenas = 0;

            for (int iter = 0; iter < ITERATIONS; iter++) {
                /* Escribir */
                ssize_t w = write(fd, mensaje, strlen(mensaje));
                if (w == -1) {
                    perror("write");
                    close(fd);
                    close(pipefd[1]);
                    exit(1);
                }

                /* Leer */
                char buffer[16];
                ssize_t r = read(fd, buffer, sizeof(buffer) - 1);
                if (r == -1) {
                    perror("read");
                    close(fd);
                    close(pipefd[1]);
                    exit(1);
                }

                buffer[r] = '\0';

                /* Comparar */
                if (strcmp(buffer, mensaje) == 0) {
                    lecturas_propias++;
                } else {
                    lecturas_ajenas++;
                }

                /* Resetear offset para la siguiente iteración */
                lseek(fd, 0, SEEK_SET);
            }

            close(fd);

            char resultado[128];
            snprintf(
                resultado,
                sizeof(resultado),
                "Hijo %d: propias=%d ajenas=%d\n",
                i,
                lecturas_propias,
                lecturas_ajenas
            );

            write(pipefd[1], resultado, strlen(resultado));
            close(pipefd[1]);
            exit(0);
        }
    }

    /* Padre: no escribe en el pipe */
    close(pipefd[1]);

    /* Esperar hijos */
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int status;
        pid_t pid = wait(&status);
        if (pid == -1) {
            perror("wait");
            close(pipefd[0]);
            return 1;
        }
    }

    /* Leer resultados del pipe */
    printf("--- Resultados ---\n");
    char buf[256];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    close(pipefd[0]);

    printf("--- Fin ---\n");

    return 0;
}