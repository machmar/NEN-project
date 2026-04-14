/*
Copyright (c) 2004-2021, Lode Vandevenne

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "raycasting.h"
#include "dogm128_fast.h"

/* flat map access: 2D array is [width][height], so stride = height */
#define MAP_AT(map, x, y)  ((map)->data[(uint16_t)(x) * (map)->height + (uint8_t)(y)])
#define MAP_IN_BOUNDS(map, x, y) ((x) >= 0 && (uint8_t)(x) < (map)->width && (y) >= 0 && (uint8_t)(y) < (map)->height)

// Screen and map dimensions:
#define screenWidth 48
#define screenHeight 64

// Parameters for scaling and moving the sprites
#define SPRITE_W_SCALE 1  // Scale factor for sprite width (1 = no scaling)
#define SPRITE_H_SCALE 1  // Scale factor for sprite height (1 = no scaling)
#define vMove 0.0 // Move sprite up and down

/* precomputed cameraX = 256 - (x*512/48) for x=0..47, negated to fix screen mirror */
static const fx_t cameraX_lut[48] = {
    256, 246, 235, 224, 214, 203, 192, 182, 171, 160, 150, 139,
    128, 118, 107, 96, 86, 75, 64, 54, 43, 32, 22, 11,
    0, -10, -21, -32, -42, -53, -64, -74, -85, -96, -106, -117,
    -128, -138, -149, -160, -170, -181, -192, -202, -213, -224, -234, -245
};

void inline Line(uint8_t location, uint8_t start, uint8_t length, uint8_t type);

int RenderFrame(const player_t *player, map_t *map) {
    int x;

    for (x = 0; x < screenWidth; x++) {
        fx_t cameraX = cameraX_lut[x];

        /* ray direction */
        fx_t rayDirX = fx_add(player->dirX, fx_mul(player->planeX, cameraX));
        fx_t rayDirY = fx_add(player->dirY, fx_mul(player->planeY, cameraX));

        /* map cell */
        int mapX = FX_TO_INT(player->posX);
        int mapY = FX_TO_INT(player->posY);

        /* delta distances */
        fx_t deltaDistX;
        fx_t deltaDistY;
        fx_t sideDistX;
        fx_t sideDistY;
        fx_t perpWallDist;
        fx_t deltaDistYTransparent;
        fx_t deltaDistXTransparen;
        fx_t sideDistXTransparent;
        fx_t sideDistYTransparent;
        fx_t perpWallDistTransparent;

        int stepX;
        int stepY;
        bool hit = 0;
        bool side = 0;
        bool sideTransparent = 0;

        deltaDistX = fx_inv_clamped(rayDirX);
        deltaDistY = fx_inv_clamped(rayDirY);

        /* initial step and sidedist */
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = fx_mul(fx_sub(player->posX, FX_FROM_INT(mapX)), deltaDistX);
        } else {
            stepX = 1;
            sideDistX = fx_mul(fx_sub(FX_FROM_INT(mapX + 1), player->posX), deltaDistX);
        }

        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = fx_mul(fx_sub(player->posY, FX_FROM_INT(mapY)), deltaDistY);
        } else {
            stepY = 1;
            sideDistY = fx_mul(fx_sub(FX_FROM_INT(mapY + 1), player->posY), deltaDistY);
        }

        /* DDA */
        uint8_t tileType = 0;
        uint8_t tileTypeTransparent = 0;
        for (uint8_t tmp_dist = 0; !hit && tmp_dist < 64; tmp_dist++) {
            if (sideDistX < sideDistY) {
                sideDistX = fx_add(sideDistX, deltaDistX);
                mapX += stepX;
                side = 0;
            } else {
                sideDistY = fx_add(sideDistY, deltaDistY);
                mapY += stepY;
                side = 1;
            }

            if (!MAP_IN_BOUNDS(map, mapX, mapY)) {
                hit = 1;
                break;
            }
            tileType = MAP_AT(map, mapX, mapY);
            if (tileType > 0 && tileType <= 0x0F) // hard coded the wall type range, don't care, cry
                hit = 1;
            else if (tileType >= 0x10 && tileType <= 0x1F && tileTypeTransparent == 0) { // transparent wall
                tileTypeTransparent = tileType;
                sideDistXTransparent = sideDistX;
                sideDistYTransparent = sideDistY;
                deltaDistXTransparen = deltaDistX;
                deltaDistYTransparent = deltaDistY;
                sideTransparent = side;
            }
        }

        /* perpendicular wall distance */
        if (side == 0)
            perpWallDist = fx_sub(sideDistX, deltaDistX);
        else
            perpWallDist = fx_sub(sideDistY, deltaDistY);

        if (perpWallDist < FX_RAW(1))
            perpWallDist = FX_RAW(1);

        if (tileTypeTransparent) {
            if (sideTransparent == 0)
                perpWallDistTransparent = fx_sub(sideDistXTransparent, deltaDistXTransparen);
            else
                perpWallDistTransparent = fx_sub(sideDistYTransparent, deltaDistYTransparent);

            if (perpWallDistTransparent < FX_RAW(1))
                perpWallDistTransparent = FX_RAW(1);
        }

        /* lineHeight = screenHeight / perpWallDist */
        {
            if (tileTypeTransparent) {
                int lineHeight = ((int32_t) 16384) / perpWallDist;
                int drawStart = (-lineHeight >> 1) + 32;

                if (drawStart < 0) drawStart = 0;
                if (lineHeight > screenHeight) lineHeight = screenHeight;
                Line(x, drawStart, lineHeight, tileType);

                lineHeight = ((int32_t) 16384) / perpWallDistTransparent;
                drawStart = (-lineHeight >> 1) + 32;

                if (drawStart < 0) drawStart = 0;
                if (lineHeight > screenHeight) lineHeight = screenHeight;
                Line(x, drawStart, lineHeight, tileTypeTransparent);
            } else {
                int lineHeight = ((int32_t) 16384) / perpWallDist;
                int drawStart = (-lineHeight >> 1) + 32;

                if (drawStart < 0) drawStart = 0;
                if (lineHeight > screenHeight) lineHeight = screenHeight;
                
                Line(x, drawStart, lineHeight, tileType);
            }

        }
        //draw the pixels of the stripe as a vertical line
        //player.zBuffer[x] = perpWallDist; //store distance in ZBuffer for sprite casting
    }

    return 0;
}

void inline Line(uint8_t location, uint8_t start, uint8_t length, uint8_t type) {
    switch (type) {
        case 0x02: // strange floating wall
        case 0x11:
        {
            uint8_t offset = length / 5;
            start += offset / 2;
            length -= offset;
        }
            break;
        case 0x03: // shorter wall
        case 0x12:
        {
            uint8_t offset = length / 5;
            start += offset;
            length -= offset;
        }
            break;
        case 0x04: // a bit lifted wall
        case 0x13:
        {
            uint8_t offset = length / 5;
            length -= offset;
        }
            break;
        case 0x05: // small window
        case 0x14:
        {
            uint8_t offset = length / 2;
            start += offset;
            length -= offset;
        }
            break;
        case 0x06: // door
        case 0x10:
        case 0x15:
        {
            uint8_t offset = length / 3 + length / 2;
            length -= offset;
        }
            break;
        case 0x07: // big window
        case 0x16:
        {
            uint8_t offset = length / 3 + length / 2;
            start += offset;
            length -= offset;
        }
            break;
        default:
            break;
    }

    dogm128_vlineBLACK2px(location * 2, start, length);
}

_Bool inline TileWalkable(uint8_t type) {
    if (type == 0x00 ||
        type == 0x10 ||
        type > 0x1F)
        return 1;
    return 0;
}

int MoveCamera(player_t *player, map_t *map, buttons_t buttons) {
    //move forward if no wall in front of you
    fx_t moveSpeed = FX_HALF; //the constant value is in squares/second
    fx_t rotSpeed = 0x0008; //the constant value is in radians/second (0.1PI per frame)
    uint8_t tile = 0; // the tile that's being walked into
    if (buttons.front) {
        tile = MAP_AT(map, FX_I(fx_add(player->posX, fx_mul(player->dirX, moveSpeed))), FX_I(player->posY));
        if (TileWalkable(tile))
            player->posX = fx_add(player->posX, fx_mul(player->dirX, moveSpeed));

        tile = MAP_AT(map, FX_I(player->posX), FX_I(fx_add(player->posY, fx_mul(player->dirY, moveSpeed))));
        if (TileWalkable(tile))
            player->posY = fx_add(player->posY, fx_mul(player->dirY, moveSpeed));
    }
    //move backwards if no wall behind you
    if (buttons.back) {
        tile = MAP_AT(map, FX_I(fx_sub(player->posX, fx_mul(player->dirX, moveSpeed))), FX_I(player->posY));
        if (TileWalkable(tile))
            player->posX = fx_sub(player->posX, fx_mul(player->dirX, moveSpeed));

        tile = MAP_AT(map, FX_I(player->posX), FX_I(fx_sub(player->posY, fx_mul(player->dirY, moveSpeed))));
        if (TileWalkable(tile))
            player->posY = fx_sub(player->posY, fx_mul(player->dirY, moveSpeed));
    }
    //rotate to the right
    if (buttons.right)
        player->angle = fx_add(player->angle, rotSpeed);
    //rotate to the left
    if (buttons.left)
        player->angle = fx_sub(player->angle, rotSpeed);

    //reconstruct dir and plane from angle (eliminates fixed-point drift)
    if (buttons.right || buttons.left) {
        player->dirX = fx_cos(player->angle);
        player->dirY = fx_sin(player->angle);
        player->planeX = fx_mul(player->dirY, (fx_t) 0x00a9);
        player->planeY = fx_neg(fx_mul(player->dirX, (fx_t) 0x00a9));
    }

    if (buttons.use) {
        player->posX = FX(map->DefaultSpwanPoint[0]);
        player->posY = FX(map->DefaultSpwanPoint[1]);
    }
}

void DrawEntities(player_t *player, entity_t* entities, int amount, uint8_t *display_buffer) {
    // Calculate distance from player to each entity
    /*for(int i = 0; i < amount; i++)
    {
      entities[i].distance = ((player->posX - entities[i].posX) * (player->posX - entities[i].posX) + (player->posY - entities[i].posY) * (player->posY - entities[i].posY));
    }

    // Sort entities by distance from player (farthest to nearest) for rendering order
    // Using bubble sort
    for (int i = 0; i < amount - 1; i++)
    {
        for (int j = 0; j < amount - i - 1; j++)
        {
            if (entities[j].distance < entities[j + 1].distance)
            {
                // Swap
                entity_t temp = entities[j];
                entities[j] = entities[j + 1];
                entities[j + 1] = temp;
            }
        }
    }
  
    // Render entities
    for(int i = 0; i < amount; i++)
    {
      //translate sprite position to relative to camera
      float spriteX = entities[i].posX - player->posX;
      float spriteY = entities[i].posY - player->posY;
      //transform sprite with the inverse camera matrix
      // [ planeX   dirX ] -1                                       [ dirY      -dirX ]
      // [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
      // [ planeY   dirY ]                                          [ -planeY  planeX ]

      float invDet = 1.0 / (player->planeX * player->dirY - player->dirX * player->planeY); //required for correct matrix multiplication

      float transformX = invDet * (player->dirY * spriteX - player->dirX * spriteY);
      float transformY = invDet * (-player->planeY * spriteX + player->planeX * spriteY); //this is actually the depth inside the screen, that what Z is in 3D, the distance of sprite to player, matching sqrt(spriteDistance[i])

      int spriteScreenX = (int)((screenWidth / 2) * (1 + transformX / transformY));
      int vMoveScreen = (int)(vMove / transformY);

      //calculate height of the sprite on screen
      int spriteHeight = abs_float((int)(screenHeight / (transformY))) / SPRITE_H_SCALE; //using "transformY" instead of the real distance prevents fisheye
      //calculate lowest and highest pixel to fill in current stripe
      int drawStartY = -spriteHeight / 2 + screenHeight / 2 + vMoveScreen;
      if(drawStartY < 0) drawStartY = 0;
      int drawEndY = spriteHeight / 2 + screenHeight / 2 + vMoveScreen;
      if(drawEndY >= screenHeight) drawEndY = screenHeight - 1;

      //calculate width of the sprite
      int spriteWidth = abs_float((int)(screenHeight / (transformY))) / SPRITE_W_SCALE; // same as height of sprite, given that it's square
      int drawStartX = -spriteWidth / 2 + spriteScreenX;
      if(drawStartX < 0) drawStartX = 0;
      int drawEndX = spriteWidth / 2 + spriteScreenX;
      if(drawEndX >= screenWidth) drawEndX = screenWidth - 1;

      //loop through every vertical stripe of the sprite on screen
      for(int stripe = drawStartX; stripe < drawEndX; stripe++)
      {
        int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * SPRITE_WIDTH / spriteWidth) / 256;
        //the conditions in the if are:
        //1) it's in front of camera plane so you don't see things behind you
        //2) ZBuffer, with perpendicular distance
        if(transformY > 0 && transformY < player->zBuffer[stripe])
        {
          int pageStart = drawStartY >> 3;
          int pageEnd   = drawEndY   >> 3;

          for(int page = pageStart; page <= pageEnd; page++)
          {
            uint8_t mask = 0;
            for(int bit = 0; bit < 8; bit++)
            {
              int y = (page << 3) + bit;
              if(y >= drawStartY && y < drawEndY)
              {
                int d = (y - vMoveScreen) * 256 - screenHeight * 128 + spriteHeight * 128;
                int texY = ((d * SPRITE_HEIGHT) / spriteHeight) / 256;
                if(texY >= 0 && texY < SPRITE_HEIGHT && entities[i].sprite[texY][texX])
                  mask |= (1 << bit);
              }
            }
            if(mask) display_buffer[page * 128 + stripe] |= mask;
          }
        }
      }
    }*/
}
