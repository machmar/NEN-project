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
#define screenWidth 128
#define screenHeight 64
#define mapWidth 24
#define mapHeight 24

int static const worldMap[mapWidth][mapHeight]=
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

const float sin_lut[64] = {
 0.0000000000, 0.0980171403, 0.1950903220, 0.2902846773,
 0.3826834324, 0.4713967368, 0.5555702330, 0.6343932842,
 0.7071067812, 0.7730104534, 0.8314696123, 0.8819212643,
 0.9238795325, 0.9569403357, 0.9807852804, 0.9951847267,
 1.0000000000, 0.9951847267, 0.9807852804, 0.9569403357,
 0.9238795325, 0.8819212643, 0.8314696123, 0.7730104534,
 0.7071067812, 0.6343932842, 0.5555702330, 0.4713967368,
 0.3826834324, 0.2902846773, 0.1950903220, 0.0980171403,
 0.0000000000,-0.0980171403,-0.1950903220,-0.2902846773,
-0.3826834324,-0.4713967368,-0.5555702330,-0.6343932842,
-0.7071067812,-0.7730104534,-0.8314696123,-0.8819212643,
-0.9238795325,-0.9569403357,-0.9807852804,-0.9951847267,
-1.0000000000,-0.9951847267,-0.9807852804,-0.9569403357,
-0.9238795325,-0.8819212643,-0.8314696123,-0.7730104534,
-0.7071067812,-0.6343932842,-0.5555702330,-0.4713967368,
-0.3826834324,-0.2902846773,-0.1950903220,-0.0980171403 };

float abs_float(float x) {return x < 0.0f ? -x : x;}

int DrawFrame(player_t player)
{
    for(int x = 0; x < screenWidth; x++)
    {
      //calculate ray position and direction
      float cameraX = 2 * x / (float)screenWidth - 1; //x-coordinate in camera space
      float rayDirX = player.dirX + player.planeX * cameraX;
      float rayDirY = player.dirY + player.planeY * cameraX;
      //which box of the map we're in
      int mapX = (int)player.posX;
      int mapY = (int)player.posY;

      //length of ray from current position to next x or y-side
      float sideDistX;
      float sideDistY;

      //length of ray from one x or y-side to next x or y-side
      //these are derived as:
      //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
      //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
      //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
      //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
      //unlike (dirX, dirY) is not 1, however this does not matter, only the
      //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
      //stepping further below works. So the values can be computed as below.
      // Division through zero is prevented, even though technically that's not
      // needed in C++ with IEEE 754 floating point values.
      float deltaDistX = (rayDirX == 0) ? 1e30 : abs_float(1 / rayDirX);
      float deltaDistY = (rayDirY == 0) ? 1e30 : abs_float(1 / rayDirY);

      float perpWallDist;

      //what direction to step in x or y-direction (either +1 or -1)
      int stepX;
      int stepY;

      int hit = 0; //was there a wall hit?
      int side; //was a NS or a EW wall hit?
      //calculate step and initial sideDist
      if(rayDirX < 0)
      {
        stepX = -1;
        sideDistX = (player.posX - mapX) * deltaDistX;
      }
      else
      {
        stepX = 1;
        sideDistX = (mapX + 1.0 - player.posX) * deltaDistX;
      }
      if(rayDirY < 0)
      {
        stepY = -1;
        sideDistY = (player.posY - mapY) * deltaDistY;
      }
      else
      {
        stepY = 1;
        sideDistY = (mapY + 1.0 - player.posY) * deltaDistY;
      }
      //perform DDA
      while(hit == 0)
      {
        //jump to next map square, either in x-direction, or in y-direction
        if(sideDistX < sideDistY)
        {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        }
        else
        {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        //Check if ray has hit a wall
        if(worldMap[mapX][mapY] > 0) hit = 1;
      }
      //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
      //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
      //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
      //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
      //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
      //steps, but we subtract deltaDist once because one step more into the wall was taken above.
      if(side == 0) perpWallDist = (sideDistX - deltaDistX);
      else          perpWallDist = (sideDistY - deltaDistY);

      //Calculate height of line to draw on screen
      int lineHeight = (int)(screenHeight / perpWallDist);

      //calculate lowest and highest pixel to fill in current stripe
      int drawStart = -lineHeight / 2 + screenHeight / 2;
      if(drawStart < 0) drawStart = 0;

      //choose wall color
      dogm128_color_t color;
      switch(worldMap[mapX][mapY])
      {
        case 1:  color = DISP_COL_LIGHT_GREY;    break; //red
        case 2:  color = DISP_COL_DARK_GREY;  break; //green
        case 3:  color = DISP_COL_GREY;   break; //blue
        case 4:  color = DISP_COL_WHITE;  break; //white
        default: color = DISP_COL_BLACK; break; //yellow
      }

      //draw the pixels of the stripe as a vertical line
      dogm128_vline(x, drawStart, lineHeight, color);
    }
    return 0;
}

int MoveCamera(player_t *player, buttons_t buttons)
{
    //move forward if no wall in front of you
    fx_t moveSpeed = FX_ONE; //the constant value is in squares/second
    fx_t rotSpeed = 0x0003; //the constant value is in radians/second (0.01PI per frame)
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