#include "common.h"
#include "gpu.h"
#include "math.h"
#include "grass.h"


#define NLAYERS 16

static void draw_quad(PrimBuf *pb, Vec3 verts[4], int layer, uint16_t tpage, uint16_t clut, uint u0, uint v0, uint u1, uint v1)
{
    uint32_t *prim = next_prim(pb, 9, layer);
    *prim++ = gp0_rgb(128, 128, 128) | gp0_quad(true, false);
    *prim++ = gp0_xy(verts[0].x, verts[0].y);
    *prim++ = gp0_uv(u0, v0, clut);    
    *prim++ = gp0_xy(verts[1].x, verts[1].y);
    *prim++ = gp0_uv(u1, v0, tpage);
    *prim++ = gp0_xy(verts[2].x, verts[2].y);
    *prim++ = gp0_uv(u0, v1, 0);
    *prim++ = gp0_xy(verts[3].x, verts[3].y);
    *prim++ = gp0_uv(u1, v1, 0);
}

static void draw_patch(PrimBuf *pb, Vec3 pos, uint len, int sheer, int spread,
    uint u0, uint v0, uint u1, uint v1)
{
    const Vec3 verts[4] = {
        { -len, 0, -len },
        {  len, 0, -len },
        { -len, 0,  len },
        {  len, 0,  len },
    };

    Vec3 quads[NLAYERS][4];
    for (int layer = 0; layer < NLAYERS; layer++) {
        Vec3 offset = { 0, -50 * layer, 0 };
        for (int i = 0; i < 4; i++) {
            quads[layer][i] = vec3_add(verts[i], offset);
            int s = ONE + layer * spread / ONE;
            quads[layer][i].x = quads[layer][i].x * s / ONE;
            quads[layer][i].z = quads[layer][i].z * s / ONE;
        }
    }

    Mat modelview = {
        {{ ONE, -sheer, 0 },
         { 0,   ONE,    0 },
         { 0,   -sheer, ONE }},
        { pos.x, pos.y, pos.z },
    };

    Mat matr = mat_multiply(projection, modelview);

    Vec3 final[NLAYERS][4];
    for (int layer = 0; layer < NLAYERS; layer++) {
        for (int i = 0; i < 4; i++) {
            // project
            Vec3 proj = vec3_multiply_matrix(quads[layer][i], &matr);
            // final step
            proj.x = ((SCREEN_H / 2) * proj.x) / proj.z;
            proj.y = ((SCREEN_W / 2) * proj.y) / proj.z;
            final[layer][i] = proj;
        }
    }
    
    for (int layer = 0; layer < NLAYERS; layer++) {
        draw_quad(pb, final[layer], 20 - layer,
            gp0_page(768 / 64, 0, GP0_BLEND_ADD, GP0_COLOR_4BPP),
            gp0_clut(768 / 16, 256 + layer),
            u0, v0, u1-1, v1-1
        );
    }
}

// TODO: separate X and Z sheer
// FIXME: l >= 7 breaks this, but not in -O3
static void _draw_grass(PrimBuf *pb, Vec3 pos, uint l, int sheer, int spread,
    uint u0, uint v0, uint u1, uint v1)
{
    int len = (ONE/4) * (1 << l);
    
    // these are only computed if l > 0 so we don't short circuit. a bit clunky
    Vec3 camera_xz = { camera.x, 0, camera.z };
    uint64_t distance2 = vec3_mag2(vec3_sub(pos, camera_xz));
    uint64_t threshhold = (ONE) * (1 << l);
    uint64_t threshhold2 = threshhold * threshhold / ONE;
    if (l > 0 && distance2 < threshhold2) {
        // subdivide
        // TODO: do the transformation of points shared between these four
        //       instances right here, and pass them down to avoid repeat
        // TODO: pass a phase modified shee and spread down to simulate wind
        _draw_grass(pb, vec3_add(pos, (Vec3) { -len/2, 0, -len/2 }), l - 1, sheer, spread,   u0,         v0,         (u0+u1)/2,  (v0+v1)/2);

        _draw_grass(pb, vec3_add(pos, (Vec3) {  len/2, 0, -len/2 }), l - 1, sheer, spread,   (u0+u1)/2,  v0,         u1,         (v0+v1)/2);

        _draw_grass(pb, vec3_add(pos, (Vec3) { -len/2, 0,  len/2 }), l - 1, sheer, spread,   u0,         (v0+v1)/2,  (u0+u1)/2,  v1);

        _draw_grass(pb, vec3_add(pos, (Vec3) {  len/2, 0,  len/2 }), l - 1, sheer, spread,   (u0+u1)/2,  (v0+v1)/2,  u1,         v1);
    } else if (in_view(pos, NULL)) {
        draw_patch(pb, pos, len, sheer, spread, u0, v0, u1, v1);
    }
}

// TODO: l >= 6 breaks this. but not if _draw_grass is called directly from main
void draw_grass(PrimBuf *pb, Vec3 pos, int sheer, int spread)
{
    int level = 5;
    int tex = 16 << level;
    _draw_grass(pb, pos, level, sheer, spread, 0, 0, tex, tex);
}