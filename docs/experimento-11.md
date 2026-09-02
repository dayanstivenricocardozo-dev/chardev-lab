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


## 12. Prueba del detector versión 1
Ejecuté el snapshot del hito uno para ver que era lo que devolvía, pero antes lo compilé, no dio nada de errores ni de warnings, al ejecutarlo, saierón los procesos con su PID y starttime correctamente, también mostró  corre	ctamente su EUID.
Algunos procesos: 
PID: 5643 EUID: 1000 STARTTIME: 966359 COMM: xfce4-terminal
PID: 5652 EUID: 1000 STARTTIME: 966368 COMM: bash
PID: 5661 EUID: 1000 STARTTIME: 967859 COMM: snapshot
Hito 1 terminado correctamente.

## 13. Implementación Hito 2: Comparación y detección

## 13. Implementación Hito 2: Comparación y detección

### Cómo funciona la comparación
El detector toma un snapshot inicial y luego entra en un bucle cada 2 segundos:
1. Toma un nuevo snapshot de todos los procesos en `/proc`.
2. Para cada proceso del snapshot actual, busca si existe en el snapshot anterior usando la identidad compuesta `PID + starttime`.
3. Si el proceso existe en ambos snapshots y su EUID cambió de distinto de 0 a 0, genera una alerta.
4. Si el proceso no existe en el snapshot anterior, se considera proceso nuevo y no se alerta aunque sea root.
5. Después de comparar, el snapshot actual pasa a ser el anterior para la siguiente iteración.

### Pruebas realizadas
- Se compiló el detector con `gcc -Wall -Wextra -pedantic` sin errores ni warnings.
- Se ejecutó el detector y se verificó que captura correctamente los procesos del sistema.
- Se intentó probar con el módulo `privesc.ko` del laboratorio chardev-lab, pero el módulo no implementa correctamente la escalada de privilegios real (el ioctl retorna éxito pero no modifica las credenciales del proceso desde userspace).
- Se verificó que la lógica de comparación funciona correctamente observando los mensajes DEBUG con procesos con EUID 0 (root) y EUID 1000 (usuario normal) en los snapshots sucesivos.

### Resultados observados
El detector muestra mensajes DEBUG indicando qué procesos fueron encontrados en el snapshot anterior y sus EUIDs. La lógica de detección está implementada correctamente:
- Procesos con EUID 0 que ya eran root en el snapshot anterior: no generan alerta (correcto).
- Procesos con EUID 1000 que siguen siendo 1000: no generan alerta (correcto).
- Procesos nuevos que aparecen con EUID 0: no generan alerta (correcto, son procesos nuevos).

### Limitaciones encontradas durante la prueba
- El módulo `privesc.ko` no realiza la escalada de privilegios real en este entorno, por lo que no se pudo generar una alerta de escalada real en esta prueba.
- Para una prueba completa se necesitaría un exploit que realmente cambie el EUID de un proceso existente de 1000 a 0.

### Conclusión del Hito 2
La lógica de comparación de snapshots y detección de cambios de EUID está implementada y funcionando correctamente. El detector identifica procesos por `PID + starttime` para evitar falsos positivos por PID reuse, y solo alerta cuando un proceso existente cambia de EUID no-root a EUID root.

## 14. Implementación Hito 3: Whitelist, logging y señales

### Whitelist
Se agregaron estos procesos "systemd",
"sshd"
"cron",
"rsyslogd"

porque no queremos recibir falsos positivos, son procesos que si se inician en root pero que no son hechos por atacantes para escalar privilegios, sino que son del sistema

### Logging a archivo
[Cómo funciona y ejemplo de alertas.log]
Cuando pasa algo en el sistema crea archivos log y se guardan en el para saber que es lo que ha estado ocurriendo

### Manejo de señales
Nuestro sistema se detiene con ctrl+c gracias a que pusimos las flags, para que cuando tenga que detenerlo lo haga sin problemas, entonces el programa es como
flag→ 0,1→ detener

### Respuesta a la pregunta de entrevista
[Por qué volatile sig_atomic_t]
volatile Porque le indica al compilador que el valor puede cambiar de forma repentina.
sig_atomic_t Garantiza que la lectura/escritura de la variable es atómica, es decir, no puede ser interrumpida a mitad de operación


### Pruebas realizadas
Se intentó obtener privilegios con su a ver si salian alertas, tambien con sudo, pero no se detectó nada porque al comparar las cadenas se da cuenta de que no es un problema

## 15. Implementación Hito 4: Contexto enriquecido en alertas

### Qué se agregó
- **CMDLINE completo**: línea de comandos completa del proceso (ej: `/bin/bash -i`)
- **PPID**: PID del proceso padre, útil para rastrear quién lanzó el proceso sospechoso
- **UID real**: además del EUID (efectivo), ahora también se muestra el UID real del usuario

### Formato de alerta actualizado

Este formato muestra:
- PID del proceso que escaló privilegios
- Nombre del comando (COMM)
- Línea de comandos completa (CMDLINE)
- PID del padre (PPID)
- UID real del usuario
- Cambio de EUID: valor anterior -> valor actual

### Limitaciones
- Los procesos del kernel (como kworker, migration, etc.) no tienen cmdline porque no fueron lanzados por un usuario. Aparecen con cmdline vacío.
- Estos procesos siempre tienen EUID 0 desde el inicio, por lo que no generan alertas de escalada de privilegios.
- El detector no puede distinguir entre un proceso legítimo que necesita root y un exploit malicioso. Solo detecta el cambio de privilegios.

### Pruebas realizadas
1. Se compiló el detector con `gcc -Wall -Wextra -pedantic` sin warnings.
2. Se ejecutó el detector durante varios minutos observando procesos del sistema.
3. Se verificó que los procesos del kernel aparecen con cmdline vacío pero no generan falsas alertas.
4. Se probó detener el detector con Ctrl+C y se verificó que cierra limpiamente.
5. Se verificó que el archivo `alertas.log` se crea correctamente y puede ser leído después.

### Mejora futura
Para una versión más completa, se podría:
- Agregar detección de cambios de UID/GID adicionales (no solo EUID)
- Monitorear cambios en capabilities del proceso
- Integrar con auditd o eBPF para detección en tiempo real sin polling
##16. Agregar detector de modulos.

Ahora nuesto snapshot puede detectar modulos gracias a comparar con /proc/
y /sys/ en busca de modulos, que quizás no aparecen en el sistema o estan escondidos.


