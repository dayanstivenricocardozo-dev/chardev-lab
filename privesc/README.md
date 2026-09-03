# Privilege Escalation Lab

Dos enfoques de escalada de privilegios local usando módulos del kernel.

## funptr/ - Function Pointer Hijack (FUNCIONA)

Explota una vulnerabilidad de buffer overflow en el driver para
sobreescribir un puntero de función y redirigir la ejecución a
`root_shell()`, que ejecuta `prepare_creds()` + `commit_creds()`.

**Flujo del exploit:**
1. Lee `/dev/privesc_lab` para obtener las direcciones de `handler` y `root_shell` (fuga de información).
2. Escribe más de 64 bytes en el buffer, sobreescribiendo el puntero `handler` con la dirección de `root_shell`.
3. Llama al ioctl `PRIVESC_CALL`, que ejecuta `state.handler()` → ahora apunta a `root_shell()`.
4. `root_shell()` ejecuta `prepare_creds()` + `commit_creds()` → UID 0.

**Nota:** Puede fallar en kernels con LSMs agresivos (AppArmor, IMA, IPE) que bloquean `prepare_creds()`.

## direct/ - Escalada directa (NO FUNCIONA en este sistema)

Intenta llamar `prepare_creds()` + `commit_creds()` directamente
desde el handler del ioctl, sin vulnerability de por medio.

**Por qué no funciona:** Los LSMs activos en este sistema
(AppArmor, Tomoyo, IMA, IPE) bloquean la asignación de nuevas
credenciales incluso desde un módulo del kernel cargado como root.

## Requisitos

- Secure Boot desactivado en BIOS
- `linux-headers-$(uname -r)` instalado
- Permisos de root para `insmod`

## Uso

```bash
cd funptr/   # o cd direct/
make
sudo insmod privesc_module.ko
sudo chmod 666 /dev/privesc_lab
./exploit


### 8. Compila y prueba (opcional, si quieres verificar)

```bash
cd funptr
make
sudo insmod privesc_module.ko
sudo chmod 666 /dev/privesc_lab
./exploit
