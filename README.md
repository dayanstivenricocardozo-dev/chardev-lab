# chardev-lab

Laboratorio de desarrollo de drivers de caracteres en Linux con enfoque en seguridad ofensiva y defensiva.

Este proyecto documenta la construcción progresiva de un character device driver en el kernel de Linux, junto con herramientas userspace para probarlo, atacarlo y defenderlo.

## Qué se aprendió

- Desarrollo de módulos del kernel de Linux en C
- Character devices: open, read, write, release, ioctl, llseek
- Aislamiento de datos por proceso con `private_data`, `kmalloc` y `kfree`
- Concurrencia con `fork()`, `pipe()` y `wait()`
- Buffer overflow en kernel y parche defensivo
- Secuestro de punteros de función (function pointer hijack)
- Escalada de privilegios local (usuario normal → root)
- Rootkits: ocultar módulos del kernel
- Detección de rootkits: comparar fuentes de información del kernel
- Git para versionado de código de sistemas

## Estructura del repositorio

## Experimentos

| # | Experimento | Estado |
|---|-------------|--------|
| 01 | Carga y prueba básica del módulo | ✅ |
| 02 | Estrés concurrente con buffer compartido | ✅ |
| 03 | Aislamiento por proceso con private_data | ✅ |
| 04 | Ioctl con comandos personalizados | ✅ |
| 05 | Ataque de buffer overflow en write() | ✅ |
| 06 | Exploit de overflow + parche defensivo | ✅ |
| 07 | Rootkit: ocultar módulo de lsmod | ✅ |
| 08 | Detector de módulos ocultos | ✅ |
| 09 | Function pointer hijack | ✅ |
| 10 | Escalada de privilegios local | ✅ |

## Herramientas userspace

| Herramienta | Propósito |
|-------------|-----------|
| chardev_test | Prueba secuencial del driver |
| chardev_concurrent | Prueba concurrente con fork + pipe |
| chardev_ioctl_test | Prueba de comandos ioctl |
| chardev_attacker | Ataque de overflow al driver seguro |
| vuln_attacker | Ataque de overflow al driver vulnerable |
| module_detector | Detector de módulos ocultos |
| funptr_attacker | Exploit de function pointer hijack |
| privesc_attacker | Exploit de escalada de privilegios |

## Cómo compilar

### Drivers del kernel

```bash
cd kernel/
make

cd vuln/
make

gcc -Wall -Wextra -o userspace/nombre userspace/nombre.c

# Cargar módulo
sudo insmod ruta/al/modulo.ko

# Ejecutar herramienta
sudo ./userspace/herramienta

# Ver logs del kernel
dmesg | tail -n 10

# Descargar módulo
sudo rmmod nombre_del_modulo