#include "common.h"

typedef struct {
    int x;
    int y;
    int z;
} Vec3;

int isin(int x);
int icos(int x);
unsigned int rnext(void);
Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_divide(Vec3 a, int s);
Vec3 vec3_scale(Vec3 a, int s);