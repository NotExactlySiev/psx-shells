#include <stdint.h>

#include "registers.h"
#include "gpucmd.h"

#include "gpu.h"

#define PRIM_NLAYERS    32

struct PrimBuf {
    uint32_t data[1 << 17];
    uint32_t layers[PRIM_NLAYERS + 1];
    uint32_t *next;
};

static PrimBuf primbufs[2];
static int curr = 0;

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
    
    while (!(IRQ_STAT & (1 << IRQ_VSYNC)));
    IRQ_STAT = 0;

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