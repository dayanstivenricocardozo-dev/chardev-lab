#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

struct proceso_info {
    int pid;
    int euid;
    long starttime;
    char comm[256];
};

// TODO: leer_euid(pid)
// Abre /proc/[pid]/status.
// Busca la línea "Uid:".
// Extrae el segundo número (effective UID).
// Retorna euid, o -1 si falla.

int leer_euid(int pid) {
    char ruta[64];

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", pid);

    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        return -1;
    }

    char linea[256];

    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        if (strncmp(linea, "Uid:", 4) == 0) {
            int real, efectivo;

            sscanf(linea, "Uid: %d %d", &real, &efectivo);

            fclose(archivo);
            return efectivo;
        }
    }

    fclose(archivo);
    return -1;
}


// TODO: leer_starttime(pid)
// Abre /proc/[pid]/stat.
// Busca el último ')'.
// Obtiene el campo starttime.
// Retorna starttime, o -1 si falla.

int leer_starttime(int pid) {
    char ruta[64];

    snprintf(ruta, sizeof(ruta), "/proc/%d/stat", pid);

    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        return -1;
    }

    char linea[4096];

    if (fgets(linea, sizeof(linea), archivo) == NULL) {
        fclose(archivo);
        return -1;
    }

    char *fin_comm = strrchr(linea, ')');

    if (fin_comm == NULL) {
        fclose(archivo);
        return -1;
    }

    char *datos = fin_comm + 2;
    char *token = strtok(datos, " ");

    if (token == NULL) {
        fclose(archivo);
        return -1;
    }

    for (int i = 3; i < 22; i++) {
        token = strtok(NULL, " ");

        if (token == NULL) {
            fclose(archivo);
            return -1;
        }
    }

    long starttime = atol(token);

    fclose(archivo);
    return starttime;
}


// TODO: leer_comm(pid, buffer, size)
// Abre /proc/[pid]/comm.
// Copia el nombre del proceso al buffer.
// Retorna 0 en éxito, -1 en error.

int leer_comm(int pid, char *buffer, int size) {
    char ruta[64];

    snprintf(ruta, sizeof(ruta), "/proc/%d/comm", pid);

    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        return -1;
    }

    if (fgets(buffer, size, archivo) == NULL) {
        fclose(archivo);
        return -1;
    }

    fclose(archivo);
    return 0;
}


// TODO: tomar_snapshot(array, max_procs)
// Abre /proc.
// Recorre las entradas.
// Filtra solo los nombres numéricos.
// Para cada PID obtiene euid, starttime y comm.
// Retorna la cantidad de procesos.

int tomar_snapshot(struct proceso_info array[], int max_procs) {
    DIR *dir = opendir("/proc");

    if (dir == NULL) {
        return -1;
    }

    struct dirent *entrada;
    int contador = 0;

    while ((entrada = readdir(dir)) != NULL) {

        int es_pid = 1;

        for (int i = 0; entrada->d_name[i] != '\0'; i++) {
            if (!isdigit(entrada->d_name[i])) {
                es_pid = 0;
                break;
            }
        }

        if (!es_pid)
            continue;

        if (contador >= max_procs)
            break;

        int pid = atoi(entrada->d_name);

        array[contador].pid = pid;
        array[contador].euid = leer_euid(pid);
        array[contador].starttime = leer_starttime(pid);

        leer_comm(pid,
                  array[contador].comm,
                  sizeof(array[contador].comm));

        contador++;
    }

    closedir(dir);

    return contador;
}


// TODO: main()
// Declara el array de procesos.
// Llama a tomar_snapshot.
// Imprime los datos de cada proceso.
// Retorna 0.

int main() {
    struct proceso_info procesos[4096];

    int cantidad = tomar_snapshot(procesos, 4096);

    if (cantidad < 0) {
        return 1;
    }

    for (int i = 0; i < cantidad; i++) {
        printf("PID: %d EUID: %d STARTTIME: %ld COMM: %s",
               procesos[i].pid,
               procesos[i].euid,
               procesos[i].starttime,
               procesos[i].comm);
    }

    return 0;
}