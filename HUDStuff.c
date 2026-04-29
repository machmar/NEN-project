#include <stddef.h>

#include "HUDStuff.h"
#include "fx8.h"
#include "utils.h"
#include <stdint.h>

void HUD_DrawBanner(const dogm128_bitmap_t *text)
{
    dogm128_hline(10, 0, 75, DISP_COL_WHITE);
    dogm128_hline(10, 1, 75, DISP_COL_WHITE);
    dogm128_hline(11, 2, 73, DISP_COL_WHITE);
    dogm128_hline(11, 3, 73, DISP_COL_WHITE);
    dogm128_hline(12, 4, 71, DISP_COL_WHITE);
    dogm128_hline(12, 5, 71, DISP_COL_WHITE);
    dogm128_hline(13, 6, 69, DISP_COL_WHITE);
    dogm128_hline(13, 7, 69, DISP_COL_WHITE);
    dogm128_hline(13, 8, 69, DISP_COL_BLACK);
    dogm128_line(9, 0, 12, 7, DISP_COL_BLACK);
    dogm128_line(85, 0, 82, 7, DISP_COL_BLACK);
    dogm128_blit_or(10, 0, text, 0);
}

void HUD_DrawMap(map_t *map, const player_t *player)
{
    fx_t posY = player->posY;
    fx_t mapHeight = map->minimap->h;

    int16_t static height_scroll = 0;
    uint8_t max_scroll = 0;
    if (mapHeight > 32)
        max_scroll = mapHeight - 32;
    uint8_t top_edge_scroll_threshold = map->minimapScrollthresholds[0];
    uint8_t bottom_edge_scroll_threshold = map->minimapScrollthresholds[1];
    uint8_t x_player_offset = map->minimapTopLeftOffset[0];
    uint8_t y_player_offset = map->minimapTopLeftOffset[1];
    uint8_t static const x_loc = 96;
    uint8_t static const y_loc = 0;

    if (FX_I(posY) - height_scroll < top_edge_scroll_threshold && height_scroll > 0)
        height_scroll--;

    if (FX_I(posY) - height_scroll > bottom_edge_scroll_threshold && height_scroll < max_scroll)
        height_scroll++;

    dogm128_blit_or(x_loc, (int16_t)y_loc - height_scroll, map->minimap, 32 + height_scroll);

    static uint8_t show = 0;
    if (show & 0b100)
        dogm128_pixel(((uint8_t)FX_I(player->posX)) + x_loc + x_player_offset,
                      (uint8_t)FX_I(posY) + (uint8_t)(y_loc - height_scroll) + y_player_offset,
                      DISP_COL_BLACK);
    show++;
}

void HUD_DrawBorders()
{
    dogm128_vline(96, 0, 64, DISP_COL_BLACK);
    dogm128_hline(96, 32, 32, DISP_COL_BLACK);
    dogm128_hline(96, 47, 32, DISP_COL_BLACK);
    dogm128_blit_or(111, 32, &wiggleLineBitmap, 0);
}

void HUD_DrawItem(item_t item)
{
    if (item >= ITEM_COUNT)
        return;

    switch (item)
    {
    case ITEM_HAND:
        dogm128_blit_or(96, 32, &item_hand, 0);
        break;
    case ITEM_KNIFE:
        dogm128_blit_or(96, 32, &item_knife, 0);
        break;
    case ITEM_GUN:
        dogm128_blit_or(96, 32, &item_gun, 0);
        break;
    }
}

void HUD_DrawCompass(fx_t angle, fx_t dirX, fx_t dirY)
{
    static const int8_t pointerPoints[3][2] = {
        {6, 0},
        {-1, -2},
        {-1, 2}};
    uint8_t static rx[3], ry[3];
    fx_t static prevAngle = INT16_MAX;
    if (prevAngle != angle)
    {
        prevAngle = angle;
        for (uint8_t i = 0; i < 3; i++)
        {
            rx[i] = 120 + FX_I(fx_sub(fx_mul(FX(pointerPoints[i][0]), dirX), fx_mul(FX(pointerPoints[i][1]), dirY)));
            ry[i] = 40 + FX_I(fx_add(fx_mul(FX(pointerPoints[i][0]), dirY), fx_mul(FX(pointerPoints[i][1]), dirX)));
        }
    }
    dogm128_line(rx[0], ry[0], rx[1], ry[1], DISP_COL_BLACK);
    dogm128_line(rx[1], ry[1], rx[2], ry[2], DISP_COL_BLACK);
    dogm128_line(rx[2], ry[2], rx[0], ry[0], DISP_COL_BLACK);
    dogm128_pixel(120, 40, DISP_COL_BLACK); // anchor point
    dogm128_pixel(120, 34, DISP_COL_BLACK); // four direction dots
    dogm128_pixel(120, 45, DISP_COL_BLACK);
    dogm128_pixel(127, 40, DISP_COL_BLACK);
    dogm128_pixel(114, 40, DISP_COL_BLACK);
}

void HUD_DrawStats(uint8_t health, uint8_t kills)
{
    char buf[5];
    dogm128_blit_or(98, 50, &HUD_hpImage, 14);
    utoa_mine(kills, buf, 3);
    dogm128_text(114, 50, buf);
    utoa_mine(health, buf, 1);
    dogm128_text(108, 57, buf);
}

void HUD_DrawItemPOV(const item_t playerItem, _Bool use)
{
    switch (playerItem)
    {
    case ITEM_HAND:
        break;
    case ITEM_KNIFE:
        if (use)
            dogm128_blit_aligned_masked(33, 40, &pov_knife_use);
        else
            dogm128_blit_aligned_masked(28, 40, &pov_knife_hold);
        break;
    case ITEM_GUN:
        if (use)
            dogm128_blit_aligned_masked(35, 40, &pov_gun_use);
        else
            dogm128_blit_aligned_masked(59, 40, &pov_gun_hold);
        break;
    default:
        break;
    }
}

_Bool HUD_DrawDialogue(const dialogue_t **dialogue, _Bool advance)
{
    if (*dialogue == NULL)
        return 0;

    static const dialogue_t *prevDialogue = NULL;
    static millis_t startTime = 0;

    const dialogue_t *d = *dialogue;

    if (d != prevDialogue)
    {
        startTime = millis;
        prevDialogue = d;
    }

    if (advance || millis - startTime >= d->timeLength)
    {
        startTime = millis;
        *dialogue = d->nextDialogue;
        prevDialogue = *dialogue;
        if (*dialogue == NULL)
            return 0;
        d = *dialogue;
    }

    uint8_t rx = d->rectangleOrigin[0];
    uint8_t ry = d->rectangleOrigin[1];
    uint8_t rw = d->rectangleSize[0];
    uint8_t rh = d->rectangleSize[1];

    dogm128_fill_rect(rx, ry, rw, rh, DISP_COL_WHITE);

    uint8_t lineCount = 0;
    for (uint8_t i = 0; i < 5; i++)
    {
        if (i > 0 && d->lineBreaks[i] == 0)
            break;
        if (d->text[d->lineBreaks[i]] == '\0')
            break;
        lineCount++;
    }

    for (uint8_t i = 0; i < lineCount; i++)
    {
        uint8_t tx = d->textOrigins[i][0];
        uint8_t ty = d->textOrigins[i][1];
        uint8_t start = d->lineBreaks[i];
        uint8_t end = (i + 1 < lineCount) ? d->lineBreaks[i + 1] - 1 : 255;

        for (uint8_t j = start; j != end && d->text[j] != '\0'; j++)
        {
            dogm128_char(tx, ty, d->text[j]);
            tx += 4;
        }
    }

    return 1;
}

uint8_t HUD_GetLEDHP(fx_t health)
{
    static millis_t PrevMill = 0;
    static _Bool blink_state = 0;
    uint8_t out = 0xff >> (8 - health);

    static const uint16_t blinkSpeed[3] = {10, 200, 500};
    if (health > 2)
    {
        blink_state = 1;
    }
    else
    {
        if (millis - PrevMill >= blinkSpeed[health <= 2 ? health : 1])
        {
            PrevMill = millis;
            blink_state = !blink_state;
        }
    }

    return blink_state ? out : 0x00;
}
