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

// Screen and map dimensions:
#define screenWidth 48
#define screenHeight 64

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