#include "math.h"
#include "cop0gte.h"

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

int vec3_dot(Vec3 a, Vec3 b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z) / ONE;
}

Vec3 vec3_multiply_matrix(Vec3 v, Mat *m)
{
#ifdef NO_GTE
    // TODO: make the translation part of the matrix a Vec3
    return (Vec3) {
        .x = m->t[0]
            + (v.x * m->m[0][0] + v.y * m->m[0][1] + v.z * m->m[0][2])/ONE,
        .y = m->t[1]
            + (v.x * m->m[1][0] + v.y * m->m[1][1] + v.z * m->m[1][2])/ONE,
        .z = m->t[2]
            + (v.x * m->m[2][0] + v.y * m->m[2][1] + v.z * m->m[2][2])/ONE,
    };
#else
    gte_setV0(v.x, v.y, v.z);
    gte_command(GTE_CMD_MVMVA | GTE_SF | GTE_MX_RT | GTE_V_V0 | GTE_CV_TR);
    return (Vec3) {
        gte_getIR1(),
        gte_getIR2(),
        gte_getIR3(),
    };
#endif
}

void vec3_print(Vec3 a)
{
    fixed_print(a.x);
    fixed_print(a.y);
    fixed_print(a.z);
    k_printf("\n", 0);
}

Mat mat_multiply(Mat a, Mat b)
{
    // B's tran has to rotate by a
    // ret = AB
    Mat ret;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
        ret.m[i][j] = (a.m[i][0] * b.m[0][j]
                     + a.m[i][1] * b.m[1][j]
                     + a.m[i][2] * b.m[2][j]) / ONE;
        ret.t[0] = (b.t[0] * a.m[0][0] + b.t[1] * a.m[0][1] + b.t[2] * a.m[0][2])/ONE + a.t[0];
        ret.t[1] = (b.t[0] * a.m[1][0] + b.t[1] * a.m[1][1] + b.t[2] * a.m[1][2])/ONE + a.t[1];
        ret.t[2] = (b.t[0] * a.m[2][0] + b.t[1] * a.m[2][1] + b.t[2] * a.m[2][2])/ONE + a.t[2];
    }
    return ret;
}

void mat_print(Mat* m)
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            fixed_print(m->m[i][j]);
            k_printf("\t", 0);
        }
        k_printf("\n", 0);
    }
    k_printf("\n", 0);
}

// TODO: make this branchless
int iabs(int x)
{
    return x < 0 ? -x : x;
    asm("sra %0,%1,31\n"
        "xor %1, %0\n"
        "sub %0, %1, %0\n"
        : "=r"(x)
        : "r"(x)
    );
    return x;
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
	int c, y;

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