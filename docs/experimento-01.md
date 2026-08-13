# Experimento 01: carga y prueba básica del character device

## Estado: EXITOSO

## Comprobaciones realizadas

1. Módulo compilado como `mi_char_device.ko`
2. Módulo cargado con `insmod mi_char_device.ko`
3. Dispositivo creado en `/dev/mi_char_device`
4. Prueba de escritura: `echo -n "hola driver" > /dev/mi_char_device`
5. Prueba de lectura: `cat /dev/mi_char_device` devolvió `hola driver`
6. Módulo descargado con `rmmod mi_char_device`
7. Dispositivo eliminado correctamente

## Observaciones

- El nombre original `kernel` colisionaba con el núcleo.
- Se renombró a `mi_char_device`.
- El ciclo completo de carga/prueba/descarga funciona.

## Fecha

2026-08-13
