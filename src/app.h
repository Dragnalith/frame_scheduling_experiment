#pragma once

#include "node_editor.h"
#include "visualizer.h"

struct App
{
    SimulationOption SimOption;
    SimulationOption LastSimOption;
    ControlOption ControlOption;
    DisplayOption DisplayOption;

    static App& get();
private:
    App() = default;
};