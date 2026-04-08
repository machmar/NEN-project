#include "HUDStuff.h"

void HUD_DrawBanner(dogm128_bitmap_t *text) {
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

void HUD_DrawMap(map_t *map, player_t *player) {
    int16_t static height_scroll = 0;
    uint8_t max_scroll = 0;
    if (map->height > 32)
        max_scroll = map->height - 32;
    uint8_t top_edge_scroll_threshold = map->minimapScrollthresholds[0];
    uint8_t bottom_edge_scroll_threshold = map->minimapScrollthresholds[1];
    uint8_t x_player_offset = map->minimapTopLeftOffset[0];
    uint8_t y_player_offset = map->minimapTopLeftOffset[1];
    uint8_t static const x_loc = 96;
    uint8_t static const y_loc = 0;

    if (FX_I(player->posY) - height_scroll < top_edge_scroll_threshold && height_scroll > 0)
        height_scroll--;

    if (FX_I(player->posY) - height_scroll > bottom_edge_scroll_threshold && height_scroll < max_scroll)
        height_scroll++;

    dogm128_blit_or(x_loc, (int16_t) y_loc - height_scroll, map->minimap, 32 + height_scroll);

    static uint8_t show = 0;
    if (show & 0b100)
        dogm128_pixel(((uint8_t) FX_I(player->posX)) + x_loc + x_player_offset,
            (uint8_t) FX_I(player->posY) + (uint8_t) (y_loc - height_scroll) + y_player_offset,
            DISP_COL_BLACK);
    show++;
}

void HUD_DrawItem(uint8_t item) {
    if (item > 2)
        return;

    switch (item) {
        case 0:
            dogm128_blit_or(96, 32, &item_hand, 0);
            break;
        case 1:
            dogm128_blit_or(96, 32, &item_knife, 0);
            break;
        case 2:
            dogm128_blit_or(96, 32, &item_gun, 0);
            break;
    }
}

void HUD_DrawCompass(player_t *player) {
    static const int8_t pointerPoints[3][2] = {
        {6, 0},
        {-1, -2},
        {-1, 2}
    };
    uint8_t static rx[3], ry[3];
    fx_t static prevAngle = INT16_MAX;
    if (prevAngle != player->angle) {
        prevAngle = player->angle;
        for (uint8_t i = 0; i < 3; i++) {
            rx[i] = 120 + FX_I(fx_sub(fx_mul(FX(pointerPoints[i][0]), player->dirX), fx_mul(FX(pointerPoints[i][1]), player->dirY)));
            ry[i] = 40 + FX_I(fx_add(fx_mul(FX(pointerPoints[i][0]), player->dirY), fx_mul(FX(pointerPoints[i][1]), player->dirX)));
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

/*void HUD_DrawStats(player_t *player) {
    char buf[5];
    dogm128_blit_or(98, 50, &HUD_hpImage, 14);
    utoa(player->kills, buf, 3);
    dogm128_text(114, 50, buf);
    utoa(player->health, buf, 1);
    dogm128_text(108, 57, buf);
}*/
