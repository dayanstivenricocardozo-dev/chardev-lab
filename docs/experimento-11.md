# Experimento 11: Detector local de escalada de privilegios (MVP)

## 1. Objetivo
Detectar cuándo un proceso identificado previamente con UID no privilegiado 
obtiene UID 0 (root), distinguiéndolo de un PID reutilizado mediante el 
uso del PID y el `starttime` del proceso para garantizar que es la misma 
entidad la que ha modificado sus credenciales.

## 2. Alcance del MVP
- Tomar snapshots periódicos leyendo `/proc/[pid]/status` y `/proc/[pid]/stat`.
- Guardar por cada proceso: `pid`, `uid` (real), `euid` (efectivo) y `starttime`.
- Comparar snapshot actual vs anterior.
- Alertar si un proceso con EUID no-root pasa a EUID 0.
- Ignorar procesos nuevos que nacen directamente como root.
- Loguear alertas en `stdout` y en un archivo `alertas.log`.

## 3. No objetivos (por ahora)
- No detecta módulos del kernel.
- No detecta puertos abiertos ni red.
- No previene ataques (no mata procesos).
- No usa eBPF ni auditd todavía.
- No se despliega en cloud.

## 4. Amenaza a detectar
Un atacante local explota una vulnerabilidad en un proceso con permisos 
normales y obtiene una shell o acceso con UID 0. El proceso cambia su 
EUID de un valor distinto de 0 a 0.

## 5. Fuentes de datos
- `/proc/[pid]/status`: de aquí extraigo los UIDs (real, efectivo, etc.).
- `/proc/[pid]/stat`: de aquí extraigo el `starttime` para identificar unívocamente el proceso.
- `/proc/[pid]/cmdline`: la línea de comandos con la que se lanzó el proceso (para contexto en la alerta).

## 6. Reglas de detección
Regla principal: si un proceso estaba en el snapshot anterior con 
EUID != 0, y ahora aparece con EUID == 0, Y su identidad 
(PID + starttime) es la misma, entonces alerta.

Excepción: si el binario está en whitelist (ej: `sudo`, `su`, `systemd`), 
no alertar o alertar con nivel de severidad informativo.

## 7. Casos límite
- PID reuse: dos procesos diferentes con el mismo PID pero distinto 
  starttime → no alertar (se trata como proceso nuevo).
- Proceso muere durante la lectura → el programa lo salta y sigue con el siguiente.
- Proceso nace y muere entre dos snapshots → no podrá detectarlo (limitación aceptada).
- Permisos denegados al leer `/proc/[pid]/...` → se ignora ese proceso en el snapshot actual y se loguea un warning.

## 8. Supuestos
- El kernel no está comprometido al inicio de la ejecución.
- `/proc` refleja información veraz de los procesos.
- El detector corre con permisos suficientes para leer la mayoría de `/proc`.
- Los snapshots cada 1-2 segundos son suficientes para el MVP.

## 9. Limitaciones
- Si el atacante ya comprometió el kernel, `/proc` puede mentir.
- Procesos que escalan y mueren entre dos snapshots son invisibles.
- Falsos positivos con `sudo`, `su`, servicios del sistema si no se 
  ajustan bien las reglas.
- No previene nada, solo alerta.

## 10. Herramientas existentes relacionadas
- auditd: sistema oficial del kernel para auditar syscalls y eventos de seguridad.
- osquery: permite consultar el estado del sistema como si fuera una base de datos SQL.
- Falco: detecta comportamientos anómalos en runtime, muy usado en Kubernetes.
- Wazuh: plataforma completa de HIDS/SIEM con agentes, reglas y dashboard.
- Tracee: herramienta de Aqua Security que usa eBPF para rastrear eventos del sistema.
- Cilium Tetragon: seguridad y observabilidad en runtime basada en eBPF.

## 11. Pruebas educativas locales
Ejecutaré el exploit de `chardev-lab/privesc/` que eleva de UID 1000 a 
UID 0. El detector debería alertar el cambio. Documentaré el resultado 
en este mismo documento.
