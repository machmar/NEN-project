/* 
 * File:   fx8.h
 * Author: 247022
 *
 * Created on 24. b?ezna 2026, 14:13
 */

#ifndef FX8_H
#define FX8_H

#include <stdint.h>

typedef int16_t fx_t;

/* 8.8 fixed-point basics */
#define FX_SHIFT        8
#define FX_ZERO         ((fx_t)0)
#define FX_ONE          ((fx_t)256)
#define FX_HALF         ((fx_t)128)
#define FX_TWO          ((fx_t)512)

#define FX_FROM_INT(x)  ((fx_t)((x) << FX_SHIFT))
#define FX(x)           (FX_FROM_INT(x))
#define FX_TO_INT(x)    ((int16_t)((x) >> FX_SHIFT))
#define FX_I(x)         (FX_TO_INT(x))
#define FX_FRAC(x)      ((uint8_t)((x) & 0xFF))
#define FX_RAW(x)       ((fx_t)(x))

/* angle unit: full turn = 2.0 = 512 raw */
#define FX_ANGLE_FULL   ((fx_t)512)
#define FX_ANGLE_HALF   ((fx_t)256)
#define FX_ANGLE_QUAD   ((fx_t)128)

/* fast ops */
static inline fx_t fx_add(fx_t a, fx_t b) { return (fx_t)(a + b); }
static inline fx_t fx_sub(fx_t a, fx_t b) { return (fx_t)(a - b); }
static inline fx_t fx_neg(fx_t a)         { return (fx_t)(-a); }
static inline fx_t fx_abs(fx_t a)         { return (a < 0) ? (fx_t)(-a) : a; }

/* 8.8 * 8.8 -> 8.8 */
static inline fx_t fx_mul(fx_t a, fx_t b)
{
    return (fx_t)(((int32_t)a * (int32_t)b) >> FX_SHIFT);
}

/* 8.8 / 8.8 -> 8.8 */
static inline fx_t fx_div(fx_t a, fx_t b)
{
    return (fx_t)(((int32_t)a << FX_SHIFT) / b);
}

/* integer helpers */
static inline fx_t fx_mul_i(fx_t a, int8_t b)
{
    return (fx_t)(a * b);
}

static inline fx_t fx_div_i(fx_t a, int8_t b)
{
    return (fx_t)(a / b);
}

static inline fx_t fx_abs_fast(fx_t a)
{
    return (a < 0) ? (fx_t)(-a) : a;
}

static inline fx_t fx_inv_clamped(fx_t x)
{
    fx_t ax = fx_abs_fast(x);

    /* clamp tiny magnitudes to avoid reciprocal overflow */
    if (ax < FX_RAW(4))          /* 4/256 = 0.015625 */
        return FX_RAW(32767);

    return fx_div(FX_ONE, ax);
}

static inline fx_t fx_from_ratio(int16_t num, int16_t den)
{
    return (fx_t)(((int32_t)num << 8) / den);
}

/* trig */
fx_t fx_sin(fx_t a);
fx_t fx_cos(fx_t a);

#endif
