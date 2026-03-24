#include <xc.h>
#include <stdint.h>
#include "globals.h"
#include "dogm128_fast.h"

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
{0x04,0x0A,0x11}, // 60 <
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

static uint8_t paint_counter = 0;

uint8_t dogm_fb[1024];

static void spi_write(uint8_t d)
{
    SSPBUF = d;
    while (!PIR1bits.SSPIF) { }
    PIR1bits.SSPIF = 0;
}
static void lcd_cmd(uint8_t c)
{
    DOGM_A0_LAT = 0;
    DOGM_CS_LAT = 0;
    spi_write(c);
    DOGM_CS_LAT = 1;
}

static void lcd_data(uint8_t d)
{
    DOGM_A0_LAT = 1;
    DOGM_CS_LAT = 0;
    spi_write(d);
    DOGM_CS_LAT = 1;
}

static void spi_init(void)
{
    DOGM_SCK_TRIS  = 0;   // RB1 = SCK out
    DOGM_MOSI_TRIS = 0;   // RC7 = SDO out

    SSPSTAT = 0x40;       // CKE=1, SMP=0
    SSPCON1 = 0x20;       // SPI master, Fosc/4, SSPEN=1

    PIR1bits.SSPIF = 0;
}

_Bool paint(dogm128_color_t color)
{
    paint_counter++;
    switch(color)
    {
        case DISP_COL_WHITE:
            return 0;
            break;
        case DISP_COL_BLACK:
            return 1;
            break;
        case DISP_COL_GREY:
            return paint_counter & 0b1;
            break;
        case DISP_COL_LIGHT_GREY:
            if (paint_counter >= 3) paint_counter = 0;
            return paint_counter == 0;
            break;
        case DISP_COL_DARK_GREY:
            if (paint_counter >= 3) paint_counter = 0;
            return paint_counter != 0;
            break;
        default:
            return 0;
            break;
    }
}

void dogm128_init(void)
{
    DOGM_CS_TRIS = 0;
    DOGM_A0_TRIS = 0;
    DOGM_RST_TRIS = 0;

    DOGM_CS_LAT = 1;

    spi_init();

    DOGM_RST_LAT = 0;
    __delay_ms(1000);
    DOGM_RST_LAT = 1;

    lcd_cmd(0x40);
    lcd_cmd(0xA1);
    lcd_cmd(0xC0);
    lcd_cmd(0xA6);
    lcd_cmd(0xA2);
    lcd_cmd(0x2F);
    lcd_cmd(0xF8);
    lcd_cmd(0x00);
    lcd_cmd(0x27);   // resistor ratio
    dogm128_contrast(0x14);
    lcd_cmd(0xAC);
    lcd_cmd(0x00);
    lcd_cmd(0xAF);
    
    dogm128_clear();
    dogm128_refresh();
}

void dogm128_refresh(void)
{
    uint8_t page, x;

    for (page = 0; page < 8; page++)
    {
        lcd_cmd(0xB0 | page);
        lcd_cmd(0x10);
        lcd_cmd(0x00);

        DOGM_A0_LAT = 1;
        DOGM_CS_LAT = 0;

        for (x = 0; x < 128; x++)
            spi_write(dogm_fb[((uint16_t)page << 7) + x]);

        DOGM_CS_LAT = 1;
    }
}

void dogm128_contrast(uint8_t v)
{
    lcd_cmd(0x81);
    lcd_cmd(v & 0x3F);
}

void dogm128_clear(void)
{
    for(uint16_t i=0;i<1024;i++)
        dogm_fb[i]=0;
}

void dogm128_fill(void)
{
    for(uint16_t i=0;i<1024;i++)
        dogm_fb[i]=0xFF;
}

void dogm128_pixel(uint8_t x, uint8_t y, dogm128_color_t color)
{
    if(x>=128 || y>=64) return;

    uint16_t i = (y>>3)*128 + x;
    uint8_t m = 1<<(y&7);

    if(paint(color))
        dogm_fb[i] |= m;
    else
        dogm_fb[i] &= ~m;
}

void dogm128_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h, dogm128_color_t color)
{
    dogm128_hline(x,y,w,color);
    dogm128_hline(x,y+h-1,w,color);
    dogm128_vline(x,y,h,color);
    dogm128_vline(x+w-1,y,h,color);
}

void dogm128_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, dogm128_color_t color)
{
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT || w == 0 || h == 0) return;

    if ((uint16_t)x + w > DOGM_WIDTH)
        w = DOGM_WIDTH - x;

    if ((uint16_t)y + h > DOGM_HEIGHT)
        h = DOGM_HEIGHT - y;

    while (w--)
    {
        dogm128_vline(x++, y, h, color);
    }
    paint_counter++;
}

void dogm128_hline(uint8_t x, uint8_t y, uint8_t w, dogm128_color_t color)
{
    if (y >= DOGM_HEIGHT || x >= DOGM_WIDTH || w == 0) return;

    if ((uint16_t)x + w > DOGM_WIDTH)
        w = DOGM_WIDTH - x;

    {
        uint16_t i = ((uint16_t)(y >> 3) << 7) + x;
        uint8_t m = (uint8_t)(1u << (y & 7));

        while (w--)
        {
            if (paint(color))
                dogm_fb[i] |= m;
            else
                dogm_fb[i] &= (uint8_t)~m;

            i++;
        }
    }
    paint_counter++;
}

void dogm128_vline(uint8_t x, uint8_t y, uint8_t h, dogm128_color_t color)
{
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT || h == 0) return;

    if ((uint16_t)y + h > DOGM_HEIGHT)
        h = DOGM_HEIGHT - y;

    while (h--)
    {
        uint16_t i = ((uint16_t)(y >> 3) << 7) + x;
        uint8_t m = (uint8_t)(1u << (y & 7));

        if (paint(color))
            dogm_fb[i] |= m;
        else
            dogm_fb[i] &= (uint8_t)~m;

        y++;
    }
    paint_counter++;
}

void dogm128_line(int x0, int y0, int x1, int y1, dogm128_color_t color)
{
    int dx, dy, sx, sy, err, e2;

    if (x0 == x1)
    {
        if (y1 < y0)
        {
            int t = y0; y0 = y1; y1 = t;
        }

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
        if (x1 < x0)
        {
            int t = x0; x0 = x1; x1 = t;
        }

        if (y0 >= 0 && y0 < DOGM_HEIGHT && x1 >= 0 && x0 < DOGM_WIDTH)
        {
            if (x0 < 0) x0 = 0;
            if (x1 >= DOGM_WIDTH) x1 = DOGM_WIDTH - 1;
            dogm128_hline((uint8_t)x0, (uint8_t)y0, (uint8_t)(x1 - x0 + 1), color);
        }
        return;
    }

    dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    sx = (x0 < x1) ? 1 : -1;
    dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    sy = (y0 < y1) ? 1 : -1;
    err = dx + dy;

    for (;;)
    {
        if ((unsigned)x0 < DOGM_WIDTH && (unsigned)y0 < DOGM_HEIGHT)
        {
            uint16_t i = (((uint16_t)(y0 >> 3)) << 7) + (uint8_t)x0;
            uint8_t m = (uint8_t)(1u << (y0 & 7));

            if (paint(color))
                dogm_fb[i] |= m;
            else
                dogm_fb[i] &= (uint8_t)~m;
        }

        if (x0 == x1 && y0 == y1) break;

        e2 = err << 1;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
    paint_counter++;
}

void dogm128_char(uint8_t x, uint8_t y, char c)
{
    uint8_t i, b, col;

    if (c >= 'a' && c <= 'z')
        c -= 32;   // map lowercase to uppercase

    if ((uint8_t)c < 32 || (uint8_t)c > 96)
        c = 32;

    c -= 32;

    for (i = 0; i < 3; i++)
    {
        col = font3x5[(uint8_t)c][i];

        for (b = 0; b < 5; b++)
        {
            if (col & (1u << b))
                dogm128_pixel(x + i, y + b, 1);
            else
                dogm128_pixel(x + i, y + b, 0);
        }
    }
}

void dogm128_text(uint8_t x, uint8_t y, const char *s)
{
    while (*s)
    {
        dogm128_char(x, y, *s++);
        x += 4;   // 3px glyph + 1px space
    }
}