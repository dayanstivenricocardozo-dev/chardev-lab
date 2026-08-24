# Experimento 10: escalada de privilegios local

## Estado: EXITOSO

## Objetivo

Demostrar una escalada de privilegios completa en el kernel de Linux.
Un usuario normal (UID 1000) se convierte en root (UID 0) mediante
la explotación de un buffer overflow que secuestra un puntero de función.

## Cadena del exploit

1. Módulo vulnerable con buffer de 64 bytes y puntero de función adyacente.
2. Fuga de información: read() filtra direcciones de handler y root_shell.
3. Attacker userspace parsea direcciones y construye payload binario.
4. Payload de 72 bytes: 64 de relleno + 8 de dirección de root_shell.
5. Ioctl PRIVESC_CALL ejecuta el puntero sobrescrito.
6. root_shell() usa prepare_creds() para copiar credenciales actuales.
7. Modifica UID, GID y capacidades a valores de root.
8. commit_creds() aplica las nuevas credenciales al proceso.

## Resultados


## Técnica de cambio de credenciales

Se usó prepare_creds() + modificación manual + commit_creds().
Esto evita depender de prepare_kernel_cred(NULL), que puede fallar
en kernels modernos con configuraciones de seguridad.

## Conexión con exploits reales

- CVEs de escalada de privilegios en Linux usan exactamente esta técnica.
- Dirty Pipe (CVE-2022-0847), Dirty COW (CVE-2016-5195) y otros
  explotan vulnerabilidades del kernel para obtener root.
- Las protecciones modernas (KASLR, SMEP, SMAP, CFI) dificultan
  pero no eliminan estos ataques.
- La defensa incluye: parches actualizados, lockdown, SELinux/AppArmor,
  y auditoría de módulos del kernel.
