#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

struct modulo {
    char *nombre;
    int visible_proc;
};

struct modulo *modulos = NULL;
size_t cantidad = 0;
size_t capacidad = 0;

/*
 * Busca un nombre dentro del arreglo de módulos.
 */
int buscar_modulo(const char *nombre)
{
    for (size_t i = 0; i < cantidad; i++) {
        if (strcmp(modulos[i].nombre, nombre) == 0) {
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    FILE *archivo;
    DIR *directorio;
    struct dirent *entrada;
    struct modulo *temp;
    char linea[256];
    char ruta_refcnt[512];

    /*
     * Abrir /proc/modules
     */
    archivo = fopen("/proc/modules", "r");
    if (archivo == NULL) {
        perror("fopen /proc/modules");
        return 1;
    }

    capacidad = 1;

    temp = realloc(modulos, capacidad * sizeof(struct modulo));
    if (temp == NULL) {
        perror("realloc");
        fclose(archivo);
        free(modulos);
        return 1;
    }

    modulos = temp;

    /*
     * Leer todos los módulos de /proc/modules
     */
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        char nombre[64];

        if (sscanf(linea, "%63s", nombre) == 1) {

            /*
             * Si necesitamos más espacio, duplicamos la capacidad.
             */
            if (cantidad == capacidad) {
                capacidad *= 2;

                temp = realloc(
                    modulos,
                    capacidad * sizeof(struct modulo)
                );

                if (temp == NULL) {
                    perror("realloc");

                    for (size_t i = 0; i < cantidad; i++) {
                        free(modulos[i].nombre);
                    }

                    free(modulos);
                    fclose(archivo);
                    return 1;
                }

                modulos = temp;
            }

            /*
             * Guardar una copia del nombre.
             */
            modulos[cantidad].nombre = strdup(nombre);

            if (modulos[cantidad].nombre == NULL) {
                perror("strdup");

                for (size_t i = 0; i < cantidad; i++) {
                    free(modulos[i].nombre);
                }

                free(modulos);
                fclose(archivo);
                return 1;
            }

            modulos[cantidad].visible_proc = 1;
            cantidad++;
        }
    }

    fclose(archivo);

    /*
     * Abrir /sys/module
     */
    directorio = opendir("/sys/module");
    if (directorio == NULL) {
        perror("opendir /sys/module");

        for (size_t i = 0; i < cantidad; i++) {
            free(modulos[i].nombre);
        }

        free(modulos);
        return 1;
    }

    /*
     * Recorrer las entradas de /sys/module.
     */
    while ((entrada = readdir(directorio)) != NULL) {

        /*
         * Ignorar "." y "..".
         */
        if (strcmp(entrada->d_name, ".") == 0 ||
            strcmp(entrada->d_name, "..") == 0) {
            continue;
        }

        /*
         * Construir:
         *
         * /sys/module/NOMBRE/refcnt
         */
        snprintf(
            ruta_refcnt,
            sizeof(ruta_refcnt),
            "/sys/module/%s/refcnt",
            entrada->d_name
        );

        /*
         * Comprobar si existe refcnt.
         */
        if (access(ruta_refcnt, F_OK) == 0) {

            /*
             * Comprobar si también aparece en /proc/modules.
             */
            if (!buscar_modulo(entrada->d_name)) {
                fprintf(
                    stderr,
                    "ALERTA: módulo %s está en /sys/module "
                    "pero no aparece en /proc/modules\n",
                    entrada->d_name
                );
            }
        }
    }

    closedir(directorio);

    /*
     * Liberar toda la memoria.
     */
    for (size_t i = 0; i < cantidad; i++) {
        free(modulos[i].nombre);
    }

    free(modulos);

    return 0;
}
