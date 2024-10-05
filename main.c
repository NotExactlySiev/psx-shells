#include <stdint.h>

#include "registers.h"
#include "gpucmd.h"

#define PREC    12
#define ONE     (1 << PREC)

static int fixed_div(int a, int b)
{
    return (a * ONE) / b;
}

static int fixed_mul(int a, int b)
{
    return a * b / ONE;
}

typedef unsigned short  ushort;
typedef unsigned int    uint;

unsigned int _seed = 2891583007UL;

int rand_r(unsigned int *seed) {
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
    return rand_r(&_seed);
}

typedef struct {
    int x;
    int y;
    int z;
} Vec3;

Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return (Vec3) {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
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

void printf(char* fmt, ...);

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

void draw_quad(Vec3 verts[4], uint16_t tpage, uint16_t clut)
{
    uint u = 64;
    uint v = 64;
    GPU_GP0 = gp0_rgb(128, 128, 128) | gp0_quad(true, false);
    GPU_GP0 = gp0_xy(verts[0].x, verts[0].y);
    GPU_GP0 = gp0_uv(0, 0, clut);
            
    GPU_GP0 = gp0_xy(verts[1].x, verts[1].y);
    GPU_GP0 = gp0_uv(u, 0, tpage);
            
    GPU_GP0 = gp0_xy(verts[2].x, verts[2].y);
    GPU_GP0 = gp0_uv(0, v, 0);
            
    GPU_GP0 = gp0_xy(verts[3].x, verts[3].y);
    GPU_GP0 = gp0_uv(u, v, 0);
}

#define NLAYERS 16

void draw_line(Vec3 a, Vec3 b, uint32_t color)
{
    GPU_GP0 = color | gp0_line(false, false);
    GPU_GP0 = gp0_xy(a.x, a.y);
    GPU_GP0 = gp0_xy(b.x, b.y);
}

Mat projection;

void draw_axes(void)
{
    const Vec3 verts[4] = {
        { 0,    0,      0   },
        { ONE,  0,      0   },
        { 0,    ONE,    0   },
        { 0,    0,      ONE },
    };

    Vec3 proj[4];

    for (int i = 0; i < 4; i++) {
        proj[i] = vec3_multiply_matrix(verts[i], &projection);
        proj[i].x = (256 * proj[i].x) / proj[i].z;
        proj[i].y = (320 * proj[i].y) / proj[i].z;
    }

    draw_line(proj[0], proj[1], gp0_rgb(255, 0, 0));
    draw_line(proj[0], proj[2], gp0_rgb(0, 255, 0));
    draw_line(proj[0], proj[3], gp0_rgb(0, 0, 255));
}

// TODO: separate X and Z sheer
void draw_patch(Vec3 pos, int sheer, int spread)
{
    const Vec3 verts[4] = {
        { -ONE, 0,  ONE },
        {  ONE, 0,  ONE },
        { -ONE, 0, -ONE },
        {  ONE, 0, -ONE },
    };
    // TODO: this part might not have to be repeated for every patch
    //Vec3 warped[NLAYERS][4];
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

    Vec3 final[NLAYERS][4];
    for (int layer = 0; layer < NLAYERS; layer++) {
        for (int i = 0; i < 4; i++) {
            // project
            Vec3 quad = vec3_multiply_matrix(quads[layer][i], &modelview);
            Vec3 proj = vec3_multiply_matrix(quad, &projection);
            
            // final step
            proj.x = (256 * proj.x) / proj.z;
            proj.y = (320 * proj.y) / proj.z;
            final[layer][i] = proj;
        }
    }
    
    for (int layer = 0; layer < NLAYERS; layer++) {
        draw_quad(final[layer], 
            gp0_page(768 / 64, 0, GP0_BLEND_ADD, GP0_COLOR_4BPP),
            gp0_clut(768 / 16, 256 + layer)
        );
    }
}

void init_pad()
{
    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE;
    SIO_MODE(0) = SIO_MODE_BAUD_DIV1 | SIO_MODE_DATA_8;
}

uint32_t read_pad()
{
    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE | SIO_CTRL_DTR | SIO_CTRL_DSR_IRQ_ENABLE;
    for (int i = 0; i < 300; i++)
        asm volatile("nop");

    const uint8_t msg[5] = { 0x01, 0x42, 0x00, 0x00, 0x00 };
    uint8_t resp[5] = {0};
    for (int i = 0; i < 5; i++) {
        SIO_DATA(0) = msg[i];
        while ((SIO_STAT(0) & SIO_STAT_RX_NOT_EMPTY) == 0);
        resp[i] = SIO_DATA(0);
    }

    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE | SIO_CTRL_ACKNOWLEDGE;
    return *(uint32_t*)&resp[1];
}

int _start()
{
    unsigned int r;
    int i, j;
    
    r = 432143;

    GPU_GP1 = gp1_resetGPU();
    //InitJoy();
    init_pad();

    // enable display
    GPU_GP1 = gp1_dispBlank(false);
    GPU_GP1 = gp1_fbMode(GP1_HRES_640, GP1_VRES_512, GP1_MODE_NTSC, true, GP1_COLOR_16BPP);
    GPU_GP0 = gp0_fbMask(false, false);
    GPU_GP0 = gp0_fbOffset1(0, 0);
    GPU_GP0 = gp0_fbOffset2(1023, 511);
    GPU_GP0 = gp0_fbOrigin(320, 256);

    GPU_GP0 = gp0_vramWrite();
    GPU_GP0 = gp0_xy(768, 0);
    GPU_GP0 = gp0_xy(128, 256);

    for (int i = 0; i < 256*128/2; i++) {
        GPU_GP0 = (rnext());
    }

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

    GPU_GP0 = gp0_fbOffset1(0, 0);
    GPU_GP0 = gp0_fbOffset2(639, 511);

    uint t = 0;
    int angle_x = ONE/16;
    int angle_y = 0;
    for (;;) {
        uint32_t idle = 0;
        // wait for vblank interrupt
        while (!(IRQ_STAT & IRQ_GPU))
            idle += 1;
        IRQ_STAT = 0;
        printf("IDLE: %d\n", idle);
        t += 1;

        //read_pad();
        //printf("%X\n", ReadJoy());
        printf("%X\n", read_pad());

        //int angle = (ONE / 4) / 5;
        //angle_x = (ONE + isin(2 * t)) / 45 + ONE/64;
        int sint = isin(angle_x);
        int cost = icos(angle_x);
        Mat matr = {
            { ONE,    0,          0,
              0,      cost,     -sint,
              0,      sint,    cost, },
            { 0, 0, 0 }
        };

        angle_y = t;
        sint = isin(angle_y);
        cost = icos(angle_y);

        // this is part of the projection matrix
        Mat matr0 = {
            { cost,       0,      sint,
              0,          ONE,    0,
              -sint,      0,      cost },
            { 0, 0, 0 }
        };

        int ar = fixed_div(640 * ONE, 512 * ONE);

        Mat zstuff = {
            { ONE,    0,      0,
              0,      ONE,    0,
              0,      0,      0.4 * ONE },
            { 0, 0, 2.5 * ONE }
        };

        projection = mat_multiply(mat_multiply(zstuff, matr), matr0);

        GPU_GP0 = gp0_rgb(64, 64, 64) | gp0_vramFill();
        GPU_GP0 = gp0_xy(0, 0);
        GPU_GP0 = gp0_xy(640, 511);


        //draw_patch((Vec3) { -1.96*ONE, 0, 0 }, icos(t * 27) / 7, isin(t * 19) * 8);
        //draw_patch((Vec3) {  1.96*ONE, 0, 0 }, isin(t * 28) / 7, isin(t * 19) * 8);
        draw_patch((Vec3) {    0, 0, 0 }, icos(t * 26) / 8, icos(t * 17) * 7);
        //draw_patch(0, (Vec3) { -1 * ONE, 0,  1 * ONE });
        //draw_patch(0, (Vec3) {  1 * ONE, 0, -1 * ONE });
        //draw_patch(0, (Vec3) { -1 * ONE, 0, -1 * ONE });
        draw_axes();
    }
}
