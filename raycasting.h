#ifndef RAYCASTING_H
#define RAYCASTING_H

#include <stdbool.h>
#include <stdint.h>
#include "fx8.h"
#include "assets.h"

#define MAX_ENTITIES 5

typedef union __attribute__((packed)) {
    struct {
      bool front:1, back:1, use:1, left:1, right:1;  
    };
    uint8_t all;
}
buttons_t;

int MoveCamera(player_t *player, const map_t *map, buttons_t buttons, const dialogue_t **pDialogue, bool dont_scale);
int RenderFrame(player_t *player, const map_t *map);
void DrawEntities(player_t *player, entity_t* entities,  uint8_t amount, uint8_t *display_buffer, buttons_t buttons);
void EnemyAi(player_t *player, entity_t* entities, uint8_t amount, map_t *map, bool dont_scale);
void HitDetection(player_t *player, entity_t *entities);

#endif // RAYCASTING_H