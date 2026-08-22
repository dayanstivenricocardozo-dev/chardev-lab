# Experimento 09: function pointer hijack

## Estado: EXITOSO

## Objetivo

Demostrar que un buffer overflow puede sobrescribir un puntero
de función en el kernel y redirigir el flujo de ejecución.

## Componentes

1. Módulo funptr con buffer vulnerable y puntero de función adyacente.
2. Fuga de información: read() filtra direcciones de handler y win_handler.
3. Attacker userspace que parsea direcciones y construye payload binario.
4. Payload de 72 bytes: 64 de relleno + 8 de dirección.
5. Ioctl FUNPTR_CALL ejecuta el puntero de función.

## Resultados


## Conclusión

El atacante controló qué función ejecutó el kernel.
No inyectó código nuevo. Solo sobrescribió una dirección.
Esto es una primitiva fundamental de explotación de kernel.

## Conexión con exploits reales

- CVEs de kernel usan exactamente esta técnica.
- En exploits reales el objetivo sería cambiar credenciales
  o ejecutar código arbitrario con privilegios de kernel.
- Las protecciones modernas (KASLR, SMEP, SMAP) hacen esto
  más difícil pero no imposible.
