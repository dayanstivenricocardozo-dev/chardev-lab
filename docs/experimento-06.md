# Experimento 06: exploit de overflow en kernel + parche defensivo

## Estado: EXITOSO

## Fase 1: Ataque

- Driver vulnerable con buffer de 64 bytes y variable admin adyacente.
- Attacker escribe 80 bytes de 'A' (0x41).
- Bytes 64-67 sobrescriben admin.
- Resultado: admin=1094795585 (0x41414141).
- write() devuelve 80.

## Fase 2: Parche

- Se agrega validacion: if (count > sizeof(state.buf)) count = sizeof(state.buf);
- Attacker intenta escribir 80 bytes.
- Driver solo acepta 64 bytes.
- write() devuelve 64.
- admin permanece en 0.

## Conclusion

Una sola linea de validacion evita la corrupcion de memoria.
Esto demuestra por que la validacion de limites es critica en drivers del kernel.

## Conexion con seguridad real

- CVEs de escalada de privilegios usan exactamente este patron.
- Un overflow en kernel puede corromper credenciales, punteros de funcion, o flags de seguridad.
- La defensa es siempre validar limites antes de copiar datos del usuario.
