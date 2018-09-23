#pragma once

#include <unordered_map>

#include "node_editor.h"
#include "visualizer.h"

struct App
{
    std::unordered_map<uint32_t, std::shared_ptr<JobType>> Types;
    std::unordered_map<uint32_t, uint32_t> Id2Node;

    SimulationOption SimOption;
    SimulationOption LastSimOption;
    ControlOption ControlOption;
    DisplayOption DisplayOption;


    FramePattern Pattern;

    static void init();
    static App& get();
private:
    App() = default;

    static App* s_App;
};