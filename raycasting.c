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
#include "fx8.h"

// Screen and map dimensions:
#define screenWidth 48
#define screenHeight 64

// Parameters for scaling and moving the sprites
#define SPRITE_W_SCALE 1  // Scale factor for sprite width (1 = no scaling)
#define SPRITE_H_SCALE 1  // Scale factor for sprite height (1 = no scaling)
#define vMove FX_ZERO // Move sprite up and down

int RenderFrame(player_t player, line_t *buffer)
{
    int x;

    for (x = 0; x < screenWidth; x++)
    {
        /* cameraX = 2*x/screenWidth - 1, in 8.8 fixed */
        fx_t cameraX = (fx_t)((((int32_t)x << 9) / screenWidth) - FX_ONE);

        /* ray direction */
        fx_t rayDirX = fx_add(player.dirX, fx_mul(player.planeX, cameraX));
        fx_t rayDirY = fx_add(player.dirY, fx_mul(player.planeY, cameraX));

        /* map cell */
        int mapX = FX_TO_INT(player.posX);
        int mapY = FX_TO_INT(player.posY);

        /* delta distances */
        fx_t deltaDistX;
        fx_t deltaDistY;
        fx_t sideDistX;
        fx_t sideDistY;
        fx_t perpWallDist;

        int stepX;
        int stepY;
        int hit = 0;
        int side = 0;

        deltaDistX = fx_inv_clamped(rayDirX);
        deltaDistY = fx_inv_clamped(rayDirY);

        /* initial step and sidedist */
        if (rayDirX < 0)
        {
            stepX = -1;
            sideDistX = fx_mul(fx_sub(player.posX, FX_FROM_INT(mapX)), deltaDistX);
        }
        else
        {
            stepX = 1;
            sideDistX = fx_mul(fx_sub(FX_FROM_INT(mapX + 1), player.posX), deltaDistX);
        }

        if (rayDirY < 0)
        {
            stepY = -1;
            sideDistY = fx_mul(fx_sub(player.posY, FX_FROM_INT(mapY)), deltaDistY);
        }
        else
        {
            stepY = 1;
            sideDistY = fx_mul(fx_sub(FX_FROM_INT(mapY + 1), player.posY), deltaDistY);
        }

        /* DDA */
        while (!hit)
        {
            if (sideDistX < sideDistY)
            {
                sideDistX = fx_add(sideDistX, deltaDistX);
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY = fx_add(sideDistY, deltaDistY);
                mapY += stepY;
                side = 1;
            }

            if (worldMap[mapX][mapY] > 0)
                hit = 1;
        }

        /* perpendicular wall distance */
        if (side == 0)
            perpWallDist = fx_sub(sideDistX, deltaDistX);
        else
            perpWallDist = fx_sub(sideDistY, deltaDistY);

        if (perpWallDist < FX_RAW(1))
            perpWallDist = FX_RAW(1);

        /* lineHeight = screenHeight / perpWallDist */
        {
            int lineHeight = ((int32_t)screenHeight << 8) / perpWallDist;
            int drawStart = (-lineHeight >> 1) + (screenHeight >> 1);
            dogm128_color_t color;

            if (drawStart < 0) drawStart = 0;
            if (lineHeight > screenHeight) lineHeight = screenHeight;

            switch (worldMap[mapX][mapY])
            {
                case 1:  color = DISP_COL_LIGHT_GREY; break;
                case 2:  color = DISP_COL_DARK_GREY;  break;
                case 3:  color = DISP_COL_GREY;       break;
                case 4:  color = DISP_COL_WHITE;      break;
                default: color = DISP_COL_BLACK;      break;
            }

            buffer[x].start = drawStart;
            buffer[x].length = lineHeight;
        }
      //draw the pixels of the stripe as a vertical line
      player.zBuffer[x] = perpWallDist; //store distance in ZBuffer for sprite casting
    }

    return 0;
}

void DrawBuffer(line_t *buffer)
{
    for (uint8_t i = 0; i < 48; i++)
    {
        dogm128_vlineBLACK2px(i * 2, buffer[i].start, buffer[i].length);
    }
}

int MoveCamera(player_t *player, buttons_t buttons)
{
    //move forward if no wall in front of you
    fx_t moveSpeed = FX_HALF; //the constant value is in squares/second
    fx_t rotSpeed = 0x0008; //the constant value is in radians/second (0.1PI per frame)
    if(buttons.front)
    {
      if(worldMap[FX_I(fx_add(player->posX, fx_mul(player->dirX, moveSpeed)))][FX_I(player->posY)] == false)
        player->posX = fx_add(player->posX, fx_mul(player->dirX, moveSpeed));
      
      if(worldMap[FX_I(player->posX)][FX_I(fx_add(player->posY, fx_mul(player->dirY, moveSpeed)))] == false)
        player->posY = fx_add(player->posY, fx_mul(player->dirY, moveSpeed));
    }
    //move backwards if no wall behind you
    if(buttons.back)
    {
      if(worldMap[FX_I(fx_sub(player->posX, fx_mul(player->dirX, moveSpeed)))][FX_I(player->posY)] == false)
        player->posX = fx_sub(player->posX, fx_mul(player->dirX, moveSpeed));
      
      if(worldMap[FX_I(player->posX)][FX_I(fx_sub(player->posY, fx_mul(player->dirY, moveSpeed)))] == false)
        player->posY = fx_sub(player->posY, fx_mul(player->dirY, moveSpeed));
    }
    //rotate to the right
    if(buttons.right)
    {
      //both camera direction and camera plane must be rotated
      fx_t oldDirX = player->dirX;
      player->dirX = fx_sub(fx_mul(player->dirX, fx_cos(-rotSpeed)), 
                            fx_mul(player->dirY, fx_sin(-rotSpeed)));
      player->dirY = fx_add(fx_mul(oldDirX,      fx_sin(-rotSpeed)),
                            fx_mul(player->dirY, fx_cos(-rotSpeed)));
      
      fx_t oldPlaneX = player->planeX;
      player->planeX = fx_sub(fx_mul(player->planeX, fx_cos(-rotSpeed)), 
                              fx_mul(player->planeY, fx_sin(-rotSpeed)));
      player->planeY = fx_add(fx_mul(oldPlaneX,      fx_sin(-rotSpeed)),
                              fx_mul(player->planeY, fx_cos(-rotSpeed)));
    }
    //rotate to the left
    if(buttons.left)
    {
      //both camera direction and camera plane must be rotated
      fx_t oldDirX = player->dirX;
      player->dirX = fx_sub(fx_mul(player->dirX, fx_cos(rotSpeed)), 
                            fx_mul(player->dirY, fx_sin(rotSpeed)));
      player->dirY = fx_add(fx_mul(oldDirX,      fx_sin(rotSpeed)),
                            fx_mul(player->dirY, fx_cos(rotSpeed)));
      
      fx_t oldPlaneX = player->planeX;
      player->planeX = fx_sub(fx_mul(player->planeX, fx_cos(rotSpeed)), 
                              fx_mul(player->planeY, fx_sin(rotSpeed)));
      player->planeY = fx_add(fx_mul(oldPlaneX,      fx_sin(rotSpeed)),
                              fx_mul(player->planeY, fx_cos(rotSpeed)));
    }
    
    if (buttons.use)
    {
        player->posX = FX(11);
        player->posY = FX(11);
    }
}

void DrawEntities(player_t *player, entity_t* entities,  int amount, uint8_t *display_buffer)
{
  if (amount <= 0)
      return;

  // Precompute squared distance once, then insertion-sort (fewer moves than bubble sort for small arrays).
  for (int i = 0; i < amount; i++)
  {
      fx_t dx = fx_sub(player->posX, entities[i].posX);
      fx_t dy = fx_sub(player->posY, entities[i].posY);
      entities[i].distance = fx_add(fx_mul(dx, dx), fx_mul(dy, dy));
  }

  for (int i = 1; i < amount; i++)
  {
      entity_t key = entities[i];
      int j = i - 1;
      while (j >= 0 && entities[j].distance < key.distance)
      {
          entities[j + 1] = entities[j];
          j--;
      }
      entities[j + 1] = key;
  }

  fx_t *zBuffer = player->zBuffer;
  int halfW = screenWidth >> 1;
  int halfH = screenHeight >> 1;
  fx_t invDet = fx_inv_clamped(fx_sub(fx_mul(player->planeX, player->dirY), fx_mul(player->dirX, player->planeY)));

  // Render entities.
  for (int i = 0; i < amount; i++)
  {
    uint8_t (*spriteTex)[SPRITE_WIDTH] = entities[i].sprite;
    if (spriteTex == 0 || entities[i].health == 0)
      continue;

    // Translate sprite position relative to camera.
    fx_t spriteX = fx_sub(entities[i].posX, player->posX);
    fx_t spriteY = fx_sub(entities[i].posY, player->posY);

    // Transform sprite with inverse camera matrix.
    fx_t transformX = fx_mul(invDet, fx_sub(fx_mul(player->dirY, spriteX), fx_mul(player->dirX, spriteY)));
    fx_t transformY = fx_mul(invDet, fx_add(fx_mul(fx_neg(player->planeY), spriteX), fx_mul(player->planeX, spriteY)));
    if (transformY <= FX_ZERO)
      continue;

    int spriteScreenX = halfW + FX_I(fx_mul(FX(halfW), fx_div(transformX, transformY)));
    int vMoveScreen = FX_I(fx_div(vMove, transformY));

    // Calculate projected sprite size.
    int spriteHeight = FX_I(fx_abs_fast(fx_div(fx_div(FX(screenHeight), transformY), FX(SPRITE_H_SCALE))));
    if (spriteHeight <= 0)
      continue;

    int drawStartY = -(spriteHeight >> 1) + halfH + vMoveScreen;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = (spriteHeight >> 1) + halfH + vMoveScreen;
    if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;
    if (drawEndY <= drawStartY)
      continue;

    int spriteWidth = FX_I(fx_abs_fast(fx_div(fx_div(FX(screenHeight), transformY), FX(SPRITE_W_SCALE))));
    if (spriteWidth <= 0)
      continue;

    int spriteLeft = -(spriteWidth >> 1) + spriteScreenX;
    int drawStartX = spriteLeft;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = (spriteWidth >> 1) + spriteScreenX;
    if (drawEndX >= screenWidth) drawEndX = screenWidth - 1;
    if (drawEndX <= drawStartX)
      continue;

    // Incremental texture mapping removes divisions from inner loops.
    int texXStep = (SPRITE_WIDTH << 8) / spriteWidth;
    int texXPos = (drawStartX - spriteLeft) * texXStep;
    int texYBase = ((drawStartY - vMoveScreen) << 8) - (screenHeight << 7) + (spriteHeight << 7);
    int texYStep = (SPRITE_HEIGHT << 8) / spriteHeight;
    int texYStart = (texYBase * SPRITE_HEIGHT) / spriteHeight;

    for (int stripe = drawStartX; stripe < drawEndX; stripe++)
    {
      if (transformY >= zBuffer[stripe])
      {
        texXPos += texXStep;
        continue;
      }

      int texX = texXPos >> 8;
      texXPos += texXStep;
      if (texX < 0 || texX >= SPRITE_WIDTH)
        continue;

      int texYPos = texYStart;
      uint8_t page = (uint8_t)(drawStartY >> 3);
      int nextPageY = ((int)page + 1) << 3;
      uint8_t mask = 0;

      for (int y = drawStartY; y < drawEndY; y++)
      {
        if (y == nextPageY)
        {
          if (mask)
            display_buffer[(page * screenWidth) + stripe] |= mask;
          page++;
          nextPageY += 8;
          mask = 0;
        }

        int texY = texYPos >> 8;
        if (texY >= 0 && texY < SPRITE_HEIGHT && spriteTex[texY][texX])
          mask |= (uint8_t)(1u << (y & 7));

        texYPos += texYStep;
      }

      if (mask)
        display_buffer[(page * screenWidth) + stripe] |= mask;
    }
  }
}
