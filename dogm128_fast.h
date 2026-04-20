#ifndef DOGM128_FAST_H
#define DOGM128_FAST_H

#include <stdint.h>

#define DOGM_WIDTH        128
#define DOGM_HEIGHT       64
#define DOGM_FB_SIZE      1024

extern uint8_t dogm_fb[DOGM_FB_SIZE];

typedef enum {
    DISP_COL_WHITE = 0,
    DISP_COL_BLACK = 1,
    DISP_COL_GREY = 2,
    DISP_COL_LIGHT_GREY = 3,
    DISP_COL_DARK_GREY = 4,
} dogm128_color_t;

typedef struct
{
    uint8_t w;
    uint8_t h;
    const uint8_t *data;
} dogm128_bitmap_t;

typedef dogm128_bitmap_t dogm128_bitmap_masked_t;

void dogm128_init(void);
void dogm128_refresh(void);
void dogm128_contrast(uint8_t v);

void dogm128_clear(void);
void dogm128_fill(void);
void dogm128_invert(void);

void dogm128_pixel(uint8_t x, uint8_t y, dogm128_color_t color);
void dogm128_hline(uint8_t x, uint8_t y, uint8_t w, dogm128_color_t color);
void dogm128_vline(uint8_t x, uint8_t y, uint8_t h, dogm128_color_t color);
void dogm128_vlineBLACK(uint8_t x, uint8_t y, uint8_t h);
void dogm128_vlineBLACK2px(uint8_t x, uint8_t y, uint8_t h);
void dogm128_line(int x0, int y0, int x1, int y1, dogm128_color_t color);
void dogm128_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color);
void dogm128_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color);
void dogm128_blit_aligned(uint8_t x, uint8_t y, const dogm128_bitmap_t *bmp);
void dogm128_blit_aligned_masked(uint8_t x, uint8_t y, const dogm128_bitmap_masked_t *bmp);
void dogm128_blit_or(int16_t x, int16_t y, const dogm128_bitmap_t *bmp, uint8_t clip_h);

void dogm128_char(uint8_t x, uint8_t y, char c);
void dogm128_text(uint8_t x, uint8_t y, const char *s);

void AdvanceDither();

#endif