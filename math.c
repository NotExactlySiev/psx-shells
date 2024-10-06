#include "math.h"

static int rand_r(unsigned int *seed)
{
    unsigned int a = *seed;
    a ^= 61;
    a ^= a >> 16;
    a += a << 3;
    a ^= a >> 4;
    a *= 668265263UL;
    a ^= a >> 15;
    a *= 3148259783UL;
    *seed = a;
    return (int) a;
}


unsigned int rnext(void)
{
    static unsigned int _seed = 2891583007UL;
    return rand_r(&_seed);
}

Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return (Vec3) {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

uint64_t vec3_mag2(Vec3 a)
{
    return (a.x * a.x + a.y * a.y + a.z * a.z) / ONE;
}

Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    Vec3 ret = {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };

    return ret;
}

Vec3 vec3_divide(Vec3 a, int s)
{
    Vec3 ret = {
        .x = fixed_div(a.x, s),
        .y = fixed_div(a.y, s),
        .z = fixed_div(a.z, s),
    };

    return ret;
}

Vec3 vec3_scale(Vec3 a, int s)
{
    Vec3 ret = {
        .x = fixed_mul(a.x, s),
        .y = fixed_mul(a.y, s),
        .z = fixed_mul(a.z, s),
    };

    return ret;
}


/*
 * PSn00bSDK (incomplete) trigonometry library
 * (C) 2019-2022 Lameguy64, spicyjpeg - MPL licensed
 *
 * Based on isin_S4 implementation from coranac:
 * https://www.coranac.com/2009/07/sines
 */

#define qN_l	10
#define qN_h	15
#define qA		12
#define B		19900
#define	C		3516

static inline int _isin(int qN, int x) {
	int c, x2, y;

	c  = x << (30 - qN);			// Semi-circle info into carry.
	x -= 1 << qN;					// sine -> cosine calc

	x <<= (31 - qN);				// Mask with PI
	x >>= (31 - qN);				// Note: SIGNED shift! (to qN)
	x  *= x;
	x >>= (2 * qN - 14);			// x=x^2 To Q14

	y = B - (x * C >> 14);			// B - x^2*C
	y = (1 << qA) - (x * y >> 16);	// A - x^2*(B-x^2*C)

	return (c >= 0) ? y : (-y);
}

int isin(int x) {
	return _isin(qN_l, x);
}

int icos(int x) {
	return _isin(qN_l, x + (1 << qN_l));
}

int hisin(int x) {
	return _isin(qN_h, x);
}

int hicos(int x) {
	return _isin(qN_h, x + (1 << qN_h));
}