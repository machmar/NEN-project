#ifndef RAYCASTING_H
#define RAYCASTING_H

#include <stdbool.h>
#include "fx8.h"
#include "assets.h"

#define MAX_ENTITIES 10

typedef struct __attribute__((packed))
{
    bool front, back, use, left, right;
} buttons_t;

int RenderFrame(player_t *player, line_t *buffer);
void DrawBuffer(line_t *buffer);
int MoveCamera(player_t *player, buttons_t buttons);
void DrawEntities(player_t *player, entity_t* entities,  int amount, uint8_t *display_buffer);

#endif // RAYCASTING_H