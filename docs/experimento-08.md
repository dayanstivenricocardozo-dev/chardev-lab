# Experimento 08: detector de módulos ocultos

## Estado: EXITOSO

## Objetivo

Construir una herramienta userspace que detecte módulos del kernel
que están presentes en /sys/module pero no en /proc/modules.

## Técnica

Comparar dos fuentes de información del kernel:
- /proc/modules: lista de módulos visibles
- /sys/module: lista de módulos conocidos por el sistema de dispositivos

Si un módulo tiene refcnt en /sys/module pero no aparece en /proc/modules,
es sospechoso de estar oculto.

## Resultados

Sin módulo oculto: sin alertas
Con hide_module oculto: ALERTA detectada correctamente
Con hide_module visible: sin alertas

## Conexión con seguridad

Este es un principio fundamental de detección de rootkits:
comparar múltiples fuentes de verdad del sistema.
