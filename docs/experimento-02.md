# Experimento 02: estrés concurrente con buffer compartido

## Estado: COMPLETADO

## Configuración

- 2 procesos hijos
- 100 iteraciones por hijo
- Buffer compartido en el driver

## Resultados
Hijo 0: propias=1 ajenas=99
Hijo 1: propias=0 ajenas=100
Hijo 0: propias=1 ajenas=99
Hijo 1: propias=0 ajenas=100
exit

## Conclusión

- El mutex protege cada operación individual. No hay corrupción.
- El buffer es compartido. El último escritor gana.
- Las lecturas "ajenas" no son fallo del mutex.
- Son consecuencia del diseño de buffer global.

## Implicación de seguridad

Si dos procesos no confiables usan el mismo driver,
uno puede leer los datos del otro.

Esto es un problema de aislamiento.
