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
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>

#define FRAMES_PER_WALK 4
#define SCREEN_WIDTH 128
#define VIEWPORT_WIDTH_PIXELS (screenWidth << 1)
#define VIEWPORT_HALF_W (VIEWPORT_WIDTH_PIXELS >> 1)
#define VIEWPORT_HALF_H (screenHeight >> 1)

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


void inline Line(uint8_t location, uint8_t start, uint8_t length, uint8_t type);

/* Keep these scratch buffers static to reduce DrawEntities auto-stack pressure. */
static uint8_t g_draw_leftVisibleRows[8];
static uint8_t g_draw_currentVisibleRows[8];
static const fx_t ENEMY_MOVE_SPEED = FX(1) / 8;

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
      player->zBuffer[x] = perpWallDist; //store distance in ZBuffer for sprite casting
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

void DrawEntities(player_t *player, entity_t* entities,  int amount, uint8_t *display_buffer)
{
  static int prevFrames = 0;
  prevFrames++;
  int screenXStart = 0;
  int screenYStart = 0;
  uint8_t rendered = 0;
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
    if (transformY <= FX_RAW(32))
      continue;

    // Skip sprite if it's clearly outside horizontal FOV — prevents int16 overflow in fx_div
    if ((int32_t)fx_abs_fast(transformX) > (int32_t)transformY * 2)
      continue;

    int spriteScreenX = VIEWPORT_HALF_W - FX_I(fx_mul(FX(VIEWPORT_HALF_W), fx_div(transformX, transformY)));
    int heightOffsetScreen = FX_I(fx_div(entities[i].heightOffset, transformY));

    // Calculate projected sprite size.
    int spriteHeight = FX_I(fx_abs_fast(fx_div(FX(screenHeight), transformY)));
    if (spriteHeight <= 0)
      continue;

    int drawStartY = -(spriteHeight >> 1) + VIEWPORT_HALF_H + heightOffsetScreen;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = (spriteHeight >> 1) + VIEWPORT_HALF_H + heightOffsetScreen;
    if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;
    if (drawEndY <= drawStartY)
      continue;

    int spriteWidth = FX_I(fx_abs_fast(fx_div(fx_div(FX(screenHeight), transformY), entities[i].ratio)));
    if (spriteWidth <= 0)
      continue;

    int spriteLeft = -(spriteWidth >> 1) + spriteScreenX;
    int drawStartX = spriteLeft;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = (spriteWidth >> 1) + spriteScreenX;
    if (drawEndX >= VIEWPORT_WIDTH_PIXELS) drawEndX = VIEWPORT_WIDTH_PIXELS - 1;
    if (drawEndX <= drawStartX)
      continue;

    // Skipping rendering if previous sprite is within +-2 pixels of current sprite
    if (rendered &&
        (drawStartX - screenXStart) >= -4 && (drawStartX - screenXStart) <= 4 &&
        (drawStartY - screenYStart) >= -4 && (drawStartY - screenYStart) <= 4){
          continue;
        }
    rendered = 1;
    screenXStart = drawStartX;
    screenYStart = drawStartY;

    // Incremental texture mapping removes divisions from inner loops.
    uint16_t texXStep = (uint16_t)(entities[i].sprite->width << 8) / (uint16_t)spriteWidth;
    uint16_t texXPos = (uint16_t)(drawStartX - spriteLeft) * texXStep;
    uint16_t texYBase = (uint16_t)((drawStartY - heightOffsetScreen) << 8) - (screenHeight << 7) + ((uint16_t)spriteHeight << 7);
    uint16_t texYStep = (uint16_t)(entities[i].sprite->height << 8) / (uint16_t)spriteHeight;
    uint16_t texYStart = (uint16_t)(texYBase * entities[i].sprite->height) / (uint16_t)spriteHeight;

    static uint8_t walkSprite = 0;
    if(entities[i].walking && prevFrames > FRAMES_PER_WALK * amount){
      walkSprite ^= 1;
      prevFrames = 0;
    }
    uint8_t usedSprite = walkSprite;
    if(entities[i].health <= 0)
      usedSprite = 2;

    uint8_t pixelStride = (entities[i].distance < FX(6)) ? ((entities[i].distance < FX(3)) ? 4 : 2) : 1;
    for (uint8_t r = 0; r < 8; r++)
      g_draw_leftVisibleRows[r] = 0;

    for (uint8_t stripe = (uint8_t)drawStartX; stripe < drawEndX; stripe += pixelStride)
    {
      if (transformY >= player->zBuffer[stripe >> 1])
      {
        for (uint8_t r = 0; r < 8; r++)
          g_draw_leftVisibleRows[r] = 0;
        texXPos += texXStep * pixelStride;
        continue;
      }

      uint16_t texX = texXPos >> 8;
      texXPos += texXStep * pixelStride;
      if (texX >= entities[i].sprite->width)
      {
        for (uint8_t r = 0; r < 8; r++)
          g_draw_leftVisibleRows[r] = 0;
        continue;
      }

      uint16_t texYPos = texYStart;
      uint8_t page = (uint8_t)(drawStartY >> 3);
      uint8_t nextPageY = (page + 1) << 3;
      uint8_t fillMask = 0;
      uint8_t fillClearMask = 0;
      uint8_t edgeHClearMask = 0;
      uint8_t edgeVClearMask = 0;
      uint16_t bufferIndex = (page * SCREEN_WIDTH) + stripe;
      uint8_t prevVisibleInStripe = 0;
      for (uint8_t r = 0; r < 8; r++)
        g_draw_currentVisibleRows[r] = 0;

      for (uint8_t y = (uint8_t)drawStartY; y < drawEndY; y++)
      {
        if (y == nextPageY)
        {
          if (fillMask || fillClearMask || edgeHClearMask || edgeVClearMask)
          {
            uint8_t mergedClear = fillClearMask | edgeHClearMask;
            for (uint8_t p = 0; p < pixelStride; p++)
              display_buffer[bufferIndex + p] &= ~mergedClear;
            for (uint8_t p = 0; p < pixelStride; p++)
              display_buffer[bufferIndex + p] |= fillMask;

            // Keep vertical outline 1px wide even when coarse x stride is enabled.
            if (edgeVClearMask)
              display_buffer[bufferIndex] &= ~edgeVClearMask;
          }
          page++;
          nextPageY += 8;
          bufferIndex = (page * SCREEN_WIDTH) + stripe;
          fillMask = 0;
          fillClearMask = 0;
          edgeHClearMask = 0;
          edgeVClearMask = 0;
        }

        uint16_t texY = texYPos >> 8;
        uint8_t screenBit = (uint8_t)(1U << (y & 7));
        uint8_t rowIndex = (uint8_t)(y >> 3);
        uint8_t leftVisible = (g_draw_leftVisibleRows[rowIndex] & screenBit) ? 1 : 0;
        uint8_t visible = 0;
        uint8_t texColor = 0;

        if (texY < entities[i].sprite->height)
        {
          uint16_t idx = (uint16_t)(texY * entities[i].sprite->width + texX);
          uint8_t shift = (uint8_t)(idx & 7);
          const uint8_t *pair = &entities[i].sprite->data[usedSprite][(idx >> 3) * 2];
          uint8_t texBit = (uint8_t)(1U << shift);

          visible = ((uint8_t)(~pair[0]) & texBit) ? 1 : 0;
          texColor = (pair[1] & texBit) ? 1 : 0;
        }

        if (visible)
          g_draw_currentVisibleRows[rowIndex] |= screenBit;

        if ((uint8_t)(visible ^ leftVisible))
        {
          edgeVClearMask |= screenBit;
        }
        else if ((uint8_t)(visible ^ prevVisibleInStripe))
        {
          edgeHClearMask |= screenBit;
        }
        else if (visible)
        {
          if (texColor)
            fillMask |= screenBit;
          else
            fillClearMask |= screenBit;
        }

        texYPos += texYStep;
        prevVisibleInStripe = visible;
      }

      if (fillMask || fillClearMask || edgeHClearMask || edgeVClearMask) {
        uint8_t mergedClear = fillClearMask | edgeHClearMask;
        for (uint8_t p = 0; p < pixelStride; p++)
          display_buffer[bufferIndex + p] &= ~mergedClear;
        for (uint8_t p = 0; p < pixelStride; p++)
          display_buffer[bufferIndex + p] |= fillMask;

        if (edgeVClearMask)
          display_buffer[bufferIndex] &= ~edgeVClearMask;
      }

      for (uint8_t r = 0; r < 8; r++)
        g_draw_leftVisibleRows[r] = g_draw_currentVisibleRows[r];
    }
  }
}

void EnemyAi(player_t *player, entity_t *entities, int amount, map_t *map){
  for(int i = 0; i < amount; i++){
    entity_t *e = &entities[i];
    if(e->health <= 0)
      continue;

    fx_t dx = fx_sub(
        fx_add(
            fx_add(player->posX, fx_mul(player->dirX, e->movementModifier)),
            fx_mul(fx_neg(player->dirY), e->lateralModifier)
        ),
        e->posX
    );
    fx_t dy = fx_sub(
        fx_add(
            fx_add(player->posY, fx_mul(player->dirY, e->movementModifier)),
            fx_mul(player->dirX, e->lateralModifier)
        ),
        e->posY
    );

    e->lineOfSight = (e->distance < FX(100)) ? 1 : 0;

    // Keep a small body radius around the player to prevent overlap/pushing.
    if (e->distance < FX_RAW(64) && e->lineOfSight) {
      e->walking = 0;
      continue;
    }

    // Move towards player if in line of sight
    if (e->lineOfSight) {
      fx_t invDistance = fx_div(FX(1), e->distance);
      fx_t dirX = fx_mul(dx, invDistance);
      fx_t dirY = fx_mul(dy, invDistance);
      uint8_t tile;

      // Check for wall collisions before moving
      tile = MAP_AT(map, FX_I(fx_add(e->posX, fx_mul(dirX, ENEMY_MOVE_SPEED))), FX_I(e->posY));

      if (tile <= 0x00 || tile >= 0x0f)
        e->posX = fx_add(e->posX, fx_mul(dirX, ENEMY_MOVE_SPEED));

      tile = MAP_AT(map, FX_I(e->posX), FX_I(fx_add(e->posY, fx_mul(dirY, ENEMY_MOVE_SPEED))));

      if (tile <= 0x00 || tile >= 0x0f)
        e->posY = fx_add(e->posY, fx_mul(dirY, ENEMY_MOVE_SPEED));
      
      e->walking = 1;
    } else {
      e->walking = 0;
    }
  }
}

void EnemyRandomMovement(entity_t *entities, int amount){
    static int prevFrames = 0;
    prevFrames++;
    if (prevFrames > 20) {
        prevFrames = 0;
        for(int i = 0; i < amount; i++){
            if(entities[i].health <= 0 || entities[i].lineOfSight == 0)
              continue;

            int16_t amplitude = entities[i].movementModifier;
            uint16_t span;
            int32_t value;

            if (amplitude < 0) amplitude = -amplitude;
            if (amplitude == 0) {
              entities[i].lateralModifier = 0;
              continue;
            }

            span = amplitude << 1;
            value = ((int32_t)((uint16_t)rand16() % span)) - amplitude;
            entities[i].lateralModifier = (fx_t)value;
        }
    }
}

