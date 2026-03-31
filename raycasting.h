#ifndef RAYCASTING_H
#define RAYCASTING_H

#include <stdbool.h>
#include "fx8.h"
#include "assets.h"

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
int MoveCamera(player_t *player, buttons_t buttons);

#endif // RAYCASTING_H