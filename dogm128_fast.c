#include <xc.h>
#include <stdint.h>
#include "globals.h"
#include "dogm128_fast.h"

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

void dogm128_init(void)
{
    DOGM_CS_TRIS = 0;
    DOGM_A0_TRIS = 0;
    DOGM_RST_TRIS = 0;

    DOGM_CS_LAT = 1;

    spi_init();

    DOGM_RST_LAT = 0;
    __delay_ms(10);
    DOGM_RST_LAT = 1;

        lcd_cmd(0xAE);
    lcd_cmd(0xA2);
    lcd_cmd(0xA0);
    lcd_cmd(0xC8);
    lcd_cmd(0x2F);
    lcd_cmd(0x27);   // resistor ratio
    dogm128_contrast(0x14);
    lcd_cmd(0x40);
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

void dogm128_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if(x>=128 || y>=64) return;

    uint16_t i = (y>>3)*128 + x;
    uint8_t m = 1<<(y&7);

    if(color)
        dogm_fb[i] |= m;
    else
        dogm_fb[i] &= ~m;
}

void dogm128_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t c)
{
    dogm128_hline(x,y,w,c);
    dogm128_hline(x,y+h-1,w,c);
    dogm128_vline(x,y,h,c);
    dogm128_vline(x+w-1,y,h,c);
}

void dogm128_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
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
}

void dogm128_hline(uint8_t x, uint8_t y, uint8_t w, uint8_t color)
{
    if (y >= DOGM_HEIGHT || x >= DOGM_WIDTH || w == 0) return;

    if ((uint16_t)x + w > DOGM_WIDTH)
        w = DOGM_WIDTH - x;

    {
        uint16_t i = ((uint16_t)(y >> 3) << 7) + x;
        uint8_t m = (uint8_t)(1u << (y & 7));

        if (color)
        {
            while (w--)
                dogm_fb[i++] |= m;
        }
        else
        {
            m = (uint8_t)~m;
            while (w--)
                dogm_fb[i++] &= m;
        }
    }
}

void dogm128_vline(uint8_t x, uint8_t y, uint8_t h, uint8_t color)
{
    if (x >= DOGM_WIDTH || y >= DOGM_HEIGHT || h == 0) return;

    if ((uint16_t)y + h > DOGM_HEIGHT)
        h = DOGM_HEIGHT - y;

    while (h)
    {
        uint8_t page = y >> 3;
        uint8_t bit  = y & 7;
        uint8_t n    = (uint8_t)(8 - bit);
        uint8_t mask;
        uint16_t i;

        if (n > h) n = h;

        mask = (uint8_t)(((1u << n) - 1u) << bit);
        i = ((uint16_t)page << 7) + x;

        if (color) dogm_fb[i] |= mask;
        else       dogm_fb[i] &= (uint8_t)~mask;

        y += n;
        h -= n;
    }
}

void dogm128_line(int x0, int y0, int x1, int y1, uint8_t color)
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

            if (color) dogm_fb[i] |= m;
            else       dogm_fb[i] &= (uint8_t)~m;
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
}