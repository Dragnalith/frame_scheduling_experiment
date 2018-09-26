// Creating a node graph editor for ImGui
// Quick demo, not production code! This is more of a demo of how to use ImGui to create custom stuff.
// Better version by @daniel_collin here https://gist.github.com/emoon/b8ff4b4ce4f1b43e79f2
// See https://github.com/ocornut/imgui/issues/306
// v0.03: fixed grid offset issue, inverted sign of 'scrolling'
// Animated gif: https://cloud.githubusercontent.com/assets/8225057/9472357/c0263c04-4b4c-11e5-9fdf-2cd4f33f6582.gif

#include <math.h> // fmodf

#include "imgui.h"
#include "imgui_internal.h"

#include "NodeEditor.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "node_editor.h"
#include "app.h"

namespace ed = ax::NodeEditor;

// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
//static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
//static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

// Really dumb data structure provided for the example.
// Note that we storing links are INDICES (not ID) to make example code shorter, obviously a bad idea for any general purpose code.

uint32_t g_Id = 100;

static uint32_t allocated_id()
{
    g_Id += 8;
    return g_Id;
}

JobType::JobType()
{
    nid = allocated_id();
    iid = allocated_id();
    oid = allocated_id();
    lid = allocated_id();
    strncpy(name, "No Name", 250);
}

FrameStage::FrameStage(const char* name, int s_tag, float w, int split, bool sync, int w_tag)
{
    strncpy(this->name, name, 100);
    stage_tag = s_tag;
    weight = w;
    split_count = split;
    wait = sync;
    wait_tag = w_tag;
}


ID::ID()
{
    node = allocated_id();
    link = allocated_id();
    in = allocated_id();
    out = allocated_id();
}

FrameFlow::FrameFlow(const char* n) {
    strncpy(name, n, 200);

    stages.push_back(std::make_shared<FrameStage>("Simulate Game1", 0, 1.f, 1, false, 0));
    stages.push_back(std::make_shared<FrameStage>("Simulate Game2", 0, 1.f, 1, false, 0));
    stages.push_back(std::make_shared<FrameStage>("Simulate Game3", 0, 1.f, 1, false, 0));
    stages.push_back(std::make_shared<FrameStage>("Prepare Render1", 1, 1.f, 1, false, 1));
    stages.push_back(std::make_shared<FrameStage>("Prepare Render2", 1, 1.f, 1, false, 1));
    stages.push_back(std::make_shared<FrameStage>("Prepare Render3", 1, 1.f, 1, false, 1));

    this->start_next_frame_stage = stages.size() - 1;
}

struct JobType;

void DrawFrameEditor(std::shared_ptr<FrameFlow> frame_flow)
{
    ImGui::SetNextWindowPos(ImVec2(300, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1600, 600), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Frame Editor");

    ImGui::DragFloat("Duration", &frame_flow->duration, 1.f, 10.f, 2000.f);
    ImGui::InputText("Name", frame_flow->name, 200);

    ed::Begin("Frame Editor", ImVec2(0, 0));

    auto pos = ImVec2(0.f, 0.f);
    const float offset = 230.f;

    for (int i = 0; i < frame_flow->stages.size(); i++) 
    {
        FrameStage& stage = *frame_flow->stages[i];

        ed::SetNodePosition(stage.id.node, pos);

        ImGui::PushID(stage.id.node);
        ed::BeginNode(stage.id.node);

        ImGui::PushItemWidth(130.f);
        ImGui::InputText("Name", stage.name, 101);
        ImGui::DragFloat("Weight", &stage.weight, 0.1f, 0.1f, 10.f);
        ImGui::InputInt("Split", &stage.split_count, 1, 1);
        stage.split_count = stage.split_count < 1 ? 1 : stage.split_count;

        ImGui::Checkbox("Wait", &stage.wait);
        ImGui::InputInt("Wait Tag", &stage.wait_tag);
        ImGui::InputInt("Stage Tag", &stage.stage_tag);

        bool foo = frame_flow->start_next_frame_stage == i;
        if (ImGui::Checkbox("Create Next", &foo))
        {
            frame_flow->start_next_frame_stage = i;
        }

        if (i > 0)
        {
            ed::BeginPin(stage.id.in, ed::PinKind::Input);
            ImGui::Text("->");
            ed::PinPivotAlignment(ImVec2(0.f, 0.5f));
            ed::PinPivotSize(ImVec2(0.f, 0.f));
            ed::EndPin();
            ImGui::SameLine();
        }

        ImGui::Text(stage.name);


        if (i < frame_flow->stages.size() - 1)
        {
            ImGui::SameLine();
            ed::BeginPin(stage.id.out, ed::PinKind::Output);
            ImGui::Text("->");
            ed::PinPivotSize(ImVec2(0.f, 0.f));
            ed::EndPin();
        }

        if (ImGui::Button("Add"))
        {
            auto new_stage = std::make_shared<FrameStage>(stage.name, stage.stage_tag, stage.weight, stage.split_count, stage.wait, stage.wait_tag);
            frame_flow->stages.insert(frame_flow->stages.begin() + i, new_stage);
            ImGui::SameLine();
        }

        if (frame_flow->stages.size() > 1)
        {
            if (ImGui::Button("Delete")) {
                if (frame_flow->start_next_frame_stage == i)
                {
                    frame_flow->start_next_frame_stage = 0;
                }
                frame_flow->stages.erase(frame_flow->stages.begin() + i);
            }
        }

        pos.x += offset;

        ImGui::PopID();

        ImGui::PopItemWidth();
        ed::EndNode();
    }

    ed::End();

    ImGui::End();
}

void DrawNodeEditor(bool* opened)
{
    ImGui::SetNextWindowPos(ImVec2(300, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1200, 600), ImGuiSetCond_FirstUseEver);
    if (!ImGui::Begin("Frame Pattern Editor", opened))
    {
        ImGui::End();
        return;
    }

    auto& io = ImGui::GetIO();

    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

    ImGui::Separator();

    ed::Begin("Frame Pattern Editor", ImVec2(0, 0));
    int uniqueId = 1;
    // Start drawing nodes.

    for(auto& pair : App::get().Pattern->Types)
    {
        auto& j = pair.second;
        ed::BeginNode(j->nid);
        ImGui::PushID(j->nid);

        ImGui::PushItemWidth(100.f);
        ImGui::InputText("Name", j->name, 255);
        ImGui::DragFloat("Duration", &j->duration, 1.f, 10.f, 300.f);
        ImGui::Checkbox("Generate Next Frame", &j->generate_next);
        ImGui::Checkbox("Generate Is Priority", &j->generation_priority);
        ImGui::Checkbox("Release Frame", &j->release_frame);
        ImGui::Checkbox("Wait Previous", &j->wait_previous);
        ImGui::DragInt("Subdive Count", &j->count, 1.f, 1, 100);
        ImGui::Text("%u, %u, %u, %u", j->nid, j->lid, j->iid, j->oid);
        ImGui::PopID();
        ed::BeginPin(j->iid, ed::PinKind::Input);
        ImGui::Text("->");
        ed::PinPivotAlignment(ImVec2(0.f, 0.5f));
        ed::PinPivotSize(ImVec2(0.f, 0.f));
        ed::EndPin();
        ImGui::SameLine();
        ImGui::Text(j->name);
        ImGui::SameLine();

        ed::BeginPin(j->oid, ed::PinKind::Output);
        ImGui::Text("->");

        ed::PinPivotSize(ImVec2(0.f, 0.f));

        ed::EndPin();

        ImGui::PopItemWidth();
        ed::EndNode();

    }
    for (auto& pair : App::get().Pattern->Types)
    {
        auto& j = pair.second;
        if (j->next)
        {
            ed::Link(j->lid, j->oid, j->next->iid, ImVec4(1, 1, 1, 1), 3.f);
        }
    }

    if (ed::BeginCreate())
    {
        ed::PinId startPinId = 0, endPinId = 0;
        if (ed::QueryNewLink(&startPinId, &endPinId))
        {
            auto in = App::get().Pattern->Types[App::get().Pattern->Id2Node[(uint32_t)static_cast<uintptr_t>(startPinId)]];
            auto out = App::get().Pattern->Types[App::get().Pattern->Id2Node[(uint32_t)(uintptr_t) endPinId]];


            if (in->iid == (uint32_t)(uintptr_t) startPinId)
            {
                std::swap(in, out);
                std::swap(startPinId, endPinId);
            }

            if ((uint32_t)(uintptr_t) endPinId == in->iid)
            {
                in->next = nullptr;
            }
            else if (startPinId && endPinId)
            {
                if (endPinId == startPinId)
                {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                }
                else
                {
                    if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                    {
                        in->next = out;
                    }
                }
            }
        }

    }

    ed::EndCreate();

    if (ed::BeginDelete())
    {
        ed::NodeId id;
        while (ed::QueryDeletedNode(&id))
        {
            int i = 0;
            i += 1;
            // Inform we decide to delete the node or not
            if (ed::AcceptDeletedItem())
            {
            }
        }
    }

    ed::EndDelete();

    auto mousePos = ImGui::GetMousePos();

    ed::NodeId nid;
    ed::PinId pid;
    ed::LinkId lid;
    ed::Suspend();
    if (ed::ShowNodeContextMenu(&nid))
        ImGui::OpenPopup("Node Context Menu");
    else if (ed::ShowPinContextMenu(&pid))
        ImGui::OpenPopup("Pin Context Menu");
    else if (ed::ShowLinkContextMenu(&lid))
        ImGui::OpenPopup("Link Context Menu");
    else if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Add Node");
    }

    if (ImGui::BeginPopup("Add Node"))
    {
        if (ImGui::MenuItem("Create"))
        {
            auto j = std::make_shared<JobType>();
            ed::SetNodePosition(j->nid, mousePos);
            App::get().Pattern->add(j);
        }

        ImGui::EndPopup();
    }

    ed::Resume();



    ed::End();

    ImGui::End();
}

void OtherNodeEditor(bool *opened)
{
        ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiSetCond_FirstUseEver);
        if (!ImGui::Begin("Example: Custom Node Graph", opened))
        {
            ImGui::End();
            return;
        }

        // Dummy
        struct Node
        {
            int     ID;
            char    Name[32];
            ImVec2  Pos, Size;
            float   Value;
            ImVec4  Color;
            int     InputsCount, OutputsCount;

            Node(int id, const char* name, const ImVec2& pos, float value, const ImVec4& color, int inputs_count, int outputs_count) { ID = id; strncpy(Name, name, 31); Name[31] = 0; Pos = pos; Value = value; Color = color; InputsCount = inputs_count; OutputsCount = outputs_count; }

            ImVec2 GetInputSlotPos(int slot_no) const { return ImVec2(Pos.x, Pos.y + Size.y * ((float)slot_no + 1) / ((float)InputsCount + 1)); }
            ImVec2 GetOutputSlotPos(int slot_no) const { return ImVec2(Pos.x + Size.x, Pos.y + Size.y * ((float)slot_no + 1) / ((float)OutputsCount + 1)); }
        };
        struct NodeLink
        {
            int     InputIdx, InputSlot, OutputIdx, OutputSlot;

            NodeLink(int input_idx, int input_slot, int output_idx, int output_slot) { InputIdx = input_idx; InputSlot = input_slot; OutputIdx = output_idx; OutputSlot = output_slot; }
        };

        static ImVector<Node> nodes;
        static ImVector<NodeLink> links;
        static bool inited = false;
        static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
        static bool show_grid = true;
        static int node_selected = -1;
        if (!inited)
        {
            nodes.push_back(Node(0, "MainTex", ImVec2(40, 50), 0.5f, ImColor(255, 100, 100), 1, 1));
            nodes.push_back(Node(1, "BumpMap", ImVec2(40, 150), 0.42f, ImColor(200, 100, 200), 1, 1));
            nodes.push_back(Node(2, "Combine", ImVec2(270, 80), 1.0f, ImColor(0, 200, 100), 2, 2));
            links.push_back(NodeLink(0, 0, 2, 0));
            links.push_back(NodeLink(1, 0, 2, 1));
            inited = true;
        }

        // Draw a list of nodes on the left side
        bool open_context_menu = false;
        int node_hovered_in_list = -1;
        int node_hovered_in_scene = -1;
        ImGui::BeginChild("node_list", ImVec2(100, 0));
        ImGui::Text("Nodes");
        ImGui::Separator();
        for (int node_idx = 0; node_idx < nodes.Size; node_idx++)
        {
            Node* node = &nodes[node_idx];
            ImGui::PushID(node->ID);
            if (ImGui::Selectable(node->Name, node->ID == node_selected))
                node_selected = node->ID;
            if (ImGui::IsItemHovered())
            {
                node_hovered_in_list = node->ID;
                open_context_menu |= ImGui::IsMouseClicked(1);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginGroup();

        const float NODE_SLOT_RADIUS = 4.0f;
        const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

        // Create our child canvas
        ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Checkbox("Show grid", &show_grid);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
        ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        ImGui::PushItemWidth(120.0f);

        ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        // Display grid
        if (show_grid)
        {
            ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
            float GRID_SZ = 64.0f;
            ImVec2 win_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_sz = ImGui::GetWindowSize();
            for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
                draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
            for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
                draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
        }

        // Display links
        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0); // Background
        for (int link_idx = 0; link_idx < links.Size; link_idx++)
        {
            NodeLink* link = &links[link_idx];
            Node* node_inp = &nodes[link->InputIdx];
            Node* node_out = &nodes[link->OutputIdx];
            ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
            ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);
            draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
        }

        // Display nodes
        for (int node_idx = 0; node_idx < nodes.Size; node_idx++)
        {
            Node* node = &nodes[node_idx];
            ImGui::PushID(node->ID);
            ImVec2 node_rect_min = offset + node->Pos;

            // Display node contents first
            draw_list->ChannelsSetCurrent(1); // Foreground
            bool old_any_active = ImGui::IsAnyItemActive();
            ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
            ImGui::BeginGroup(); // Lock horizontal position
            ImGui::Text("%s", node->Name);
            ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
            ImGui::ColorEdit3("##color", &node->Color.x);
            ImGui::EndGroup();

            // Save the size of what we have emitted and whether any of the widgets are being used
            bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
            node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
            ImVec2 node_rect_max = node_rect_min + node->Size;

            // Display node box
            draw_list->ChannelsSetCurrent(0); // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            ImGui::InvisibleButton("node", node->Size);
            if (ImGui::IsItemHovered())
            {
                node_hovered_in_scene = node->ID;
                open_context_menu |= ImGui::IsMouseClicked(1);
            }
            bool node_moving_active = ImGui::IsItemActive();
            if (node_widgets_active || node_moving_active)
                node_selected = node->ID;
            if (node_moving_active && ImGui::IsMouseDragging(0))
                node->Pos = node->Pos + ImGui::GetIO().MouseDelta;

            ImU32 node_bg_color = (node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && node_selected == node->ID)) ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255);
            draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
            draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);
            for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++)
                draw_list->AddCircleFilled(offset + node->GetInputSlotPos(slot_idx), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
            for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++)
                draw_list->AddCircleFilled(offset + node->GetOutputSlotPos(slot_idx), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));

            ImGui::PopID();
        }
        draw_list->ChannelsMerge();

        // Open context menu
        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
        {
            node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
            open_context_menu = true;
        }
        if (open_context_menu)
        {
            ImGui::OpenPopup("context_menu");
            if (node_hovered_in_list != -1)
                node_selected = node_hovered_in_list;
            if (node_hovered_in_scene != -1)
                node_selected = node_hovered_in_scene;
        }

        // Draw context menu
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("context_menu"))
        {
            Node* node = node_selected != -1 ? &nodes[node_selected] : NULL;
            ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
            if (node)
            {
                ImGui::Text("Node '%s'", node->Name);
                ImGui::Separator();
                if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
                if (ImGui::MenuItem("Delete", NULL, false, false)) {}
                if (ImGui::MenuItem("Copy", NULL, false, false)) {}
            }
            else
            {
                if (ImGui::MenuItem("Add")) { nodes.push_back(Node(nodes.Size, "New node", scene_pos, 0.5f, ImColor(100, 100, 200), 2, 2)); }
                if (ImGui::MenuItem("Paste", NULL, false, false)) {}
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        // Scrolling
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
            scrolling = scrolling + ImGui::GetIO().MouseDelta;

        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::EndGroup();

        ImGui::End();
}

int FrameFlow::count_stage(int wait_tag)
{
    int count = 0;
    for (int i = 0; i < stages.size(); i++)
    {
        if (stages[i]->stage_tag == wait_tag)
        {
            count += 1;
        }
    }

    return count;
}
float FrameFlow::compute_critical_path_time()
{
    float time = 0.f;
    for (int i = 0; i < stages.size(); i++)
    {
        time += stage_duration(i);
    }

    return time;
}

float FrameFlow::stage_duration(int index)
{
    float total = 0.f;

    for (auto s : stages)
    {
        total += s->weight;
    }

    total *= (float) stages[index]->split_count;
    
    float coeff = stages[index]->weight / total;
    float val = duration * coeff;

    return val;
}

void create_job(std::shared_ptr<FrameFlow> flow, int index, Simulator* sim, std::shared_ptr<Frame> frame)
{
    int count = flow->stages[index]->split_count;
    if (count > 1) {

        auto counter = std::make_shared<int>(count);
        for (int i = 0; i < count; i++) {
            sim->push_job(std::make_shared<PatternJob>(flow, index, sim, frame, counter));
        }
    }
    else {
        sim->push_job(std::make_shared<PatternJob>(flow, index, sim, frame));
    }
}
