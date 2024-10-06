#include <stdint.h>

typedef struct {
    uint16_t clock;
    uint16_t hblank;
} Time;

void clock_init(void);
void clock_begin(void);
Time clock_end(void);
void clock_print(Time t);

Time clock_sub(Time a, Time b);