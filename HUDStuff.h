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
void HUD_DrawMap(map_t *map, player_t *player);
void HUD_DrawBorders();
void HUD_DrawItem(item_t item);
void HUD_DrawCompass(const player_t *player);
void HUD_DrawStats(const player_t *player);
_Bool HUD_DrawDialogue(const dialogue_t **dialogue, _Bool advance);
uint8_t inline HUD_GetLEDHP(const player_t *player);

#endif	/* HUDSTUFF_H */

