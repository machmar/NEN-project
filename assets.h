/* 
 * File:   assets.h
 * Author: 247022
 *
 * Created on 31. b?ezna 2026, 14:46
 */

#ifndef ASSETS_H
#define	ASSETS_H

#include "dogm128_fast.h"
#include "fx8.h"

typedef struct {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
    uint8_t DefaultSpwanPoint[2];
    dogm128_bitmap_t *minimap;
    uint8_t minimapScrollthresholds[2];
    uint8_t minimapTopLeftOffset[2];
    dogm128_bitmap_t *Banner;
} map_t;

extern map_t SmallMap;
extern map_t BigMap;
extern map_t TestMap;
extern map_t AgentOrangeMap;

extern dogm128_bitmap_t wiggleLineBitmap;

extern dogm128_bitmap_t item_hand;
extern dogm128_bitmap_t item_knife;
extern dogm128_bitmap_t item_gun;

extern dogm128_bitmap_t HUD_hpImage;

typedef enum {
    ITEM_HAND = 0,
    ITEM_KNIFE,
    ITEM_GUN,
    ITEM_COUNT
} item_t;

typedef struct
{
    fx_t posX, posY; // position
    fx_t dirX, dirY; // direction vector
    fx_t planeX, planeY; // camera plane
    fx_t angle; // rotation angle (fx angle units, 512 = full turn)
    fx_t zBuffer[3];
    uint8_t health;
    uint8_t kills;
    item_t currentItem;
} player_t;

typedef struct {
    uint8_t start;
    uint8_t length;
} line_t;

line_t frame_buffer[1][48];

#endif	/* ASSETS_H */

