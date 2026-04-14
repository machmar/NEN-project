#ifndef RAYCASTING_H
#define RAYCASTING_H

#include <stdbool.h>
#include "fx8.h"
#include "assets.h"

#define MAX_ENTITIES 10

typedef struct __attribute__((packed)) {
    bool front, back, use, left, right;
}
buttons_t;

int MoveCamera(player_t *player, map_t *map, buttons_t buttons);
int RenderFrame(player_t *player, map_t *map, line_t *buffer);
void DrawEntities(player_t *player, entity_t* entities,  int amount, uint8_t *display_buffer);
void EnemyAi(player_t *player, entity_t* entities, int amount, map_t *map);

#endif // RAYCASTING_H