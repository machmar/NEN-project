#ifndef RAYCASTING_H
#define RAYCASTING_H

#include <stdbool.h>
#include "fx8.h"
#include "assets.h"

#define MAX_ENTITIES 10

typedef struct
{
    fx_t posX, posY; // position
    fx_t dirX, dirY; // direction vector
    fx_t planeX, planeY; // camera plane
} player_t;

typedef struct __attribute__((packed))
{
    bool front, back, use, left, right;
} buttons_t;

int extern renderFrame(player_t player, line_t *buffer);
void DrawBuffer(line_t *buffer);
typedef struct
{
    float posX, posY; // position
    float distance; // distance from the player (for sorting)
    uint8_t health;
    uint8_t (*sprite)[SPRITE_WIDTH];
}entity_t;

#define SPRITE_WIDTH 5
#define SPRITE_HEIGHT 10
int const sprite[SPRITE_HEIGHT][SPRITE_WIDTH] = {
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

int MoveCamera(player_t *player, buttons_t buttons);
void DrawEntities(player_t *player, entity_t* entities,  int amount, uint8_t *display_buffer);

#endif // RAYCASTING_H