#include <stdint.h>
#include <string.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "dogm128_fast.h"

#define DOGM_I2C i2c1
#define DOGM_I2C_ADDR 0x3C
#define DOGM_I2C_SDA_PIN 26
#define DOGM_I2C_SCL_PIN 27
#define DOGM_I2C_BAUDRATE 3000000u

static const uint8_t font3x5[][3] = {
    {0x00,0x00,0x00}, // 32 space
    {0x00,0x17,0x00}, // 33 !
    {0x03,0x00,0x03}, // 34 "
    {0x1F,0x0A,0x1F}, // 35 #
    {0x12,0x1F,0x09}, // 36 $
    {0x19,0x04,0x13}, // 37 %
    {0x0A,0x15,0x1A}, // 38 &
    {0x00,0x03,0x00}, // 39 '
    {0x0E,0x11,0x00}, // 40 (
    {0x00,0x11,0x0E}, // 41 )
    {0x05,0x02,0x05}, // 42 *
    {0x04,0x0E,0x04}, // 43 +
    {0x10,0x08,0x00}, // 44 ,
    {0x04,0x04,0x04}, // 45 -
    {0x00,0x10,0x00}, // 46 .
    {0x18,0x04,0x03}, // 47 /
    {0x1F,0x11,0x1F}, // 48 0
    {0x12,0x1F,0x10}, // 49 1
    {0x1D,0x15,0x17}, // 50 2
    {0x11,0x15,0x1F}, // 51 3
    {0x07,0x04,0x1F}, // 52 4
    {0x17,0x15,0x1D}, // 53 5
    {0x1F,0x15,0x1D}, // 54 6
    {0x01,0x01,0x1F}, // 55 7
    {0x1F,0x15,0x1F}, // 56 8
    {0x17,0x15,0x1F}, // 57 9
    {0x00,0x0A,0x00}, // 58 :
    {0x00,0x14,0x00}, // 59 ;
    {0x04,0x0A,0x11}, // 60 
    {0x0A,0x0A,0x0A}, // 61 =
    {0x11,0x0A,0x04}, // 62 >
    {0x01,0x15,0x03}, // 63 ?
    {0x0E,0x15,0x16}, // 64 @
    {0x1E,0x05,0x1E}, // 65 A
    {0x1F,0x15,0x0A}, // 66 B
    {0x0E,0x11,0x11}, // 67 C
    {0x1F,0x11,0x0E}, // 68 D
    {0x1F,0x15,0x15}, // 69 E
    {0x1F,0x05,0x05}, // 70 F
    {0x0E,0x11,0x1D}, // 71 G
    {0x1F,0x04,0x1F}, // 72 H
    {0x11,0x1F,0x11}, // 73 I
    {0x08,0x10,0x0F}, // 74 J
    {0x1F,0x04,0x1B}, // 75 K
    {0x1F,0x10,0x10}, // 76 L
    {0x1F,0x06,0x1F}, // 77 M
    {0x1F,0x0E,0x1F}, // 78 N
    {0x0E,0x11,0x0E}, // 79 O
    {0x1F,0x05,0x02}, // 80 P
    {0x0E,0x19,0x1E}, // 81 Q
    {0x1F,0x0D,0x16}, // 82 R
    {0x12,0x15,0x09}, // 83 S
    {0x01,0x1F,0x01}, // 84 T
    {0x0F,0x10,0x0F}, // 85 U
    {0x07,0x18,0x07}, // 86 V
    {0x1F,0x0C,0x1F}, // 87 W
    {0x1B,0x04,0x1B}, // 88 X
    {0x03,0x1C,0x03}, // 89 Y
    {0x19,0x15,0x13}, // 90 Z
    {0x00,0x1F,0x11}, // 91 [
    {0x03,0x04,0x18}, // 92 backslash
    {0x11,0x1F,0x00}, // 93 ]
    {0x02,0x01,0x02}, // 94 ^
    {0x10,0x10,0x10}, // 95 _
    {0x01,0x02,0x00}, // 96 `
};

uint8_t dogm_fb[1024];

static void i2c_write_raw(const uint8_t *data, size_t len)
{
    (void)i2c_write_blocking(DOGM_I2C, DOGM_I2C_ADDR, data, len, false);
}

static void lcd_cmd(uint8_t c)
{
    const uint8_t tx[2] = {0x00u, c};
    i2c_write_raw(tx, sizeof(tx));
}

static void lcd_data(const uint8_t *data, size_t len)
{
    uint8_t tx[17];
    tx[0] = 0x40u;

    while (len > 0)
    {
        size_t chunk = (len > 16u) ? 16u : len;
        memcpy(&tx[1], data, chunk);
        i2c_write_raw(tx, chunk + 1u);
        data += chunk;
        len -= chunk;
    }
}

static void display_bus_init(void)
{
    i2c_init(DOGM_I2C, DOGM_I2C_BAUDRATE);
    gpio_set_function(DOGM_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DOGM_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DOGM_I2C_SDA_PIN);
    gpio_pull_up(DOGM_I2C_SCL_PIN);
    sleep_ms(20);
}

void dogm128_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,       /* display off */
        0xD5, 0x80, /* clock divide */
        0xA8, 0x3F, /* mux ratio for 64 lines */
        0xD3, 0x00, /* display offset */
        0x40,       /* start line 0 */
        0x8D, 0x14, /* charge pump on */
        0x20, 0x02, /* page addressing mode */
        0xA1,       /* segment remap */
        0xC8,       /* COM scan dec */
        0xDA, 0x12, /* COM pins */
        0x81, 0x8F, /* contrast */
        0xD9, 0xF1, /* pre-charge */
        0xDB, 0x40, /* VCOM detect */
        0xA4,       /* display follows RAM */
        0xA6,       /* normal (not inverted) */
        0xAF        /* display on */
    };

    display_bus_init();

    for (size_t i = 0; i < sizeof(init_seq); ++i)
        lcd_cmd(init_seq[i]);

    dogm128_clear();
    dogm128_refresh();
}

void dogm128_refresh(void)
{
    const uint8_t *src = dogm_fb;
    uint8_t page;

    for (page = 0; page < 8; page++)
    {
        lcd_cmd(0xB0 | page);
        lcd_cmd(0x00);
        lcd_cmd(0x10);
        lcd_data(src, 128);
        src += 128;
    }
}

void dogm128_contrast(uint8_t v)
{
    lcd_cmd(0x81);
    lcd_cmd(v);
}

void dogm128_clear(void)
{
    uint8_t *p = dogm_fb;
    uint8_t *end = dogm_fb + 1024;
    while (p < end)
        *p++ = 0;
}

void dogm128_fill(void)
{
    uint8_t *p = dogm_fb;
    uint8_t *end = dogm_fb + 1024;
    while (p < end)
        *p++ = 0xFF;
}

void dogm128_invert(void)
{
    uint8_t *p = dogm_fb;
    uint8_t *end = dogm_fb + 1024;
    while (p < end)
        *p++ ^= 0xFF;
}

void dogm128_pixel(uint8_t x, uint8_t y, dogm128_color_t color)
{
    uint16_t i;
    uint8_t  m;

    if (x >= 128 || y >= 64) return;

    i = ((uint16_t)(y >> 3) << 7) + x;
    m = (uint8_t)(1u << (y & 7));

    if (color)
        dogm_fb[i] |= m;
    else
        dogm_fb[i] &= (uint8_t)~m;
}

void dogm128_hline(uint8_t x, uint8_t y, uint8_t w, dogm128_color_t color)
{
    uint8_t *p;
    uint8_t  m;

    if (y >= DOGM_HEIGHT || x >= DOGM_WIDTH || w == 0) return;

    if ((uint16_t)x + w > DOGM_WIDTH)
        w = DOGM_WIDTH - x;

    p = &dogm_fb[((uint16_t)(y >> 3) << 7) + x];
    m = (uint8_t)(1u << (y & 7));

    /* Hoist the branch out of the per-pixel loop.
       paint() is called once; for a solid color its result is constant. */
    if (color)
    {
        while (w--)
            *p++ |= m;
    }
    else
    {
        uint8_t nm = (uint8_t)~m;
        while (w--)
            *p++ &= nm;
    }
}

void dogm128_vline(uint8_t x, uint8_t y, uint8_t h, dogm128_color_t color)
{
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT || h == 0) return;

    if ((uint16_t)y + h > DOGM_HEIGHT)
        h = DOGM_HEIGHT - y;

    /* Hoist the color decision out of the per-pixel loop. */
    if (color)
    {
        while (h--)
        {
            dogm_fb[((uint16_t)(y >> 3) << 7) + x] |= (uint8_t)(1u << (y & 7));
            y++;
        }
    }
    else
    {
        while (h--)
        {
            dogm_fb[((uint16_t)(y >> 3) << 7) + x] &= (uint8_t)~(1u << (y & 7));
            y++;
        }
    }
}

void dogm128_vlineBLACK(uint8_t x, uint8_t y, uint8_t h)
{
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT || h == 0) return;

    if ((uint16_t)y + h > DOGM_HEIGHT)
        h = DOGM_HEIGHT - y;

    while (h)
    {
        uint8_t  bit  = y & 7;
        uint8_t  n    = (uint8_t)(8 - bit);
        uint8_t  mask;

        if (n > h) n = h;

        mask = (uint8_t)(((1u << n) - 1u) << bit);
        dogm_fb[((uint16_t)(y >> 3) << 7) + x] |= mask;

        y += n;
        h -= n;
    }
}

void dogm128_vlineBLACK2px(uint8_t x, uint8_t y, uint8_t h)
{
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT || h == 0) return;

    if (x == DOGM_WIDTH - 1)
    {
        dogm128_vlineBLACK(x, y, h);
        return;
    }

    if ((uint16_t)y + h > DOGM_HEIGHT)
        h = DOGM_HEIGHT - y;

    while (h)
    {
        uint8_t  bit  = y & 7;
        uint8_t  n    = (uint8_t)(8 - bit);
        uint8_t  mask;
        uint16_t i;

        if (n > h) n = h;

        mask = (uint8_t)(((1u << n) - 1u) << bit);
        i = ((uint16_t)(y >> 3) << 7) + x;

        dogm_fb[i]     |= mask;
        dogm_fb[i + 1] |= mask;

        y += n;
        h -= n;
    }
}

void dogm128_line(int x0, int y0, int x1, int y1, dogm128_color_t color)
{
    int dx, dy, sx, sy, err, e2;

    /* Fast paths for axis-aligned lines */
    if (x0 == x1)
    {
        if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
        if (x0 >= 0 && x0 < DOGM_WIDTH && y1 >= 0 && y0 < DOGM_HEIGHT)
        {
            if (y0 < 0) y0 = 0;
            if (y1 >= DOGM_HEIGHT) y1 = DOGM_HEIGHT - 1;
            dogm128_vline((uint8_t)x0, (uint8_t)y0, (uint8_t)(y1 - y0 + 1), color);
        }
        return;
    }

    if (y0 == y1)
    {
        if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
        if (y0 >= 0 && y0 < DOGM_HEIGHT && x1 >= 0 && x0 < DOGM_WIDTH)
        {
            if (x0 < 0) x0 = 0;
            if (x1 >= DOGM_WIDTH) x1 = DOGM_WIDTH - 1;
            dogm128_hline((uint8_t)x0, (uint8_t)y0, (uint8_t)(x1 - x0 + 1), color);
        }
        return;
    }

    dx  = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    sx  = (x0 < x1) ? 1 : -1;
    dy  = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    sy  = (y0 < y1) ? 1 : -1;
    err = dx + dy;

    /* Hoist the color check out of the Bresenham loop.
       On a PIC18 this eliminates a function call + switch per pixel. */
    if (color == DISP_COL_BLACK)
    {
        for (;;)
        {
            if ((unsigned)x0 < DOGM_WIDTH && (unsigned)y0 < DOGM_HEIGHT)
                dogm_fb[((uint16_t)(y0 >> 3) << 7) + (uint8_t)x0] |= (uint8_t)(1u << (y0 & 7));

            if (x0 == x1 && y0 == y1) break;
            e2 = err << 1;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
    else if (color == DISP_COL_WHITE)
    {
        for (;;)
        {
            if ((unsigned)x0 < DOGM_WIDTH && (unsigned)y0 < DOGM_HEIGHT)
                dogm_fb[((uint16_t)(y0 >> 3) << 7) + (uint8_t)x0] &= (uint8_t)~(1u << (y0 & 7));

            if (x0 == x1 && y0 == y1) break;
            e2 = err << 1;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
    else
    {
        /* Dithered colors: paint() must be called once per pixel */
        for (;;)
        {
            if ((unsigned)x0 < DOGM_WIDTH && (unsigned)y0 < DOGM_HEIGHT)
            {
                uint16_t i = ((uint16_t)(y0 >> 3) << 7) + (uint8_t)x0;
                uint8_t  m = (uint8_t)(1u << (y0 & 7));
                if (color)
                    dogm_fb[i] |= m;
                else
                    dogm_fb[i] &= (uint8_t)~m;
            }
            else

            if (x0 == x1 && y0 == y1) break;
            e2 = err << 1;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}

/* Apply a byte-mask to a horizontal run of w consecutive framebuffer bytes.
   Working at the byte level instead of bit-by-bit is 8× faster on the PIC18. */
static void fill_hrun(uint8_t *p, uint8_t w, uint8_t mask, dogm128_color_t color)
{
    if (color == DISP_COL_BLACK)
    {
        while (w--) *p++ |= mask;
    }
    else if (color == DISP_COL_WHITE)
    {
        uint8_t nm = (uint8_t)~mask;
        while (w--) *p++ &= nm;
    }
    else
    {
        while (w--)
        {
            if (color) *p |= mask;
            else              *p &= (uint8_t)~mask;
            p++;
        }
    }
}

void dogm128_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color)
{
    if (w == 0 || h == 0) return;
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT) return;
    if ((uint16_t)x + w > DOGM_WIDTH)  w = DOGM_WIDTH - x;
    if ((uint16_t)y + h > DOGM_HEIGHT) h = DOGM_HEIGHT - y;

    /* top edge */
    dogm128_hline(x, y, w, color);
    if (h == 1) return;

    /* bottom edge */
    dogm128_hline(x, (uint8_t)(y + h - 1), w, color);
    if (h == 2) return;

    /* left vertical side — use page-chunked BLACK fast-path when possible */
    if (color == DISP_COL_BLACK)
        dogm128_vlineBLACK(x, (uint8_t)(y + 1), (uint8_t)(h - 2));
    else
        dogm128_vline(x, (uint8_t)(y + 1), (uint8_t)(h - 2), color);

    if (w < 2) return;

    /* right vertical side */
    if (color == DISP_COL_BLACK)
        dogm128_vlineBLACK((uint8_t)(x + w - 1), (uint8_t)(y + 1), (uint8_t)(h - 2));
    else
        dogm128_vline((uint8_t)(x + w - 1), (uint8_t)(y + 1), (uint8_t)(h - 2), color);
}

void dogm128_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color)
{
    uint8_t  y2, page_s, page_e, p;
    uint8_t  top_mask, bot_mask;
    uint8_t *row;

    if (w == 0 || h == 0) return;
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT) return;
    if ((uint16_t)x + w > DOGM_WIDTH)  w = DOGM_WIDTH - x;
    if ((uint16_t)y + h > DOGM_HEIGHT) h = DOGM_HEIGHT - y;

    y2     = (uint8_t)(y + h - 1);
    page_s = y  >> 3;
    page_e = y2 >> 3;

    /* bits [y&7 .. 7] in the first page */
    top_mask = (uint8_t)(0xFFu << (y & 7));
    /* bits [0 .. y2&7] in the last page */
    bot_mask = (uint8_t)(0xFFu >> (7u - (y2 & 7u)));

    if (page_s == page_e)
    {
        /* entire rect fits in one page row */
        fill_hrun(&dogm_fb[((uint16_t)page_s << 7) + x], w,
                  (uint8_t)(top_mask & bot_mask), color);
        return;
    }

    /* top partial page */
    fill_hrun(&dogm_fb[((uint16_t)page_s << 7) + x], w, top_mask, color);

    /* full middle pages — write whole bytes, no read-modify-write needed */
    for (p = (uint8_t)(page_s + 1u); p < page_e; p++)
    {
        row = &dogm_fb[((uint16_t)p << 7) + x];
        if (color == DISP_COL_BLACK)
        {
            uint8_t xi = w;
            while (xi--) *row++ = 0xFFu;
        }
        else if (color == DISP_COL_WHITE)
        {
            uint8_t xi = w;
            while (xi--) *row++ = 0x00u;
        }
        else
        {
            fill_hrun(row, w, 0xFFu, color);
        }
    }

    /* bottom partial page */
    fill_hrun(&dogm_fb[((uint16_t)page_e << 7) + x], w, bot_mask, color);
}

void dogm128_blit_aligned(uint8_t x, uint8_t y, const dogm128_bitmap_t *bmp)
{
    uint8_t        page_y, pages, sp, sx;
    uint8_t       *dst;
    const uint8_t *src;

    if (!bmp || !bmp->data) return;
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT) return;
    if (bmp->w == 0 || bmp->h == 0) return;
    if (y & 7) return;

    page_y = y >> 3;
    pages  = (bmp->h + 7) >> 3;

    if ((uint16_t)x + bmp->w > DOGM_WIDTH)  return;
    if ((uint16_t)page_y + pages > 8)        return;

    dst = &dogm_fb[((uint16_t)page_y << 7) + x];
    src = bmp->data;

    for (sp = 0; sp < pages; sp++)
    {
        uint8_t *d = dst;                        /* walk dst without recomputing address */
        for (sx = 0; sx < bmp->w; sx++)
            *d++ = *src++;

        dst += 128;                              /* advance one full page row */
    }
}

void dogm128_blit_aligned_masked(uint8_t x, uint8_t y, const dogm128_bitmap_masked_t *bmp)
{
    uint8_t        page_y, pages, sp, sx;
    uint8_t       *dst;
    const uint8_t *src;

    if (!bmp || !bmp->data) return;
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT) return;
    if (bmp->w == 0 || bmp->h == 0) return;
    if (y & 7) return;

    page_y = y >> 3;
    pages  = (bmp->h + 7) >> 3;

    if ((uint16_t)x + bmp->w > DOGM_WIDTH)  return;
    if ((uint16_t)page_y + pages > 8)        return;

    dst = &dogm_fb[((uint16_t)page_y << 7) + x];
    src = bmp->data;

    for (sp = 0; sp < pages; sp++)
    {
        uint8_t *d = dst;
        for (sx = 0; sx < bmp->w; sx++)
        {
            uint8_t mask  = *src++;
            uint8_t color = *src++;
            *d = (uint8_t)((*d & mask) | (color & (uint8_t)~mask));
            d++;
        }

        dst += 128;
    }
}

void dogm128_blit_or(int16_t x, int16_t y, const dogm128_bitmap_t *bmp, uint8_t clip_h)
{
    uint8_t        shift, pages, sp, sx, x0, w, anti_shift, h;
    uint16_t       src_base;
    const uint8_t *src_ptr;
    uint8_t       *dst0_ptr;
    uint8_t       *dst1_ptr;

    if (!bmp || !bmp->data) return;
    if (bmp->w == 0 || bmp->h == 0) return;

    h = bmp->h;
    if (clip_h && clip_h < h) h = clip_h;

    if (x >= DOGM_WIDTH  || (x + (int16_t)bmp->w) <= 0) return;
    if (y >= DOGM_HEIGHT || (y + (int16_t)h) <= 0) return;

    x0 = 0;
    w  = bmp->w;
    if (x < 0) { x0 = (uint8_t)(-x); w -= x0; x = 0; }
    if ((uint8_t)x + w > DOGM_WIDTH)
        w = (uint8_t)(DOGM_WIDTH - x);

    pages      = (uint8_t)((h + 7) >> 3);
    int16_t base_page = y >> 3;
    shift      = (uint8_t)(y - (base_page << 3));
    anti_shift = 8 - shift;

    for (sp = 0; sp < pages; sp++)
    {
        int16_t dp0s = base_page + sp;
        int16_t dp1s = dp0s + 1;

        /* on the last page, mask off bits beyond the clip height */
        uint8_t src_mask = 0xFF;
        if (sp == pages - 1 && (h & 7))
            src_mask = (uint8_t)((1u << (h & 7)) - 1u);

        src_ptr = &bmp->data[(uint16_t)sp * bmp->w + x0];

        if (dp0s >= 0 && dp0s < 8)
        {
            dst0_ptr = &dogm_fb[((uint16_t)dp0s << 7) + (uint16_t)x];

            if (shift == 0)
            {
                for (sx = 0; sx < w; sx++)
                    dst0_ptr[sx] |= (uint8_t)(src_ptr[sx] & src_mask);
            }
            else if (dp1s >= 0 && dp1s < 8)
            {
                dst1_ptr = &dogm_fb[((uint16_t)dp1s << 7) + (uint16_t)x];
                for (sx = 0; sx < w; sx++)
                {
                    uint8_t byte = src_ptr[sx] & src_mask;
                    dst0_ptr[sx] |= (uint8_t)(byte << shift);
                    dst1_ptr[sx] |= (uint8_t)(byte >> anti_shift);
                }
            }
            else
            {
                for (sx = 0; sx < w; sx++)
                    dst0_ptr[sx] |= (uint8_t)((src_ptr[sx] & src_mask) << shift);
            }
        }
        else if (shift && dp1s >= 0 && dp1s < 8)
        {
            dst1_ptr = &dogm_fb[((uint16_t)dp1s << 7) + (uint16_t)x];
            for (sx = 0; sx < w; sx++)
                dst1_ptr[sx] |= (uint8_t)((src_ptr[sx] & src_mask) >> anti_shift);
        }
    }
}

void dogm128_char(uint8_t x, uint8_t y, char c)
{
    const uint8_t *col_data;
    uint8_t        i;

    /* Fold lowercase, clamp to printable range */
    if ((uint8_t)c >= 'a' && (uint8_t)c <= 'z')
        c -= 32;
    if ((uint8_t)c < 32 || (uint8_t)c > 96)
        c = 32;

    col_data = font3x5[(uint8_t)c - 32];

    /* Unroll the 3-column loop: avoids loop counter + bounds check overhead.
       Unroll the 5-bit loop too: replaces shift+test+branch with straight-line
       set/clear pairs the compiler can schedule freely. */
    for (i = 0; i < 3; i++)
    {
        uint8_t col = col_data[i];
        uint8_t cx  = x + i;

        dogm128_pixel(cx, y + 0, (col & 0x01) ? 1 : 0);
        dogm128_pixel(cx, y + 1, (col & 0x02) ? 1 : 0);
        dogm128_pixel(cx, y + 2, (col & 0x04) ? 1 : 0);
        dogm128_pixel(cx, y + 3, (col & 0x08) ? 1 : 0);
        dogm128_pixel(cx, y + 4, (col & 0x10) ? 1 : 0);
    }
}

void dogm128_text(uint8_t x, uint8_t y, const char *s)
{
    while (*s)
    {
        dogm128_char(x, y, *s++);
        x += 4;
    }
}