#include "fx8.h"

/*
Quarter-wave sine LUT
Input range: 0 .. 0.5 turn
Index range: 0 .. 128
Output range: 0 .. 1.0 in 8.8 fixed
Size: 129 * 2 = 258 bytes
*/
static const fx_t fx_sin_q[129] = {
      0,   3,   6,   9,  13,  16,  19,  22,  25,  28,  31,  34,  38,  41,  44,  47,
     50,  53,  56,  59,  62,  65,  68,  71,  74,  77,  80,  83,  86,  89,  92,  95,
     98, 101, 104, 107, 109, 112, 115, 118, 121, 123, 126, 129, 132, 134, 137, 140,
    142, 145, 147, 150, 152, 155, 157, 160, 162, 165, 167, 170, 172, 174, 177, 179,
    181, 183, 185, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 207, 209, 211,
    213, 215, 216, 218, 220, 221, 223, 224, 226, 227, 229, 230, 231, 233, 234, 235,
    237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250,
    251, 252, 252, 253, 253, 254, 254, 254, 255, 255, 255, 256, 256, 256, 256, 256,
    256
};

/*
Angle format:
  0.0 .. 2.0 = one full turn
Raw range per turn:
  0 .. 511

Quadrants:
  0:   0 .. 127
  1: 128 .. 255
  2: 256 .. 383
  3: 384 .. 511
*/
fx_t fx_sin(fx_t a)
{
    uint16_t p = (uint16_t)a & 0x01FF;   /* wrap to 0..511 */
    uint8_t q = (uint8_t)(p >> 7);       /* quadrant 0..3 */
    uint8_t i = (uint8_t)(p & 0x7F);     /* index 0..127 */

    switch (q)
    {
        default:
        case 0:  return  fx_sin_q[i];
        case 1:  return  fx_sin_q[128 - i];
        case 2:  return (fx_t)-fx_sin_q[i];
        case 3:  return (fx_t)-fx_sin_q[128 - i];
    }
}

fx_t fx_cos(fx_t a)
{
    return fx_sin((fx_t)(a + FX_ANGLE_QUAD));
}
