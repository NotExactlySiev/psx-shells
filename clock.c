#include "common.h"
#include "registers.h"
#include "gpucmd.h"

#include "clock.h"

// TODO: add automatic checking of whether or not the returned numbers
//       make sense. clock should be ~400 times larger than hblank.

void clock_print(Time t)
{
    //int ratio = fixed_div(t.clock, t.hblank);
    printf("\t%d : %d", t.hblank,  t.clock);
}

void clock_init(void)
{
    TIMER_CTRL(0) = TIMER_CTRL_EXT_CLOCK;
    TIMER_CTRL(1) = TIMER_CTRL_RELOAD
                  | TIMER_CTRL_EXT_CLOCK;
    (void) TIMER_CTRL(0);
    (void) TIMER_CTRL(1);
}

void clock_begin(void)
{
    TIMER_VALUE(1) = 0;
    TIMER_VALUE(0) = 0;
}

Time clock_sub(Time a, Time b)
{
    return (Time) {
        .clock = a.clock - b.clock,
        .hblank = a.hblank - b.hblank,
    };
}

Time clock_end(void)
{
    return (Time) {
        .clock = TIMER_VALUE(0),
        .hblank = TIMER_VALUE(1),
    };
}