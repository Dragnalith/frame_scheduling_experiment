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
    ControlOption ControlOption;
    DisplayOption DisplayOption;

    ax::NodeEditor::EditorContext* NodeEditorContext = nullptr;

    bool OnlyFramePattern = false;
    int SelectedPreset = 0;

    std::shared_ptr<Simulator> CurrentSimulation;

    std::unordered_set<std::shared_ptr<Simulator>> FrozenSimulations;

    static void set_preset(const Preset& p);

    static void init();
    static App& get();
private:
    App() = default;

    static App* s_App;
};

std::shared_ptr<JobType> create_job_type(const char* name, float duration,
    bool is_first = false, bool generate_next = false, bool release_frame = false);