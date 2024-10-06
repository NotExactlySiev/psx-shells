#include "common.h"

typedef enum {
    PAD_UP      =   1 << 4,
    PAD_RIGHT   =   1 << 5,
    PAD_DOWN    =   1 << 6,
    PAD_LEFT    =   1 << 7,

    PAD_TRI     =   1 << 12,
    PAD_CIRCLE  =   1 << 13,
    PAD_CROSS   =   1 << 14,
    PAD_SQUARE  =   1 << 15,
} Button;

void init_pad(void);
uint16_t read_pad(void);
