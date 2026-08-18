# Experimento 05: ataque de buffer overflow en write()

## Estado: COMPLETADO

## Objetivo

Intentar escribir 2000 bytes en un buffer de 1024 bytes
para verificar si el driver protege contra buffer overflow.

## Hipótesis

Se esperaba que el driver truncara la escritura a 1023 bytes.

## Resultado

El programa attacker escribió 2000 bytes.
El driver devolvió 1023 bytes escritos.
dmesg confirmó: write() <- 1023 bytes.

## Conclusión

El driver está protegido contra buffer overflow en write().
La línea clave es:

    if (count > BUFFER_SIZE - 1)
        bytes_to_write = BUFFER_SIZE - 1;

No hay overflow.
