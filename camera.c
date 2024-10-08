#include "common.h"
#include "math.h"
#include "camera.h"

static int right    = ONE;
static int bottom   = ONE;
static int near     = 100;
static int far      = 10*ONE;
Vec3 camera = {0};
Mat projection;

// is a point in view when projected to camera space?
bool in_view(Vec3 point, Vec3 *projected)
{
    // TODO: this is using hardcoded values. not good
    Vec3 proj = vec3_multiply_matrix_soft(point, &projection);
    return (proj.z > near || proj.z < far)
        && (iabs(proj.x) < proj.z)
        && (iabs(proj.y) < proj.z);
}

void camera_set(Vec3 pos, int angle_x, int angle_y)
{
    Mat cam_trans = {
        {{{ ONE,    0,      0 },
        {  0,      ONE,    0 },
        {  0,      0,      ONE }}},
        .t = vec3_sub(VEC3_ZERO, pos)
    };
    // TODO: use a better sincos() function
    int sint = isin(angle_x);
    int cost = icos(angle_x);
    Mat cam_rotx = {
        {{{ ONE,    0,          0 },
         { 0,      cost,     -sint },
         { 0,      sint,    cost, }}},
        { 0, 0, 0 }
    };

    sint = isin(angle_y);
    cost = icos(angle_y);
    // this is part of the projection matrix
    Mat cam_roty = {
        {{{ cost,       0,      sint },
        {  0,          ONE,    0 },
        {  -sint,      0,      cost }}},
        { 0, 0, 0 }
    };

    // to normalized screen space coordinates
    Mat cam_screen = {
        {{{ fixed_div(ONE, right),       0,      0 },
        {  0,          fixed_div(ONE, bottom),    0 },
        {  0,      0,            ONE }}},
        { 0, 0, 0 }
    };

    projection = mat_multiply(cam_screen,
                 mat_multiply(cam_rotx,
                 mat_multiply(cam_roty,
                              cam_trans)));
}