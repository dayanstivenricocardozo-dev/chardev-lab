#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>


struct proceso_info {
    int pid;
    int euid;
    long starttime;
    char comm[256];

    char cmdline[512];
    int ppid;
    int uid;
};


volatile sig_atomic_t flag = 1;


void handler(int signal) {
    (void)signal;
    flag = 0;
}


const char *whitelist[] = {
    "systemd",
    "sshd",
    "cron",
    "rsyslogd"
};


int en_whitelist(const char *nombre) {

    int cantidad = sizeof(whitelist) / sizeof(whitelist[0]);

    for (int i = 0; i < cantidad; i++) {

        if (strcmp(nombre, whitelist[i]) == 0) {
            return 1;
        }
    }

    return 0;
}


int leer_uids(int pid, int *uid_real, int *euid) {

    char ruta[64];

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", pid);

    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        return -1;
    }

    char linea[256];

    while (fgets(linea, sizeof(linea), archivo) != NULL) {

        if (strncmp(linea, "Uid:", 4) == 0) {

            if (sscanf(linea, "Uid: %d %d",
                       uid_real, euid) == 2) {

                fclose(archivo);
                return 0;
            }

            fclose(archivo);
            return -1;
        }
    }

    fclose(archivo);

    return -1;
}


int leer_cmdline(int pid, char *buffer, int size) {
    char ruta[64];
    snprintf(ruta, sizeof(ruta), "/proc/%d/cmdline", pid);

    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        buffer[0] = '\0';  // cmdline vacío
        return 0;          // no es error, solo no tiene cmdline
    }

    int leidos = fread(buffer, 1, size - 1, archivo);
    fclose(archivo);

    if (leidos <= 0) {
        buffer[0] = '\0';  // cmdline vacío
        return 0;
    }

    buffer[leidos] = '\0';

    for (int i = 0; i < leidos; i++) {
        if (buffer[i] == '\0') {
            buffer[i] = ' ';
        }
    }
    buffer[leidos] = '\0';

    return 0;
}


int leer_ppid(int pid) {

    char ruta[64];

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", pid);

    FILE *archivo = fopen(ruta, "r");

    if (archivo == NULL) {
        return -1;
    }

    char linea[256];

    while (fgets(linea, sizeof(linea), archivo) != NULL) {

        if (strncmp(linea, "PPid:", 5) == 0) {

            int ppid;

            if (sscanf(linea, "PPid: %d", &ppid) == 1) {

                fclose(archivo);
                return ppid;
            }

            fclose(archivo);
            return -1;
        }
    }

    fclose(archivo);

    return -1;
}


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


        /* Leer UID real y EUID */

        int uid;
        int euid;

        if (leer_uids(pid, &uid, &euid) < 0) {
            continue;
        }


        /* Leer PPID */

        int ppid = leer_ppid(pid);

        if (ppid < 0) {
            continue;
        }


        /* Leer CMDLINE */

        char cmdline[512];

        if (leer_cmdline(pid, cmdline, sizeof(cmdline)) < 0) {
            continue;
        }


        /* Leer STARTTIME */

        long starttime = leer_starttime(pid);

        if (starttime < 0) {
            continue;
        }


        /* Leer COMM */

        char comm[256];

        if (leer_comm(pid, comm, sizeof(comm)) < 0) {
            continue;
        }


        /* Guardar información en el snapshot */

        array[contador].pid = pid;

        array[contador].euid = euid;

        array[contador].starttime = starttime;

        array[contador].uid = uid;

        array[contador].ppid = ppid;


        strncpy(array[contador].comm,
                comm,
                sizeof(array[contador].comm) - 1);

        array[contador].comm[
            sizeof(array[contador].comm) - 1
        ] = '\0';


        strncpy(array[contador].cmdline,
                cmdline,
                sizeof(array[contador].cmdline) - 1);

        array[contador].cmdline[
            sizeof(array[contador].cmdline) - 1
        ] = '\0';


        contador++;
    }

    closedir(dir);

    return contador;
}


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


void comparar_snapshots(
    struct proceso_info anterior[], int cant_anterior,
    struct proceso_info actual[], int cant_actual,
    FILE *log
) {

    for (int i = 0; i < cant_actual; i++) {

        int indice = buscar_proceso(
            anterior,
            cant_anterior,
            actual[i].pid,
            actual[i].starttime
        );


        if (indice == -1) {
            continue;
        }


        /* Detectar escalada de privilegios */

        if (anterior[indice].euid != 0 &&
            actual[i].euid == 0) {


            /* Revisar whitelist */

            if (en_whitelist(actual[i].comm)) {

                printf(
                    "[INFO] PID %d (%s) está en whitelist. "
                    "No se alerta.\n",
                    actual[i].pid,
                    actual[i].comm
                );

                continue;
            }


            /* Mostrar alerta en pantalla */

            printf(
                "ALERTA: PID=%d COMM=%s CMDLINE=%s "
                "PPID=%d UID=%d EUID=%d->%d\n",
                actual[i].pid,
                actual[i].comm,
                actual[i].cmdline,
                actual[i].ppid,
                actual[i].uid,
                anterior[indice].euid,
                actual[i].euid
            );


            /* Guardar alerta en archivo */

            fprintf(
                log,
                "ALERTA: PID=%d COMM=%s CMDLINE=%s "
                "PPID=%d UID=%d EUID=%d->%d\n",
                actual[i].pid,
                actual[i].comm,
                actual[i].cmdline,
                actual[i].ppid,
                actual[i].uid,
                anterior[indice].euid,
                actual[i].euid
            );


            fflush(log);
        }
    }
}


int main() {

    signal(SIGINT, handler);
    signal(SIGTERM, handler);


    FILE *log = fopen("alertas.log", "a");

    if (log == NULL) {

        perror("Error al abrir alertas.log");

        return 1;
    }


    struct proceso_info snapshot_anterior[4096];

    struct proceso_info snapshot_actual[4096];


    int cant_anterior =
        tomar_snapshot(snapshot_anterior, 4096);


    if (cant_anterior < 0) {

        fclose(log);

        return 1;
    }


    while (flag) {

        sleep(2);


        int cant_actual =
            tomar_snapshot(snapshot_actual, 4096);


        if (cant_actual < 0) {
            continue;
        }


        comparar_snapshots(
            snapshot_anterior,
            cant_anterior,
            snapshot_actual,
            cant_actual,
            log
        );


        memcpy(
            snapshot_anterior,
            snapshot_actual,
            cant_actual * sizeof(struct proceso_info)
        );


        cant_anterior = cant_actual;
    }


    fclose(log);

    return 0;
}