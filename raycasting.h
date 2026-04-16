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

int RenderFrame(const player_t *player, const map_t *map);

#define SPRITE_WIDTH 5
#define SPRITE_HEIGHT 10

typedef struct {
    float posX, posY; // position
    float distance; // distance from the player (for sorting)
    uint8_t health;
    uint8_t(*sprite)[SPRITE_WIDTH];
} entity_t;

int const static sprite[SPRITE_HEIGHT][SPRITE_WIDTH] = {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0},
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0},
    {0, 1, 1, 1, 0},
    {1, 1, 0, 1, 1},
    {1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1},
};

int MoveCamera(player_t *player, const map_t *map, buttons_t buttons, const dialogue_t **pDialogue);
void DrawEntities(player_t *player, entity_t* entities, int amount, uint8_t *display_buffer);

#endif // RAYCASTING_H