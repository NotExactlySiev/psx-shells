#include <stdint.h>

#include "common.h"
#include "gpucmd.h"


// FIXME: lto doesn't work so we're doing this for now :P
//#include "math.h"
//#include "clock.h"
//#include "gpu.h"
//#include "input.h"
#include "math.c"
#include "clock.c"
#include "gpu.c"
#include "input.c"

#define DEC(x)  (((((x) * 10)/8 * 10)/8 * 10)/ 8 * 10)/ 8

void fixed_print(int x)
{
    if (x >= 0)
        printf(" %d.%04d", x >> 12, DEC(x & (ONE - 1)));
    else
        printf("-%d.%04d", (-x) >> 12, DEC((-x) & (ONE - 1)));

    printf(" [0x%08X] ", x);
}

static void vec3_print(Vec3 a)
{
    fixed_print(a.x);
    fixed_print(a.y);
    fixed_print(a.z);
    printf("\n", 0);
}

typedef struct {
    int m[3][3];    // row, col
    int t[3];
} Mat;

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
            printf("\t", 0);
        }
        printf("\n", 0);
    }
    printf("\n", 0);
}

Vec3 vec3_multiply_matrix(Vec3 v, Mat *m)
{
    // TODO: make the translation part of the matrix a Vec3
    return (Vec3) {
        .x = m->t[0]
            + (v.x * m->m[0][0] + v.y * m->m[0][1] + v.z * m->m[0][2])/ONE,
        .y = m->t[1]
            + (v.x * m->m[1][0] + v.y * m->m[1][1] + v.z * m->m[1][2])/ONE,
        .z = m->t[2]
            + (v.x * m->m[2][0] + v.y * m->m[2][1] + v.z * m->m[2][2])/ONE,
    };
}

void set_rotation_matrix(Mat* m);
void set_translation_vector(Vec3* v);

void perspective_transform(Vec3* out, Vec3* vert);

#define VPRINT(v)   printf(#v "\t= ", (v)); vec3_print(v);
#define MPRINT(m)   printf(#m "\t=\n", (m)); mat_print(m); printf("\n", m);

void make_camera_matrix(Mat* m, Vec3* cam, Vec3* target)
{
    Vec3 dir = vec3_sub(*target, *cam);
    int R = sqrt((dir.x*dir.x + dir.y*dir.y) >> PREC);
    int a = (dir.y << 12) / R;
    int b = (dir.x << 12) / R;
    int c = (ONE << 12) / R;
    Mat m0 = {{
        { a, -b, 0 },
        { b, a, 0 },
        { 0, 0, ONE }
    }};

    Vec3 dir0;
    //vec3_multiply_matrix(&dir0, &dir, &m0);

    R = (((dir.x*dir.x + dir.y*dir.y + dir.z*dir.z) >> PREC));
    int D = sqrt(R);
    a = (dir0.z << 12) / D;
    b = (dir0.y << 12) / D;
    c = (ONE << 12) / D;
    Mat m1 = {{
        { ONE, 0, 0 },
        { 0, a, -b },
        { 0, b, a }
    }};
    //mat_multiply(m, &m1, &m0);
}

int vec3_dot(Vec3* a, Vec3* b)
{
    return (a->x*b->x + a->y*b->y + a->z*b->z) >> 12;
}

void draw_quad(PrimBuf *pb, Vec3 verts[4], int layer, uint16_t tpage, uint16_t clut, uint u0, uint v0, uint u1, uint v1)
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

void draw_line(PrimBuf *pb, Vec3 a, Vec3 b, uint32_t color)
{
    uint32_t *prim = next_prim(pb, 3, 1);
    prim[0] = color | gp0_line(false, false);
    prim[1] = gp0_xy(a.x, a.y);
    prim[2] = gp0_xy(b.x, b.y);
}

Vec3 camera = {0};
Mat projection;

int near = 2*ONE;
int far = 50*ONE;

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

// is a point in view when projected to camera space?
// can also output the projection to be reused
bool in_view(Vec3 point, Vec3 *projected)
{
    Vec3 proj = vec3_multiply_matrix(point, &projection);
    if (projected != NULL) {
        *projected = proj;
        projected->x = ((SCREEN_H / 2) * projected->x) / projected->z;
        projected->y = ((SCREEN_W / 2) * projected->y) / projected->z;
    }
    return (proj.z > near && proj.z < far)
        && (iabs(proj.x) <= proj.z + 0*proj.z/4)
        && (iabs(proj.y) <= proj.z + 0*proj.z/4);
}

void draw_axes(PrimBuf *pb)
{
    const Vec3 verts[4] = {
        { 0,    0,      0   },
        { ONE,  0,      0   },
        { 0,    ONE,    0   },
        { 0,    0,      ONE },
    };
    
    Vec3 proj[4];

    if (!in_view(verts[0], &proj[0]))
        return;
    
    for (int i = 1; i < 4; i++) {
        proj[i] = vec3_multiply_matrix(verts[i], &projection);
        proj[i].x = ((SCREEN_H / 2) * proj[i].x) / proj[i].z;
        proj[i].y = ((SCREEN_W / 2) * proj[i].y) / proj[i].z;
    }

    draw_line(pb, proj[0], proj[1], gp0_rgb(255, 0, 0));
    draw_line(pb, proj[0], proj[2], gp0_rgb(0, 255, 0));
    draw_line(pb, proj[0], proj[3], gp0_rgb(0, 0, 255));
}

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
        uint64_t threshhold = (ONE/2) * (1 << (l));
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
    
    // 0 is 1/4 units (32 * 32 texels)
    // each level is double
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
        /*draw_quad(pb, final[layer], 20 - layer,
            gp0_page(768 / 64, 0, GP0_BLEND_ADD, GP0_COLOR_4BPP),
            gp0_clut(768 / 16, 256 + layer)
        );*/
        draw_quad(pb, final[layer], 20 - layer,
            gp0_page(768 / 64, 0, GP0_BLEND_ADD, GP0_COLOR_4BPP),
            gp0_clut(768 / 16, 256 + layer),
            u0, v0, u1-1, v1-1
            //0, 0, 63, 63
        );
    }
}

extern volatile int frame;

int _start()
{
    init_pad();
    clock_init();

    GPU_GP1 = gp1_resetGPU();
	GPU_GP1 = gp1_dmaRequestMode(GP1_DREQ_GP0_WRITE);
    GPU_GP1 = gp1_dispBlank(false);
    GPU_GP1 = gp1_fbMode(GP1_HRES_256, GP1_VRES_256, GP1_MODE_NTSC, false, GP1_COLOR_16BPP);
    GPU_GP0 = gp0_fbMask(false, false);
    GPU_GP0 = gp0_fbOffset1(0, 0);
    GPU_GP0 = gp0_fbOffset2(1023, 511);
    GPU_GP0 = gp0_fbOrigin(SCREEN_W / 2, SCREEN_H / 2);

    inline uint16_t green(uint8_t val) {
        return (val & 0x1F) << 5;
    }

    GPU_GP0 = gp0_vramWrite();
    GPU_GP0 = gp0_xy(768, 256);
    GPU_GP0 = gp0_xy(16, 16);
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16/2; col++) {
            if (col*2 < row) {
                GPU_GP0 = 0;
            } else {
                uint16_t col = green(row * 2 + 1);
                GPU_GP0 = col | (col << 16);
            }
        }
    }

    GPU_GP0 = gp0_vramWrite();
    GPU_GP0 = gp0_xy(768, 0);
    GPU_GP0 = gp0_xy(128, 256);

    for (int i = 0; i < 256*128/2; i++) {
        GPU_GP0 = (rnext());
    }

    gpu_sync();
    
    DMA_DPCR |= DMA_DPCR_ENABLE << (DMA_GPU * 4);

    PrimBuf *pb = gpu_init();

    int angle_x = ONE/25;
    int angle_y = 0;
    uint16_t pad = 0;
    Vec3 pos = { 0, 0, 0 };
    
    camera.y = -2.5 * ONE;
    camera.z = 0 * ONE;
    int last_frame = frame;
    uint t = 0;
    for (;;) {
        int delta = frame - last_frame;
        last_frame = frame;
        t += delta;
        printf("\nFRAME %05d (+%d): ", t, delta);
        uint32_t *prim;

        uint16_t pad_new = read_pad();
        uint16_t pad_edge = pad ^ pad_new;
        uint16_t pressed = pad_new & pad_edge;
        pad = pad_new;

        if (pad & PAD_CROSS) {
            const int move_increment = ONE/2;
            // move patch around
            if (pressed & PAD_UP) {
                pos.z += move_increment;
            } else if (pressed & PAD_DOWN) {
                pos.z -= move_increment;
            }
            
            if (pressed & PAD_LEFT) {
                pos.x -= move_increment;
            } else if (pressed & PAD_RIGHT) {
                pos.x += move_increment;
            }
        } else if (pad & PAD_CIRCLE) {
            const int move_increment = ONE/8;
            if (pressed & PAD_UP) {
                camera.z += move_increment;
            } else if (pressed & PAD_DOWN) {
                camera.z -= move_increment;
            }
            
            if (pressed & PAD_LEFT) {
                camera.x -= move_increment;
            } else if (pressed & PAD_RIGHT) {
                camera.x += move_increment;
            }
        } else {
            int amount = 10 * delta;

            // camera controls
            if (pad & PAD_UP) {
                angle_x -= amount;
            } else if (pad & PAD_DOWN) {
                angle_x += amount;
            }

            if (angle_x < 0) angle_x = 0;
            if (angle_x > ONE/4) angle_x = ONE/4;
            
            if (pad & PAD_LEFT) {
                angle_y += amount;
            } else if (pad & PAD_RIGHT) {
                angle_y -= amount;
            }
        }


        Mat cam_trans = {
            { ONE,    0,      0,
              0,      ONE,    0,
              0,      0,      ONE },
            { -camera.x, -camera.y, -camera.z }
        };
        
        int sint = isin(angle_x);
        int cost = icos(angle_x);
        Mat cam_rotx = {
            { ONE,    0,          0,
              0,      cost,     -sint,
              0,      sint,    cost, },
            { 0, 0, 0 }
        };

        sint = isin(angle_y);
        cost = icos(angle_y);
        // this is part of the projection matrix
        Mat cam_roty = {
            { cost,       0,      sint,
              0,          ONE,    0,
              -sint,      0,      cost },
            { 0, 0, 0 }
        };

        projection = mat_multiply(cam_rotx,
                     mat_multiply(cam_roty, 
                                  cam_trans));
        
        // drawing
        clock_begin();
        int tex = 1024;
        draw_patch(pb, pos, 6, icos(t * 26) / 8, icos(t * 17) * 7, 0, 0, tex, tex);
                
        if (~pad & 1)
            draw_axes(pb);
        clock_print(clock_end());

        pb = swap_buffer();
    }
}
