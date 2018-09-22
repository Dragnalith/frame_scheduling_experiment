#include "visualizer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <random>

namespace
{

const ImVec2 g_Origin(10.f, 50.f);
constexpr float g_Height = 20.f;
ImU32 g_White = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
ImU32 g_Green = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
ImU32 g_Red = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
ImU32 g_Blue = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
ImU32 g_Yellow = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
ImU32 g_Cyan = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
ImU32 g_Magenta = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
ImU32 g_Grey = ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

ImU32 g_Colors[] = {
    g_White,
    g_Green,
    g_Red,
    g_Blue,
    g_Yellow,
    g_Cyan,
    g_Magenta,
    g_Grey
};

int g_CoreNumber = 4;
int g_TimeBoxNumber = 700;

template<class T, size_t N>
constexpr size_t array_size(T(&)[N]) { return N; }

ImVec2 TimeBoxP0(const TimeBox& timebox)
{
    return ImVec2(timebox.start_time, timebox.core_index * g_Height);
}
ImVec2 TimeBoxP1(const TimeBox& timebox)
{
    return TimeBoxP0(timebox) + ImVec2(timebox.end_time - timebox.start_time, g_Height);
}
void DrawTimeBox(ImVec2 origin, const TimeBox& timebox)
{
    auto drawList = ImGui::GetWindowDrawList();
    auto win = ImGui::GetWindowPos();

    auto p0 = win + origin + TimeBoxP0(timebox);
    auto p1 = win + origin + TimeBoxP1(timebox);
    drawList->AddRectFilled(p0, p1, timebox.color, 3.5f, ImDrawCornerFlags_All);
}

}

void DrawVisualizer()
{
    std::mt19937 gen(0);
    std::uniform_real_distribution<> dis(1.2, 2.0);
    auto random = [&gen, &dis] { return dis(gen); };

    bool yes = true;
    ImGui::Begin("Frame Centric Simulation", &yes, ImGuiWindowFlags_HorizontalScrollbar);
    auto origin = ImGui::GetCursorPos() - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    auto max = ImGui::GetCursorPos();
    ImU32 col = 0;
    for (int core = 0; core < g_CoreNumber; core++)
    {
        float time = 0.f;
        for (int t = 0; t < g_TimeBoxNumber; t++)
        {
            float timespan = 30.f * random();
            auto timebox = TimeBox(core, time, time + timespan, g_Colors[col]);
            DrawTimeBox(origin, timebox);
            time += timespan;
            max = ImMax(max, TimeBoxP1(timebox));
            col = (col + 1) % array_size(g_Colors);
        }
    }

    // Add an offset to scroll a bit more than the max of the timeline
    ImGui::SetCursorPos(max + ImVec2(100.f, 0.f));

    ImGui::End();
}