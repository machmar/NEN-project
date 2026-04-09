/* 
 * File:   HUDStuff.h
 * Author: 247022
 *
 * Created on 7. dubna 2026, 13:53
 */

#ifndef HUDSTUFF_H
#define	HUDSTUFF_H

#include "dogm128_fast.h"
#include "assets.h"

void HUD_DrawBanner(dogm128_bitmap_t *text);
void HUD_DrawMap(uint8_t x_loc, uint8_t y_loc, map_t *map, player_t *player);

#endif	/* HUDSTUFF_H */

