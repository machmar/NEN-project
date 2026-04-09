/* 
 * File:   utils.h
 * Author: 247022
 *
 * Created on 10. b?ezna 2026, 14:34
 */

#ifndef UTILS_H
#define	UTILS_H

static void utoa(uint16_t v, char *buf, uint8_t len) {
    char tmp[6];
    uint8_t i = 0;

    if (v == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }

    if (len == 0) {
        while (v > 0) {
            tmp[i++] = '0' + (v % 10);
            v /= 10;
        }
    } else {
        for (uint8_t cnt = 0; cnt < len; cnt++) {
            tmp[i++] = '0' + (v % 10);
            v /= 10;
        }
    }

    while (i > 0)
        *buf++ = tmp[--i];

    *buf = 0;
}

static void utoa32(uint32_t v, char *buf) {
    char tmp[11];
    uint8_t i = 0;

    if (v == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }

    while (v > 0) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }

    while (i > 0)
        *buf++ = tmp[--i];

    *buf = 0;
}

static uint16_t rng = 0xACE1;

static inline uint16_t rand16(void) {
    rng ^= rng << 7;
    rng ^= rng >> 9;
    rng ^= rng << 8;
    return rng;
}

static inline uint8_t rand128(void) {
    return rand16() & 0x7F;
}

static inline uint8_t rand64(void) {
    return rand16() & 0x3F;
}

#endif	/* UTILS_H */

