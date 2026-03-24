#ifndef RAYCASTING_H
#define RAYCASTING_H

#include <stdbool.h>

typedef struct
{
    float posX, posY; // position
    float dirX, dirY; // direction vector
    float planeX, planeY; // camera plane
} player_t;

typedef struct __attribute__((packed))
{
    bool front, back, use, left, right;
} buttons_t;

int extern DrawFrame(player_t player);
int MoveCamera(player_t *player, buttons_t buttons);

#endif // RAYCASTING_H