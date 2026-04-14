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
#include "utils.h"

/* Forward declarations so map_t can reference both types before they are defined */
typedef struct dialogue_t dialogue_t;
typedef struct player_t player_t;

typedef struct {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
    uint8_t DefaultSpwanPoint[2];
    dogm128_bitmap_t *minimap;
    uint8_t minimapScrollthresholds[2];
    uint8_t minimapTopLeftOffset[2];
    dogm128_bitmap_t *Banner;
    /* Called when player steps on (stepOn=1) or off (stepOn=0) an event tile (0x30-0x3F).
     * eventNum is the lower nibble of the tile value (0-15). */
    void (*OnEventTile)(uint8_t eventNum, _Bool stepOn, player_t *player, dialogue_t **pDialogue);
    /* Called only when player steps onto a dialogue tile (0x40-0xEF).
     * pDialogue points to the active dialogue pointer so the callback can set it. */
    void (*OnDialogueTile)(uint8_t tileVal, dialogue_t **pDialogue);
} map_t;

extern map_t SmallMap;
extern map_t BigMap;
extern map_t TestMap;
extern map_t AgentOrangeMap;
extern map_t WallDemoMap;

struct dialogue_t{
    char* text;
    millis_t timeLength;
    dialogue_t *nextDialogue;
    uint8_t lineBreaks[5];
    uint8_t textOrigins[5][2];
    uint8_t rectangleOrigin[2];
    uint8_t rectangleSize[2];
};

void PrecomputeAssets();

extern dialogue_t dialogue_Room1;
extern dialogue_t dialogue_Room2;

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

struct player_t
{
    fx_t posX, posY; // position
    fx_t dirX, dirY; // direction vector
    fx_t planeX, planeY; // camera plane
    fx_t angle; // rotation angle (fx angle units, 512 = full turn)
    fx_t zBuffer[3];
    uint8_t health;
    uint8_t kills;
    item_t currentItem;
};

#endif	/* ASSETS_H */

