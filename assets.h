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

typedef struct
{
    fx_t posX, posY; // position
    fx_t dirX, dirY; // direction vector
    fx_t planeX, planeY; // camera plane
    fx_t angle; // rotation angle (fx angle units, 512 = full turn)
    fx_t zBuffer[3];
} player_t;

typedef struct {
    uint8_t start;
    uint8_t length;
} line_t;

line_t frame_buffer[2][48];

#endif	/* ASSETS_H */

