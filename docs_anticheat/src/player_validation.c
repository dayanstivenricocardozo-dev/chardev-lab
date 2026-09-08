#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

struct Player {
    char id[10];
    int health;
    int damage;
    int level;
    int positionx;
    int positiony;
};

#define MAX_HEALTH_DELTA 100

bool health_change_is_suspicious(const struct Player *player, int new_health) {
    if (new_health - player->health > MAX_HEALTH_DELTA) {
        printf("Suspicious health change detected for player %s: %d -> %d\n", player->id, player->health, new_health);
        return true;
    }
    return false;
}

void print_all_players(const struct Player *array, int count) {
    for (int i = 0; i < count; i++) {
        printf("Player %d: ID: %s, Health: %d, Damage: %d, Level: %d, Position: (%d, %d)\n",
               i + 1, array[i].id, array[i].health, array[i].damage, array[i].level,
               array[i].positionx, array[i].positiony);
    }
}

int player_distance_squared(const struct Player *player1, const struct Player *player2) {

    int distancia = (player1->positionx - player2->positionx) * (player1->positionx - player2->positionx) +
                    (player1->positiony - player2->positiony) * (player1->positiony - player2->positiony);
    return distancia; // Example distance
}

int main(){
    struct Player player1 = {"Player1", 100, 10, 5, 0, 0};
    struct Player player2 = {"Player2", 100, 10, 5, 10, 10};


    struct Player *players = malloc(5 * sizeof(*players));

    if (players == NULL) {
        printf("Error: no se pudo reservar memoria\n");
        return 1;
    }

    players[0] = player1;
    players[1] = player2;

    int num_players = 2; // Porque solo llenaste la posición 0 y 1
    print_all_players(players, num_players);
    

    printf("Estado player 1: %s, Health: %d, Damage: %d, Level: %d, Position: (%d, %d)\n", player1.id, player1.health, player1.damage, player1.level, player1.positionx, player1.positiony);

    int new_health = 10000000; // Example of a suspicious health change

    if (health_change_is_suspicious(&player1, new_health)) {
        printf("suspicious\n");
    }

    else {
        printf("OK\n");
    }

    int distance = player_distance_squared(&player1, &player2);
    printf("Distance between players: %d\n", distance);

    free(players);


}


