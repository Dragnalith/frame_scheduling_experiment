#include <stdint.h>

#include <string>
#include <sstream>
#include <deque>
#include <vector>

struct TimeBox
{
    TimeBox(int index, int frame, float start, float end, const std::string& n, uint32_t c = 0xffffffff)
        : core_index(index)
        , start_time(start)
        , end_time(end)
        , name(n)
        , color(c)
    {
        std::stringstream s;
        s << frame << "-";
        frame_index = s.str();
    }

    int core_index;
    std::string frame_index;
    float start_time;
    float end_time;
    std::string name;
    uint32_t color;
};

void DrawVisualizer();