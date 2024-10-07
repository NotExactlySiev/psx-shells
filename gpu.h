#include "common.h"

#define SCREEN_W    256
#define SCREEN_H    240

typedef struct PrimBuf PrimBuf;

extern Vec3 camera;
extern int near;
extern int far;
extern Mat projection;

PrimBuf *gpu_init(void);
uint32_t *next_prim(PrimBuf *pb, int len, int layer);
PrimBuf *swap_buffer(void);
void gpu_sync(void);

bool in_view(Vec3 point, Vec3 *projected);