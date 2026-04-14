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
#include <stdint.h>


static inline uint8_t tex_at(const spriteData_t *sprite, uint8_t frame, uint8_t x, uint8_t y)
{
	uint16_t idx = (uint16_t)y * sprite->width + x;
	uint8_t shift = (uint8_t)(idx & 7);
	//printf("idx: %d\tshift: %d\n", idx, shift);
	const uint8_t *pair = &sprite->data[frame][(idx >> 3) * 2];
	//printf("pair num start: %d\n", (idx >> 3) * 2);
	//printf("%d ", pair[0]);
	//printf("%d", pair[1]);
	if (((pair[0] >> shift) & 1)) return 0;   
	return ((pair[1] >> shift) & 1) ? 1 : 2;   
	return 0;
}
#define TEX_AT(sprite, frame, x, y)  ((sprite)->data[frame][(uint16_t)(y) * (sprite)->width + (uint8_t)(x)])
/* flat map access: 2D array is [width][height], so stride = height */
#define MAP_AT(map, x, y)  ((map)->data[(uint16_t)(x) * (map)->height + (uint8_t)(y)])
#define MAP_IN_BOUNDS(map, x, y) ((x) >= 0 && (uint8_t)(x) < (map)->width && (y) >= 0 && (uint8_t)(y) < (map)->height)

// Screen and map dimensions:
#define screenWidth 48
#define screenHeight 64

/* precomputed cameraX = 256 - (x*512/48) for x=0..47, negated to fix screen mirror */
static const fx_t cameraX_lut[48] = {
    256, 246, 235, 224, 214, 203, 192, 182, 171, 160, 150, 139,
    128, 118, 107, 96, 86, 75, 64, 54, 43, 32, 22, 11,
    0, -10, -21, -32, -42, -53, -64, -74, -85, -96, -106, -117,
    -128, -138, -149, -160, -170, -181, -192, -202, -213, -224, -234, -245
};

int RenderFrame(player_t *player, map_t *map, line_t *buffer)
{
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

        int stepX;
        int stepY;
        int hit = 0;
        int side = 0;

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
            if (tileType > 0 && tileType < 0x10) // hard coded the wall type range, don't care, cry
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
            int lineHeight = ((int32_t) 16384) / perpWallDist;
            int drawStart = (-lineHeight >> 1) + 32;

            if (drawStart < 0) drawStart = 0;
            if (lineHeight > screenHeight) lineHeight = screenHeight;

            buffer[x].start = hit ? drawStart : 0;
            buffer[x].length = hit ? lineHeight : 0;
            buffer[x].type = tileType;
        }
      //draw the pixels of the stripe as a vertical line
      player->zBuffer[x] = perpWallDist; //store distance in ZBuffer for sprite casting
    }

    return 0;
}

void DrawBuffer(line_t *buffer) {
    for (uint8_t i = 0; i < 48; i++) {
        uint8_t start = buffer[i].start;
        uint8_t length = buffer[i].length;

        switch (buffer[i].type) {
            case 2: // strange floating wall
            {
                uint8_t offset = length / 5;
                start += offset / 2;
                length -= offset;
            }
                break;
            case 3: // shorter wall
            {
                uint8_t offset = length / 5;
                start += offset;
                length -= offset;
            }
                break;
            case 4: // a bit lifted wall
            {
                uint8_t offset = length / 5;
                length -= offset;
            }
                break;
            case 5: // small window
            {
                uint8_t offset = length / 2;
                start += offset;
                length -= offset;
            }
                break;
            case 0x0F:
            case 6: // door
            {
                uint8_t offset = length / 3 + length / 2;
                length -= offset;
            }
                break;
            case 7: // big window
            {
                uint8_t offset = length / 3 + length / 2;
                start += offset;
                length -= offset;
            }
                break;
            default:
                break;
        }

        dogm128_vlineBLACK2px(i * 2, start, length);
    }
}

int MoveCamera(player_t *player, map_t *map, buttons_t buttons) {
    //move forward if no wall in front of you
    fx_t moveSpeed = FX_HALF; //the constant value is in squares/second
    fx_t rotSpeed = 0x0008; //the constant value is in radians/second (0.1PI per frame)
    uint8_t tile = 0; // the tile that's being walked into
    if (buttons.front) {
        tile = MAP_AT(map, FX_I(fx_add(player->posX, fx_mul(player->dirX, moveSpeed))), FX_I(player->posY));
        if (tile <= 0x00 || tile >= 0x0f)
            player->posX = fx_add(player->posX, fx_mul(player->dirX, moveSpeed));

        tile = MAP_AT(map, FX_I(player->posX), FX_I(fx_add(player->posY, fx_mul(player->dirY, moveSpeed))));
        if (tile <= 0x00 || tile >= 0x0f)
            player->posY = fx_add(player->posY, fx_mul(player->dirY, moveSpeed));
    }
    //move backwards if no wall behind you
    if (buttons.back) {
        tile = MAP_AT(map, FX_I(fx_sub(player->posX, fx_mul(player->dirX, moveSpeed))), FX_I(player->posY));
        if (tile <= 0x00 || tile >= 0x0f)
            player->posX = fx_sub(player->posX, fx_mul(player->dirX, moveSpeed));

        tile = MAP_AT(map, FX_I(player->posX), FX_I(fx_sub(player->posY, fx_mul(player->dirY, moveSpeed))));
        if (tile <= 0x00 || tile >= 0x0f)
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
  int screenWidthPixels = screenWidth; // Half-resolution: 48 strips covering 96 pixels
  int halfW = screenWidthPixels >> 1;
  int halfH = screenHeight >> 1;
  
  fx_t invDet = fx_inv_clamped(fx_sub(fx_mul(player->planeX, player->dirY), fx_mul(player->dirX, player->planeY)));

  // Render entities.
  for (int i = 0; i < amount; i++)
  {
    if (entities[i].sprite->data[0] == 0)
      continue;

    // Translate sprite position relative to camera.
    fx_t spriteX = fx_sub(entities[i].posX, player->posX);
    fx_t spriteY = fx_sub(entities[i].posY, player->posY);

    // Transform sprite with inverse camera matrix.
    fx_t transformX = fx_mul(invDet, fx_sub(fx_mul(player->dirY, spriteX), fx_mul(player->dirX, spriteY)));
    fx_t transformY = fx_mul(invDet, fx_add(fx_mul(fx_neg(player->planeY), spriteX), fx_mul(player->planeX, spriteY)));
    if (transformY <= 0X0005)
      continue;

    int spriteScreenX = halfW - FX_I(fx_mul(FX(halfW), fx_div(transformX, transformY)));
    int heightOffsetScreen = FX_I(fx_div(entities[i].heightOffset, transformY));

    // Calculate projected sprite size.
    int spriteHeight = FX_I(fx_abs_fast(fx_div(fx_div(FX(screenHeight), transformY), FX(entities[i].heightScale))));
    if (spriteHeight <= 0)
      continue;

    int drawStartY = -(spriteHeight >> 1) + halfH + heightOffsetScreen;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = (spriteHeight >> 1) + halfH + heightOffsetScreen;
    if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;
    if (drawEndY <= drawStartY)
      continue;

    int spriteWidth = FX_I(fx_abs_fast(fx_div(fx_div(FX(screenHeight), transformY), FX(entities[i].widthScale))));
    if (spriteWidth <= 0)
      continue;

    int spriteLeft = -(spriteWidth >> 1) + spriteScreenX;
    int drawStartX = spriteLeft;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = (spriteWidth >> 1) + spriteScreenX;
    if (drawEndX >= screenWidthPixels) drawEndX = screenWidthPixels - 1;
    if (drawEndX <= drawStartX)
      continue;

    // Incremental texture mapping removes divisions from inner loops.
    int texXStep = (entities[i].sprite->width << 8) / spriteWidth;
    int texXPos = (drawStartX - spriteLeft) * texXStep;
    int texYBase = ((drawStartY - heightOffsetScreen) << 8) - (screenHeight << 7) + (spriteHeight << 7);
    int texYStep = (entities[i].sprite->height << 8) / spriteHeight;
    int texYStart = (texYBase * entities[i].sprite->height) / spriteHeight;
    /* If problems with overflow arise, the above can be rewritten to use 32-bit intermediate values:
    long texYBase = ((long)(drawStartY - heightOffsetScreen) << 8) - ((long)screenHeight << 7) + ((long)spriteHeight << 7);
    int texYStep = (int)(((long)entities[i].sprite->height << 8) / spriteHeight);
    int texYStart = (int)((texYBase * entities[i].sprite->height) / spriteHeight);
    */
    static uint8_t prevFrames = 0;
    static uint8_t walkSprite = 0;
    uint8_t usedSprite = 0;
    if(entities[i].health <= 0)
      usedSprite = 2;
    if(entities[i].walking && prevFrames++ > 5){
      walkSprite ^= 1;
      usedSprite = walkSprite;
      prevFrames = 0;
    }

    uint8_t pixelStride = (entities[i].distance < FX(15)) ? ((entities[i].distance < FX(3)) ? 8 : 4) : 2;
    int loopStride = (pixelStride >> 1);

    for (int stripe = drawStartX; stripe < drawEndX; stripe += loopStride)
    {
      if (transformY >= zBuffer[stripe])
      {
        texXPos += texXStep * loopStride;
        continue;
      }

      uint16_t texX = texXPos >> 8;
      texXPos += texXStep * loopStride;
      if (texX >= entities[i].sprite->width)
        continue;

      int texYPos = texYStart;
      uint8_t page = (uint8_t)(drawStartY >> 3);
      int nextPageY = ((int)page + 1) << 3;
      uint8_t mask = 0;
      uint8_t clearMask = 0;

      for (int y = drawStartY; y < drawEndY; y++)
      {
        if (y == nextPageY)
        {
          if (mask || clearMask)
          {
            for (uint8_t p = 0; p < pixelStride; p++)
              display_buffer[(page * 128) + stripe * 2 + p] &= ~clearMask;
            for (uint8_t p = 0; p < pixelStride; p++)
              display_buffer[(page * 128) + stripe * 2 + p] |= mask;
          }
          page++;
          nextPageY += 8;
          mask = 0;
          clearMask = 0;
        }

        uint16_t texY = texYPos >> 8;
        if (texY < entities[i].sprite->height)
        {
          uint8_t texVal = tex_at(entities[i].sprite, usedSprite, texX, texY);
          uint8_t bit = (uint8_t)(1u << (y & 7));
          if (texVal == 1)
            mask |= bit;
          else if (texVal == 2)
            clearMask |= bit;
        }
        texYPos += texYStep;
      }

      if (mask || clearMask) {
        for (uint8_t p = 0; p < pixelStride; p++)
          display_buffer[(page * 128) + stripe * 2 + p] &= clearMask;
        for (uint8_t p = 0; p < pixelStride; p++)
          display_buffer[(page * 128) + stripe * 2 + p] |= mask;
      }
    }
  }
}

void EnemyAi(player_t *player, entity_t *entities, int amount, map_t *map){
  for(int i = 0; i < amount; i++){
    if(entities[i].health <= 0)
      continue;

    // Check line of sight
    fx_t dx = fx_sub(fx_add(player->posX, player->dirX), entities[i].posX);
    fx_t dy = fx_sub(fx_add(player->posY, player->dirY), entities[i].posY);
    if (entities[i].distance < FX(10)) {
      entities[i].lineOfSight = 1;
    }

    // Move towards player if in line of sight
    if (entities[i].lineOfSight) {
      fx_t moveSpeed = FX(1) / 8; //Speed multiplier for enemies
      fx_t dirX = fx_div(dx, entities[i].distance);
      fx_t dirY = fx_div(dy, entities[i].distance);

      // Check for wall collisions before moving
      uint8_t tileX = MAP_AT(map, FX_I(fx_add(entities[i].posX, fx_mul(dirX, moveSpeed))), FX_I(entities[i].posY));
      uint8_t tileY = MAP_AT(map, FX_I(entities[i].posX), FX_I(fx_add(entities[i].posY, fx_mul(dirY, moveSpeed))));

      if (tileX <= 0x00 || tileX >= 0x0f)
        entities[i].posX = fx_add(entities[i].posX, fx_mul(dirX, moveSpeed));

      if (tileY <= 0x00 || tileY >= 0x0f)
        entities[i].posY = fx_add(entities[i].posY, fx_mul(dirY, moveSpeed));
      
      entities[i].walking = 1;
    } else {
      entities[i].walking = 0;
    }
  }
}

