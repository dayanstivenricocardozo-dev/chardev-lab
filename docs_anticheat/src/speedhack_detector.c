#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h> // Para sqrt()

struct PlayerState {
    char id[10];
    int x;
    int y;
    int timestamp;
};

#define MAX_SPEED_ALLOWED 5.0f // Ahora es float

bool is_speedhacking(const struct PlayerState *prev_state, const struct PlayerState *curr_state) {
    
    // TODO 1: Calcular distancia REAL (no al cuadrado)
    // Fórmula: sqrt((x2-x1)² + (y2-y1)²)
    int dx = curr_state->x - prev_state->x;
    int dy = curr_state->y - prev_state->y;
    float distance = sqrt(dx * dx + dy * dy); // Usando float para distancia
    
    // TODO 2: Calcular tiempo transcurrido
    int time_delta = curr_state->timestamp - prev_state->timestamp;
    
    // TODO 3: Validar que time_delta sea válido (¡NO dividir por cero!)
    if (time_delta <= 0) {
        printf("Error: time_delta inválido\n");
        return false;
    }
    
    // TODO 4: Calcular velocidad usando float/double
    float speed = distance / (float)time_delta; // Convertir time_delta a float para obtener velocidad en float
    
    // TODO 5: Validar contra MAX_SPEED_ALLOWED
    if (speed > MAX_SPEED_ALLOWED) {
        printf("¡Hacker detectado! Velocidad: %.2f\n", speed);
        return true;
    }
    
    return false;
}

int main() {
    // Caso 1: Movimiento legal
    struct PlayerState p1 = {"Player1", 0, 0, 100};
    struct PlayerState p2 = {"Player1", 3, 4, 102}; // distancia=5, tiempo=2, velocidad=2.5
    
    if (is_speedhacking(&p1, &p2)) {
        printf("Caso 1: BANEADO (ERROR - debería ser legal)\n");
    } else {
        printf("Caso 1: OK (CORRECTO - velocidad 2.5 < 5)\n");
    }
    
    // Caso 2: Teleport hack
    struct PlayerState p3 = {"Player2", 0, 0, 100};
    struct PlayerState p4 = {"Player2", 100, 100, 102}; // distancia=141, tiempo=2, velocidad=70.5
    
    if (is_speedhacking(&p3, &p4)) {
        printf("Caso 2: BANEADO (CORRECTO - velocidad 70.5 > 5)\n");
    } else {
        printf("Caso 2: OK (ERROR - debería ser baneado)\n");
    }
    
    // Caso 3: División por cero
    struct PlayerState p5 = {"Player3", 0, 0, 100};
    struct PlayerState p6 = {"Player3", 10, 10, 100}; // Mismo timestamp
    
    if (is_speedhacking(&p5, &p6)) {
        printf("Caso 3: BANEADO\n");
    } else {
        printf("Caso 3: OK (CORRECTO - manejaste división por cero)\n");
    }
    
    return 0;
}