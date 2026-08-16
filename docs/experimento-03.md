# Experimento 03: aislamiento por proceso con private_data

## Estado: EXITOSO

## Problema detectado en Experimento 02

El buffer global compartido permitía que un proceso
leyera los datos escritos por otro proceso.

Esto es una fuga de información.

## Cambios realizados en el driver

1. Se eliminó el buffer global `device_buffer`.
2. Se creó `struct mi_dev_data` con buffer, tamaño y mutex propios.
3. `open()` reserva memoria con `kmalloc()` y la asigna a `file->private_data`.
4. `release()` libera la memoria con `kfree()`.
5. `read()` y `write()` usan los datos privados de cada apertura.
6. Se agregó `.llseek = default_llseek` para permitir reposicionar el offset.
7. Se movió la verificación de offset dentro del mutex para evitar carrera.

## Resultados


## Conclusión

Cada proceso tiene su propio buffer aislado.
No hay fuga de información entre procesos.
El mutex protege las operaciones individuales.
El llseek permite reutilizar el descriptor en múltiples iteraciones.

## Implicación de seguridad

Se eliminó la vulnerabilidad de lectura cruzada entre procesos.
