#include <stdint.h>
#include "cop0gte.h"

#include "registers.h"
#include "gpucmd.h"
#include "math.h"

#include "gpu.h"

#define PRIM_NLAYERS    32

struct PrimBuf {
    uint32_t data[1 << 17];
    uint32_t layers[PRIM_NLAYERS + 1];
    uint32_t *next;
};

static PrimBuf primbufs[2];
static int curr = 0;
static int vsync_event = 0;

// TODO: move kernel stuff to another header
#define RCntCNT3    0xF2000003
#define EvSpINT     0x0002
#define EvMdINTR    0x1000

int  EnterCriticalSection(void);
void ExitCriticalSection(void);
int OpenEvent(uint,int,int,int (*func)());
int CloseEvent(int);
int WaitEvent(int);
int TestEvent(int);
int EnableEvent(int);
int DisableEvent(int);
int enable_timer_irq(uint);
int disable_timer_irq(uint);

int enable_vblank_event(void* handler)
{
    int event;
    EnterCriticalSection();
    event = OpenEvent(RCntCNT3, EvSpINT, EvMdINTR, handler);    
    enable_timer_irq(3);
    EnableEvent(event);
    ExitCriticalSection();
    return event;
}

void disable_vblank_event(int event)
{
    EnterCriticalSection();
    disable_timer_irq(3);
    CloseEvent(event);
    ExitCriticalSection();
}

volatile int frame = 0;
void vsync_handler(void)
{
    frame++;
}

static void clear_buffer(PrimBuf *pb)
{
    pb->next = &pb->data[0];
    for (int i = 1; i < PRIM_NLAYERS + 1; i++) {
        pb->layers[i] = gp0_tag(0, &pb->layers[i-1]);
    }
    pb->layers[0] = gp0_endTag(0);
}

static void send_prims(uint32_t *data)
{
	while (DMA_CHCR(DMA_GPU) & DMA_CHCR_ENABLE);
	DMA_MADR(DMA_GPU) = (uint32_t) data;
	DMA_CHCR(DMA_GPU) = DMA_CHCR_WRITE | DMA_CHCR_MODE_LIST | DMA_CHCR_ENABLE;
}

// TODO: bring all the initialization stuff here from main
PrimBuf *gpu_init(void)
{
    curr = 0;
    clear_buffer(&primbufs[curr]);
    vsync_event = enable_vblank_event(vsync_handler);
    frame = 0;

    return &primbufs[curr];
}

// TODO: rename to gpu_alloc?
uint32_t *next_prim(PrimBuf *pb, int len, int layer)
{
    uint32_t *prim = pb->next;
    prim[0] = gp0_tag(len, (void*) pb->layers[layer]);
    pb->layers[layer] = gp0_tag(0, prim);
    pb->next += len + 1;
    return &prim[1];
}

// TODO: rename to gpu_swap
PrimBuf *swap_buffer(void)
{
    static int last_rendered = 0;

    uint32_t *prim;
    PrimBuf *pb = &primbufs[curr];
    
    int bufferX = curr * SCREEN_W;
    int bufferY = 0;
    prim = next_prim(pb, 4, PRIM_NLAYERS);
    prim[0] = gp0_texpage(0, true, false);
	prim[1] = gp0_fbOffset1(bufferX, bufferY);
	prim[2] = gp0_fbOffset2(bufferX + SCREEN_W - 1, bufferY + SCREEN_H - 2);
	prim[3] = gp0_fbOrigin(bufferX + SCREEN_W / 2, bufferY + SCREEN_H / 2);

    gpu_sync();    
    while (last_rendered == frame);
    last_rendered = frame;

    send_prims(&primbufs[curr].layers[PRIM_NLAYERS]);
    curr = 1 - curr;
    pb = &primbufs[curr];
    GPU_GP1 = gp1_fbOffset(curr * SCREEN_W, 0);
    
    clear_buffer(&primbufs[curr]);
    prim = next_prim(pb, 3, 30);
    prim[0] = gp0_rgb(10, 70, 250) | gp0_vramFill();
    prim[1] = gp0_xy((curr) * SCREEN_W, 0);
    prim[2] = gp0_xy(SCREEN_W, SCREEN_H);

    return &primbufs[curr];
}

void gpu_sync(void)
{
    while (!(GPU_GP1 & GP1_STAT_CMD_READY));
}

// perspective stuff. should be in a camera.c?

Vec3 camera = {0};
Mat projection;
int near = ONE/16;
int far = 10*ONE;

// is a point in view when projected to camera space?
bool in_view(Vec3 point, Vec3 *projected)
{
    Vec3 proj;
    vec3_multiply_matrix(&proj, &point, 1, &projection);
    if (proj.z < near || proj.z > far) return false;
    if (projected != NULL) {
        *projected = proj;
        projected->x = ((SCREEN_H / 2) * projected->x) / projected->z;
        projected->y = ((SCREEN_W / 2) * projected->y) / projected->z;
    }
    return (iabs(proj.x) <= proj.z + 0*proj.z/5)
        && (iabs(proj.y) <= proj.z + 0*proj.z/8);
}