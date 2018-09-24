#pragma once

#include <unordered_map>
#include <unordered_set>

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

    std::shared_ptr<Simulator> CurrentSimulation;

    std::unordered_set<std::shared_ptr<Simulator>> FrozenSimulations;

    static void init();
    static App& get();
private:
    App() = default;

    static App* s_App;
};