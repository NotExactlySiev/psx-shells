#pragma once
#include "common.h"

typedef struct {
    int x;
    int y;
    int z;
} Vec3;

typedef struct {
    int m[3][3];    // row, col
    int t[3];
} Mat;

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
void vec3_print(Vec3 a);
Mat mat_multiply(Mat a, Mat b);
void mat_print(Mat* m);
Vec3 vec3_multiply_matrix(Vec3 v, Mat *m);