#pragma once
#include "common.h"
#include "cop0gte.h"

typedef GTEVector16 Vec3;
typedef struct {
    GTEMatrix m;    // row, col
    Vec3 t;
} Mat;

#define VEC3_ZERO   (Vec3) { 0, 0, 0 }
#define VEC3_I      (Vec3) { ONE, 0, 0 }
#define VEC3_J      (Vec3) { 0, ONE, 0 }
#define VEC3_K      (Vec3) { 0, 0, ONE }

void fixed_print(int x);

int isin(int x);
int icos(int x);
int iabs(int x);
unsigned int rnext(void);
Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_divide(Vec3 a, int s);
Vec3 vec3_scale(Vec3 a, int s);
int vec3_dot(Vec3 a, Vec3 b);
uint64_t vec3_mag2(Vec3 a);
void vec3_print(Vec3 a);
Mat mat_multiply(Mat a, Mat b);
void mat_print(Mat* m);
void vec3_multiply_matrix(Vec3 *out, Vec3 *in, uint n, Mat *m);
Vec3 vec3_multiply_matrix_soft(Vec3 v, Mat *m);
void transform(Vec3 *out, Vec3 *in, uint n, Mat *m);

static void gte_load_matrix(Mat *m)
{
    gte_loadRotationMatrix(&m->m);
    gte_setTranslationVector(m->t.x, m->t.y, m->t.z);
}

#define VPRINT(v)   k_printf(#v "\t= ", (v)); vec3_print(v);
#define MPRINT(m)   k_printf(#m "\t=\n", (m)); mat_print(m); k_printf("\n", m);