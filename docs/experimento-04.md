# Experimento 04: ioctl con comandos personalizados

## Estado: EXITOSO

## Comandos implementados

1. MI_IOC_GET_SIZE: devuelve el tamaño actual del buffer privado.
2. MI_IOC_CLEAR: borra el buffer privado.

## Cambios realizados

1. Se creó el header compartido `kernel/mi_char_device_ioctl.h`.
2. Se definió MI_IOC_MAGIC 'm' para identificar el driver.
3. Se implementó la función `mi_ioctl()` en el driver.
4. Se registró `.unlocked_ioctl = mi_ioctl` en file_operations.
5. Se creó la herramienta userspace `chardev_ioctl_test.c`.

## Resultados


## Conexión con seguridad

ioctl es una superficie de ataque común en drivers reales.
Este experimento demuestra la forma correcta de implementarlo:
- Validación del comando con switch/default.
- Uso de copy_to_user para devolver datos.
- Protección con mutex.
- Retorno de -ENOTTY para comandos desconocidos.

## Conexión con DevOps

Herramientas como nvidia-smi, docker, ip y muchas otras
usan ioctl por debajo para comunicarse con drivers.
