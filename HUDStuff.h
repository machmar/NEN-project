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


void HUD_DrawBanner(const dogm128_bitmap_t *text);
void HUD_DrawMap(map_t *map, const player_t *player);
void HUD_DrawBorders();
void HUD_DrawItem(item_t item);
void HUD_DrawCompass(fx_t angle, fx_t dirX, fx_t dirY);
void HUD_DrawStats(uint8_t health, uint8_t kills);
void HUD_DrawItemPOV(const item_t playerItem, _Bool use);
_Bool HUD_DrawDialogue(const dialogue_t **dialogue, _Bool advance);
uint8_t inline HUD_GetLEDHP(fx_t health);

#endif	/* HUDSTUFF_H */

