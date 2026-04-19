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
#include "assets.h"
#include "dogm128_fast.h"
#include "fx8.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_HIT_FRAME_DELAY 50
#define FRAMES_PER_WALK_ANIMATE 16
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
void HitPlayer(player_t *player, entity_t *entity);

/* Keep these scratch buffers static to reduce DrawEntities auto-stack pressure. */
static uint8_t g_draw_leftVisibleRows[8];
static uint8_t g_draw_currentVisibleRows[8];
static const fx_t BASE_ENEMY_MOVE_SPEED = FX(1) / 4;

static void DrawEntities_ClearRows(uint8_t *rows)
{
  for (uint8_t r = 0; r < 8; r++)
    rows[r] = 0;
}

static void DrawEntities_CopyRows(uint8_t *dst, const uint8_t *src)
{
  for (uint8_t r = 0; r < 8; r++)
    dst[r] = src[r];
}

static void DrawEntities_ApplyStripeMasks(uint8_t *display_buffer,
                                          uint16_t bufferIndex,
                                          uint8_t pixelStride,
                                          uint8_t fillMask,
                                          uint8_t fillClearMask,
                                          uint8_t edgeHClearMask,
                                          uint8_t edgeVClearMask)
{
  if (!(fillMask || fillClearMask || edgeHClearMask || edgeVClearMask))
    return;

  uint8_t mergedClear = (uint8_t)(fillClearMask | edgeHClearMask);
  uint8_t invClear = (uint8_t)(~mergedClear);
  uint8_t *dst = &display_buffer[bufferIndex];

  for (uint8_t p = 0; p < pixelStride; p++)
    dst[p] = (uint8_t)((dst[p] & invClear) | fillMask);

  // Keep vertical outline 1px wide even when coarse x stride is enabled.
  if (edgeVClearMask)
    dst[0] &= (uint8_t)(~edgeVClearMask);
}

int RenderFrame(player_t *player, const map_t *map)
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

int MoveCamera(player_t *player, const map_t *map, buttons_t buttons, const dialogue_t **pDialogue) {
    //move forward if no wall in front of you
    fx_t moveSpeed = FX_HALF; //the constant value is in squares/second
    fx_t rotSpeed = 0x0008; //the constant value is in radians/second (0.1PI per frame)
    uint8_t tile = 0; // the tile that's being walked into
    static uint8_t prevCellX = 0xFF; // 0xFF = sentinel (uninitialized)
    static uint8_t prevCellY = 0xFF;
    static uint8_t prevTile = 0;
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

    /* Tile-change detection: fire map callbacks when the player enters a new cell */
    uint8_t newCellX = (uint8_t)FX_I(player->posX);
    uint8_t newCellY = (uint8_t)FX_I(player->posY);

    if (prevCellX == 0xFF) {
        /* First call: initialise tracking without firing any callbacks */
        prevCellX = newCellX;
        prevCellY = newCellY;
        prevTile  = MAP_AT(map, newCellX, newCellY);
    } else if (newCellX != prevCellX || newCellY != prevCellY) {
        uint8_t newTile = MAP_AT(map, newCellX, newCellY);

        /* Stepped off an event tile */
        if (prevTile >= 0x30 && prevTile <= 0x3F && map->OnEventTile)
            map->OnEventTile(prevTile & 0x0F, 0, player, pDialogue);

        /* Stepped onto an event tile */
        if (newTile >= 0x30 && newTile <= 0x3F && map->OnEventTile)
            map->OnEventTile(newTile & 0x0F, 1, player, pDialogue);

        /* Stepped onto a dialogue tile (entry only, not exit) */
        if (newTile >= 0x40 && newTile <= 0xEF && map->OnDialogueTile)
            map->OnDialogueTile(newTile, pDialogue);

        prevCellX = newCellX;
        prevCellY = newCellY;
        prevTile  = newTile;
    }
}

void DrawEntities(player_t *player, entity_t* entities,  int amount, uint8_t *display_buffer, buttons_t buttons)
{
  static int prevFrames = 0;
  prevFrames++;
  int screenXStart = 0;
  int screenYStart = 0;
  uint8_t rendered = 0;
  if (amount <= 0)
      return;
  if (amount > MAX_ENTITIES)
      amount = MAX_ENTITIES;

  uint8_t renderOrder[MAX_ENTITIES];

  fx_t playerPosX = player->posX;
  fx_t playerPosY = player->posY;
  fx_t playerDirX = player->dirX;
  fx_t playerDirY = player->dirY;
  fx_t playerPlaneX = player->planeX;
  fx_t playerPlaneY = player->planeY;
  fx_t *zBuffer = player->zBuffer;

    // Precompute squared distance and initialize render order.
  for (int i = 0; i < amount; i++)
  {
      fx_t dx = fx_sub(playerPosX, entities[i].posX);
      fx_t dy = fx_sub(playerPosY, entities[i].posY);
      entities[i].distance = fx_add(fx_mul(dx, dx), fx_mul(dy, dy));
      renderOrder[i] = (uint8_t)i;
  }

    // Sort only indices (far to near), avoiding full entity struct moves.
  for (int i = 1; i < amount; i++)
  {
      uint8_t keyIndex = renderOrder[i];
      fx_t keyDistance = entities[keyIndex].distance;
      int j = i - 1;
      while (j >= 0 && entities[renderOrder[j]].distance < keyDistance)
      {
        renderOrder[j + 1] = renderOrder[j];
          j--;
      }
      renderOrder[j + 1] = keyIndex;
  }

  fx_t invDet = fx_inv_clamped(fx_sub(fx_mul(playerPlaneX, playerDirY), fx_mul(playerDirX, playerPlaneY)));

  static bool useHeld = false;
  bool useRisingEdge = buttons.use && !useHeld;
  uint8_t hitTarget = 0xFF;

  useHeld = buttons.use;

  // Render entities in sorted order.
  for (int i = 0; i < amount; i++)
  {
    entity_t *e = &entities[renderOrder[i]];
    spriteData_t *sprite = e->sprite;

    if (e->distance < FX_ZERO){
      e->lineOfSight = 0;
      e->hitDelayFrames = 30;
      e->distance = FX(127);
      continue;
    }


    // Translate sprite position relative to camera.
    fx_t spriteX = fx_sub(e->posX, playerPosX);
    fx_t spriteY = fx_sub(e->posY, playerPosY);
    fx_t cameraX = fx_sub(fx_mul(playerDirY, spriteX), fx_mul(playerDirX, spriteY));
    fx_t cameraY = fx_add(fx_mul(fx_neg(playerPlaneY), spriteX), fx_mul(playerPlaneX, spriteY));

    // Transform sprite with inverse camera matrix.
    fx_t transformX = fx_mul(invDet, cameraX);
    fx_t transformY = fx_mul(invDet, cameraY);
    if (transformY <= FX_RAW(32)){
      e->lineOfSight = 0;
      e->hitDelayFrames = 30;
      continue;
    }
  
    if ((int32_t)fx_abs_fast(transformX) > (int32_t)transformY * 2)
      continue;

    // Use one reciprocal for several projections to reduce expensive fixed-point divisions.
    fx_t invTransformY = fx_inv_clamped(transformY);
    fx_t projectedScale = fx_mul(FX(screenHeight), invTransformY);
    int spriteScreenX = VIEWPORT_HALF_W - FX_I(fx_mul(FX(VIEWPORT_HALF_W), fx_mul(transformX, invTransformY)));
    int heightOffsetScreen = FX_I(fx_mul(e->heightOffset, invTransformY));

    // Calculate projected sprite size.
    int spriteHeight = FX_I(fx_abs_fast(projectedScale));
    if (spriteHeight <= 0)
      continue;

    int drawStartY = -(spriteHeight >> 1) + VIEWPORT_HALF_H + heightOffsetScreen;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = (spriteHeight >> 1) + VIEWPORT_HALF_H + heightOffsetScreen;
    if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;
    if (drawEndY <= drawStartY)
      continue;

    int spriteWidth = FX_I(fx_abs_fast(fx_div(projectedScale, e->ratio)));
    if (spriteWidth <= 0)
      continue;

    int spriteLeft = -(spriteWidth >> 1) + spriteScreenX;
    int drawStartX = spriteLeft;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = (spriteWidth >> 1) + spriteScreenX;
    if (drawEndX >= VIEWPORT_WIDTH_PIXELS) drawEndX = VIEWPORT_WIDTH_PIXELS - 1;
    if (drawEndX <= drawStartX)
      continue;

    if (useRisingEdge) {
      int centerX = (drawStartX + drawEndX) >> 1;
      if (centerX > 40 && centerX < 56) { // aim tolerance of +-8 pixels around center of screen
        hitTarget = renderOrder[i];
      }
    }

    // Incremental texture mapping removes divisions from inner loops.
    uint8_t texWidth = sprite->width;
    uint8_t texHeight = sprite->height;
    uint16_t texXStep = (uint16_t)(texWidth << 8) / (uint16_t)spriteWidth;
    uint16_t texXPos = (uint16_t)(drawStartX - spriteLeft) * texXStep;
    uint16_t texYBase = (uint16_t)((drawStartY - heightOffsetScreen) << 8) - (screenHeight << 7) + ((uint16_t)spriteHeight << 7);
    uint16_t texYStep = (uint16_t)(texHeight << 8) / (uint16_t)spriteHeight;
    uint16_t texYStart = (uint16_t)(texYBase * texHeight) / (uint16_t)spriteHeight;
    uint16_t texXAdvance;

    static uint8_t walkSprite = 0;
    if(e->walking && prevFrames > FRAMES_PER_WALK_ANIMATE){
      walkSprite ^= 1;
      prevFrames = 0;
    }
    uint8_t usedSprite = walkSprite;
    if(e->health <= 0)
      usedSprite = 2;
    const uint8_t *spriteFrame = sprite->data[usedSprite];

    uint8_t pixelStride = (e->distance < FX(6)) ? ((e->distance < FX(3)) ? 4 : 2) : 1;
    DrawEntities_ClearRows(g_draw_leftVisibleRows);
    texXAdvance = (uint16_t)(texXStep * pixelStride);

    e->lineOfSight = 1;
    if (e->hitDelayFrames > 0)
      e->hitDelayFrames--;
    HitPlayer(player, e);

    for (uint8_t stripe = (uint8_t)drawStartX; stripe < drawEndX; stripe += pixelStride)
    {
      if (transformY >= zBuffer[stripe >> 1])
      {
        DrawEntities_ClearRows(g_draw_leftVisibleRows);
        texXPos += texXAdvance;
        continue;
      }

      uint16_t texX = texXPos >> 8;
      texXPos += texXAdvance;
      if (texX >= texWidth)
      {
        DrawEntities_ClearRows(g_draw_leftVisibleRows);
        continue;
      }

      uint16_t texYPos = texYStart;
      uint8_t rowIndex = (uint8_t)(drawStartY >> 3);
      uint8_t screenBit = (uint8_t)(1U << (drawStartY & 7));
      uint8_t fillMask = 0;
      uint8_t fillClearMask = 0;
      uint8_t edgeHClearMask = 0;
      uint8_t edgeVClearMask = 0;
      uint16_t bufferIndex = ((uint16_t)rowIndex * SCREEN_WIDTH) + stripe;
      uint8_t prevVisibleInStripe = 0;
      DrawEntities_ClearRows(g_draw_currentVisibleRows);

      for (uint8_t y = (uint8_t)drawStartY; y < drawEndY; y++)
      {
        uint16_t texY = texYPos >> 8;
        uint8_t leftVisible = (g_draw_leftVisibleRows[rowIndex] & screenBit) ? 1 : 0;
        uint8_t visible = 0;
        uint8_t texColor = 0;

        if (texY < texHeight)
        {
          uint16_t idx = (uint16_t)(texY * texWidth + texX);
          uint8_t shift = (uint8_t)(idx & 7);
          const uint8_t *pair = &spriteFrame[(idx >> 3) * 2];
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

        if (screenBit == 0x80)
        {
          DrawEntities_ApplyStripeMasks(display_buffer, bufferIndex, pixelStride,
                                        fillMask, fillClearMask, edgeHClearMask, edgeVClearMask);
          bufferIndex += SCREEN_WIDTH;
          fillMask = 0;
          fillClearMask = 0;
          edgeHClearMask = 0;
          edgeVClearMask = 0;
          screenBit = 1;
          rowIndex++;
        }
        else
        {
          screenBit <<= 1;
        }
      }

      DrawEntities_ApplyStripeMasks(display_buffer, bufferIndex, pixelStride,
                                    fillMask, fillClearMask, edgeHClearMask, edgeVClearMask);

      DrawEntities_CopyRows(g_draw_leftVisibleRows, g_draw_currentVisibleRows);
    }
  }

  if (useRisingEdge && hitTarget != 0xFF)
    HitDetection(player, &entities[hitTarget]);
}

void EnemyAi(player_t *player, entity_t *entities, int amount, map_t *map){
  static uint16_t prevFrames = 0;
  static uint16_t hitDelayFrames = 0;
  bool updateLateral = false;

  prevFrames++;
  hitDelayFrames++;
  if (prevFrames > 20) {
    prevFrames = 0;
    updateLateral = true;
  }

  for(int i = 0; i < amount; i++){
    entity_t *e = &entities[i];
    if(e->health <= 0 || e->distance == FX(127))
      continue;

    // Randomize lateral strafe periodically for all visible enemies.
    if (updateLateral && e->lineOfSight) {
      int16_t amplitude = (int16_t)e->movementModifier;

      if (amplitude < 0) amplitude = -amplitude;
      if (amplitude == 0) {
        e->lateralModifier = 0;
      } else {
        int16_t span = ((amplitude << 1) + 1);
        e->lateralModifier = (fx_t)(((int32_t)rand16() % span) - (amplitude));
      }
    }

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

      // Scaling speed with distance
      fx_t moveSpeed = fx_mul(BASE_ENEMY_MOVE_SPEED, fx_mul(e->distance, 0X0020));
      // Check for wall collisions before moving
      uint8_t tileX = MAP_AT(map, FX_I(fx_add(e->posX, fx_mul(dirX, moveSpeed))), FX_I(e->posY));
      uint8_t tileY = MAP_AT(map, FX_I(e->posX), FX_I(fx_add(e->posY, fx_mul(dirY, moveSpeed))));

      if (tileX <= 0x00 || tileX >= 0x0f)
          e->posX = fx_add(e->posX, fx_mul(dirX, moveSpeed));

      if (tileY <= 0x00 || tileY >= 0x0f)
        e->posY = fx_add(e->posY, fx_mul(dirY, moveSpeed));
      
      e->walking = 1;
    } else {
      e->walking = 0;
    }
  }
}

void HitDetection(player_t *player, entity_t *entities){
  fx_t hitDistance = FX_ZERO;
  if(player->currentItem == ITEM_GUN) hitDistance = FX(50);
  else if(player->currentItem == ITEM_KNIFE) hitDistance = FX(2);
  if (entities->distance < hitDistance) {
    entities->health = 0;
  }
}

void HitPlayer(player_t *player, entity_t *entity){
  if (entity->distance <= entity->hitDistance && entity->hitDelayFrames == 0 && entity->health > 0) {
    player->health -= 1;
    entity->hitDelayFrames = PLAYER_HIT_FRAME_DELAY;
  }
}