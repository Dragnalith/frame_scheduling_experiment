#pragma once

#include <unordered_map>
#include <unordered_set>

#include "node_editor.h"
#include "visualizer.h"


#include "NodeEditor.h"

namespace ed = ax::NodeEditor;

struct App
{
    std::shared_ptr<FramePattern> Pattern;

    SimulationOption SimOption;
    SimulationOption LastSimOption;
    ControlOption ControlOption;
    DisplayOption DisplayOption;

    ax::NodeEditor::EditorContext* NodeEditorContext = nullptr;



    std::shared_ptr<Simulator> CurrentSimulation;

    std::unordered_set<std::shared_ptr<Simulator>> FrozenSimulations;

    static void init();
    static App& get();
private:
    App() = default;

    static App* s_App;
};