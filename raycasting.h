#ifndef RAYCASTING_H
#define RAYCASTING_H

typedef struct
{
    double posX, posY; // position
    double dirX, dirY; // direction vector
    double planeX, planeY; // camera plane
} player_t;

int drawFrame(player_t player);

#endif // RAYCASTING_H