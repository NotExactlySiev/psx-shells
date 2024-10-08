#include "common.h"
#include "gpucmd.h"
#include "cop0gte.h"

#include "math.h"
#include "clock.h"
#include "gpu.h"
#include "input.h"
#include "grass.h"
#include "camera.h"

#define DEC(x)  (((((x) * 10)/8 * 10)/8 * 10)/ 8 * 10)/ 8

void fixed_print(int x)
{
    if (x >= 0)
        k_printf(" %d.%04d", x >> 12, DEC(x & (ONE - 1)));
    else
        k_printf("-%d.%04d", (-x) >> 12, DEC((-x) & (ONE - 1)));

    k_printf(" [0x%08X] ", x);
}

void draw_line(PrimBuf *pb, Vec3 a, Vec3 b, uint32_t color)
{
    uint32_t *prim = next_prim(pb, 3, 1);
    prim[0] = color | gp0_line(false, false);
    prim[1] = gp0_xy(a.x, a.y);
    prim[2] = gp0_xy(b.x, b.y);
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

    transform(proj, verts, 4, &projection);
    draw_line(pb, proj[0], proj[1], gp0_rgb(255, 0, 0));
    draw_line(pb, proj[0], proj[2], gp0_rgb(0, 255, 0));
    draw_line(pb, proj[0], proj[3], gp0_rgb(0, 0, 255));
    
}

extern volatile int frame;

static void gte_init(void) {
    cop0_setSR(cop0_getSR() | COP0_SR_CU2);
    gte_setFieldOfView(SCREEN_W/2);
}

int _start()
{
    init_pad();
    clock_init();
    gte_init();

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
    
    camera.y = -0.5 * ONE;
    camera.z = -0 * ONE;
    int last_frame = frame;
    uint t = 0;
    for (;;) {
        int delta = frame - last_frame;
        last_frame = frame;
        t += delta;
        k_printf("\nFRAME %05d (+%d): ", t, delta);
        //uint32_t *prim;

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

        camera_set(camera, angle_x, angle_y);

        // drawing
        clock_begin();
        draw_grass(pb, pos, icos(t * 26) / 16, icos(t * 17) * 3);

        if (~pad & 1)
            draw_axes(pb);
        clock_print(clock_end());

        pb = swap_buffer();
    }
}