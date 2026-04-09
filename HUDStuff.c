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

void HUD_DrawMap(uint8_t x_loc, uint8_t y_loc, map_t *map, player_t *player) {
    int16_t static height_scroll = 0;
    uint8_t max_scroll = 0;
    if (map->height > 32)
        max_scroll = map->height - 32;
    uint8_t top_edge_scroll_threshold = map->minimapScrollthresholds[0];
    uint8_t bottom_edge_scroll_threshold = map->minimapScrollthresholds[1];
    uint8_t x_player_offset = map->minimapTopLeftOffset[0];
    uint8_t y_player_offset = map->minimapTopLeftOffset[1];

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
