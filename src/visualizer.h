#include <stdint.h>

struct TimeBox
{
    TimeBox(int index, float start, float end, uint32_t c = 0xffffffff)
        : core_index(index)
        , start_time(start)
        , end_time(end)
        , color(c)
    {}

    int core_index;
    float start_time;
    float end_time;
    uint32_t color;
};

void DrawVisualizer();