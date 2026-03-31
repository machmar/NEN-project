#ifndef DOGM128_FAST_H
#define DOGM128_FAST_H

#include <xc.h>
#include <stdint.h>

#define DOGM_CS_LAT       LATCbits.LATC0
#define DOGM_CS_TRIS      TRISCbits.TRISC0

#define DOGM_A0_LAT       LATDbits.LATD0
#define DOGM_A0_TRIS      TRISDbits.TRISD0

/* optional, only if LCD reset is actually connected */
#define DOGM_RST_LAT      LATDbits.LATD1
#define DOGM_RST_TRIS     TRISDbits.TRISD1

#define DOGM_SCK_TRIS     TRISBbits.TRISB1
#define DOGM_MOSI_TRIS    TRISCbits.TRISC7

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

void dogm128_init(void);
void dogm128_refresh(void);
void dogm128_contrast(uint8_t v);

void dogm128_clear(void);
void dogm128_fill(void);

void dogm128_pixel(uint8_t x, uint8_t y, dogm128_color_t color);
void dogm128_hline(uint8_t x, uint8_t y, uint8_t w, dogm128_color_t color);
void dogm128_vline(uint8_t x, uint8_t y, uint8_t h, dogm128_color_t color);
void dogm128_line(int x0, int y0, int x1, int y1, dogm128_color_t color);
void dogm128_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color);
void dogm128_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color);

void dogm128_char(uint8_t x, uint8_t y, char c);
void dogm128_text(uint8_t x, uint8_t y, const char *s);

void AdvanceDither();

#endif