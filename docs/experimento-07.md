# Experimento 07: rootkit educativo - ocultar módulo de lsmod

## Estado: EXITOSO

## Objetivo

Crear un módulo del kernel que pueda ocultarse de lsmod
y volver a mostrarse mediante una orden escrita en /proc/hide_lab.

## Cambios realizados

1. Se creó el módulo hide_module.
2. Se creó el archivo virtual /proc/hide_lab con permisos 0600.
3. Se implementó una función write para recibir comandos.
4. El comando hide elimina el módulo de la lista de módulos cargados.
5. El comando show vuelve a insertar el módulo en la lista.
6. Se agregó remove_proc_entry() en init para limpiar entradas huérfanas.
7. Se agregó remove_proc_entry() en exit para limpieza correcta.

## Resultados


## Observaciones

- lsmod lee la lista de módulos cargados.
- Si el módulo se elimina de esa lista, lsmod no lo ve.
- Esto demuestra cómo un rootkit puede manipular estructuras internas del kernel.
- En este laboratorio no se ocultaron procesos ni archivos.
- El módulo sigue siendo visible en /sys/module mientras está oculto de lsmod.

## Lección de seguridad

La ocultación no siempre significa que el componente desapareció.
Solo desapareció de una vista concreta del sistema.
Por eso las herramientas de detección comparan varias fuentes de información.
