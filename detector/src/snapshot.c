#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>


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

            if (sscanf(linea, "Uid: %d %d", &real, &efectivo) == 2) {
                fclose(archivo);
                return efectivo;
            }

            fclose(archivo);
            return -1;
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

long leer_starttime(int pid) {
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

    buffer[strcspn(buffer, "\n")] = '\0';

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
            if (!isdigit((unsigned char)entrada->d_name[i])) {
                es_pid = 0;
                break;
            }
        }

        if (!es_pid) {
            continue;
        }

        if (contador >= max_procs) {
            break;
        }

        int pid = atoi(entrada->d_name);

        int euid = leer_euid(pid);

        if (euid < 0) {
            continue;
        }

        long starttime = leer_starttime(pid);

        if (starttime < 0) {
            continue;
        }

        char comm[256];

        if (leer_comm(pid, comm, sizeof(comm)) < 0) {
            continue;
        }

        array[contador].pid = pid;
        array[contador].euid = euid;
        array[contador].starttime = starttime;

        strncpy(array[contador].comm,
                comm,
                sizeof(array[contador].comm) - 1);

        array[contador].comm[sizeof(array[contador].comm) - 1] = '\0';

        contador++;
    }

    closedir(dir);

    return contador;
}


// Busca un proceso por PID + starttime.
// Retorna el índice si lo encuentra, -1 si no existe.

int buscar_proceso(struct proceso_info array[], int cantidad,
                   int pid, long starttime) {

    for (int i = 0; i < cantidad; i++) {

        if (array[i].pid == pid &&
            array[i].starttime == starttime) {

            return i;
        }
    }

    return -1;
}


// Compara el snapshot anterior con el actual.
// Detecta procesos que pasaron de EUID != 0 a EUID == 0.

void comparar_snapshots(
    struct proceso_info anterior[], int cant_anterior,
    struct proceso_info actual[], int cant_actual
) {

    printf("DEBUG: Snapshot anterior tiene %d procesos\n", cant_anterior);
    printf("DEBUG: Snapshot actual tiene %d procesos\n", cant_actual);

    for (int i = 0; i < cant_actual; i++) {

        int indice = buscar_proceso(
            anterior,
            cant_anterior,
            actual[i].pid,
            actual[i].starttime
        );

        // AGREGA ESTO TEMPORALMENTE:
        if (indice != -1) {
            printf("DEBUG: Proceso PID %d encontrado en anterior. EUID anterior: %d, EUID actual: %d\n",
                   actual[i].pid, anterior[indice].euid, actual[i].euid);
        }

        if (indice == -1) {
            continue;
        }

        if (anterior[indice].euid != 0 && actual[i].euid == 0) {
            printf("ALERTA: PID %d (%s) escaló de EUID %d a 0\n",
                   actual[i].pid, actual[i].comm, anterior[indice].euid);
        }
    }
}


int main() {

    struct proceso_info snapshot_anterior[4096];
    struct proceso_info snapshot_actual[4096];

    // Primer snapshot.
    int cant_anterior =
        tomar_snapshot(snapshot_anterior, 4096);

    if (cant_anterior < 0) {
        return 1;
    }

    while (1) {

        // Esperamos 2 segundos.
        sleep(2);

        // Nuevo snapshot.
        int cant_actual =
            tomar_snapshot(snapshot_actual, 4096);

        if (cant_actual < 0) {
            continue;
        }

        // Comparamos anterior vs actual.
        comparar_snapshots(
            snapshot_anterior,
            cant_anterior,
            snapshot_actual,
            cant_actual
        );

        // El snapshot actual pasa a ser el anterior.
        memcpy(
            snapshot_anterior,
            snapshot_actual,
            cant_actual * sizeof(struct proceso_info)
        );

        cant_anterior = cant_actual;
    }

    return 0;
}
