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
#include <stdint.h>

/* Forward declarations so map_t can reference both types before they are defined */
typedef struct dialogue_t dialogue_t;
typedef struct player_t player_t;

typedef struct {
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
    uint8_t DefaultSpwanPoint[2];
    const dogm128_bitmap_t *minimap;
    uint8_t minimapScrollthresholds[2];
    uint8_t minimapTopLeftOffset[2];
    const dogm128_bitmap_t *Banner;
    /* Called when player steps on (stepOn=1) or off (stepOn=0) an event tile (0x30-0x3F).
     * eventNum is the lower nibble of the tile value (0-15). */
    void (*OnEventTile)(uint8_t eventNum, _Bool stepOn, player_t *player, const dialogue_t **pDialogue);
    /* Called only when player steps onto a dialogue tile (0x40-0xEF).
     * pDialogue points to the active dialogue pointer so the callback can set it. */
    void (*OnDialogueTile)(uint8_t tileVal, const dialogue_t **pDialogue);
} map_t;

extern map_t SmallMap;
extern map_t BigMap;
extern map_t TestMap;
extern map_t AgentOrangeMap;
extern map_t WallDemoMap;

/* Callback set by main.c; called by map event handlers with two arbitrary parameters. */
extern void (*MapEventCallback)(uint8_t param1, uint8_t param2);

const extern dogm128_bitmap_t WallDemo_banner;

struct dialogue_t{
    const char* text;
    const millis_t timeLength;
    const dialogue_t *nextDialogue;
    uint8_t lineBreaks[5];
    uint8_t textOrigins[5][2];
    uint8_t rectangleOrigin[2];
    uint8_t rectangleSize[2];
};

extern const dialogue_t dialogue_Room1;
extern const dialogue_t dialogue_Room2;

extern const dogm128_bitmap_t wiggleLineBitmap;

extern const dogm128_bitmap_t item_hand;
extern const dogm128_bitmap_t item_knife;
extern const dogm128_bitmap_t item_gun;

extern const dogm128_bitmap_t HUD_hpImage;

extern const dogm128_bitmap_masked_t pov_knife_hold;
extern const dogm128_bitmap_masked_t pov_knife_use;
extern const dogm128_bitmap_masked_t pov_gun_hold;
extern const dogm128_bitmap_masked_t pov_gun_use;

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
    fx_t zBuffer[48];
    uint8_t health;
    uint8_t kills;
    item_t currentItem;
};

typedef struct {
    uint8_t start;
    uint8_t length;
    uint8_t type;
} line_t;

typedef struct
{
    uint8_t width;
    uint8_t height;
    const uint8_t *data[5];
} spriteData_t;

typedef struct
{
    fx_t posX, posY;
    fx_t distance;
    uint8_t health;
    _Bool lineOfSight;
    _Bool walking;
    fx_t hitDistance;
    uint8_t hitDelayFrames;
    spriteData_t *sprite;
    fx_t ratio; // Ratio of width to height (<1 means wider than taller, >1 means taller than wider)
    fx_t heightOffset;  // Vertical offset by the number of pixels (<0 means lower, >0 means higher)
    fx_t movementModifier; // Forward offset from player along view direction.
    fx_t lateralModifier; // Side offset from player; +right, -left relative to player facing.

}entity_t;

extern spriteData_t blobSprite;
extern spriteData_t chapadloSprite;
extern spriteData_t ctyrruckaSprite;
extern spriteData_t soilderSprite;

#endif	/* ASSETS_H */

