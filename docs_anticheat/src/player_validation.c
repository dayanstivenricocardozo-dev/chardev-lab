#include <stdio.h>

struct Player {
    char id[10];
    int health;
    int damage;
    int level;
    int positionx;
    int positiony;
};

#define MAX_HEALTH_DELTA 100

int health_change_is_suspicious(struct Player *player, int new_health) {
    if (new_health - player->health > MAX_HEALTH_DELTA) {
        printf("Suspicious health change detected for player %s: %d -> %d\n", player->id, player->health, new_health);
        return 1;
    }
    return 0;
}

int player_distance_squared(struct Player *player1, struct Player *player2) {

    int distancia = (player1->positionx - player2->positionx) * (player1->positionx - player2->positionx) +
                    (player1->positiony - player2->positiony) * (player1->positiony - player2->positiony);
    return distancia; // Example distance
}

int main(){
    struct Player player1 = {"Player1", 100, 10, 5, 0, 0};
    struct Player player2 = {"Player2", 100, 10, 5, 10, 10};

    printf("Estado player 1: %s, Health: %d, Damage: %d, Level: %d, Position: (%d, %d)\n", player1.id, player1.health, player1.damage, player1.level, player1.positionx, player1.positiony);

    int new_health = 10000000; // Example of a suspicious health change

    if (health_change_is_suspicious(&player1, new_health)) {
        printf("suspicious\n");
    }

    else {
        printf("OK\n");
    }

    int distancia = player_distance_squared(&player1, &player2);
    printf("Distance between players: %d\n", distancia);



}


