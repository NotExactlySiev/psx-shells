#include "common.h"
#include "gpu.h"
#include "math.h"

#define NLAYERS 16

// break them down into smaller chunks if they're closer, and frustum cull those
// one by one. the fucntion that draws a chunk should take in u,v
// TODO: separate X and Z sheer
// FIXME: l >= 7 breaks this
void draw_patch(PrimBuf *pb, Vec3 pos, uint l, int sheer, int spread,
    uint u0, uint v0, uint u1, uint v1)
{
    int len = (ONE/4) * (1 << l);
    // at first let's break down ALL of them
    if (l > 0) {
        Vec3 camera_xz = { camera.x, 0, camera.z };
        uint64_t distance2 = vec3_mag2(vec3_sub(pos, camera_xz));
        uint64_t threshhold = (ONE) * (1 << (l));
        uint64_t threshhold2 = threshhold * threshhold / ONE;
        
        if (distance2 < threshhold2) {
            // TODO: do the transformation of points shared between these four
            //       instances right here, and pass them down to avoid repeat
            // TODO: pass a phase modified shee and spread down to simulate wind
            draw_patch(pb, vec3_add(pos, (Vec3) { -len/2, 0, -len/2 }), l - 1, sheer, spread,
                u0,         v0,         (u0+u1)/2,  (v0+v1)/2);
            draw_patch(pb, vec3_add(pos, (Vec3) {  len/2, 0, -len/2 }), l - 1, sheer, spread,
                (u0+u1)/2,  v0,         u1,         (v0+v1)/2);
            draw_patch(pb, vec3_add(pos, (Vec3) { -len/2, 0,  len/2 }), l - 1, sheer, spread,
                u0,         (v0+v1)/2,  (u0+u1)/2,  v1);
            draw_patch(pb, vec3_add(pos, (Vec3) {  len/2, 0,  len/2 }), l - 1, sheer, spread,
                (u0+u1)/2,  (v0+v1)/2,  u1,         v1);
            return;
        }
    }

    if (!in_view(pos, NULL)) {
        return;
    }

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
        { ONE, -sheer, 0,
          0, ONE, 0,
          0, -sheer, ONE },
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
