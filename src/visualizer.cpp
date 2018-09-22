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
ImU32 g_LightGrey = ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
ImU32 g_Black = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

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
float g_TimeBoxAverageSize = 100.f;
int g_TimeBoxNumber = 100;

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

ImU32 GetConstrastColor(ImU32 color)
{
    int a0 = (color & 0x000000ff) >= 128 ? 1 : 0;
    int a1 = ((color >> 8) & 0x000000ff) >= 128 ? 1 : 0;
    int a2 = ((color >> 16) & 0x000000ff) >= 128 ? 1 : 0;

    if (a0 + a1 + a2 < 2)
    {
        return g_Black;
    }
    else
    {
        return g_White;
    }
}

void DrawTimeBox(ImVec2 origin, const TimeBox& timebox)
{
    auto drawList = ImGui::GetWindowDrawList();
    auto win = ImGui::GetWindowPos();

    auto p0 = win + origin + TimeBoxP0(timebox);
    auto p1 = win + origin + TimeBoxP1(timebox);
    drawList->AddRectFilled(p0, p1, timebox.color, 3.5f, ImDrawCornerFlags_All);

    ImVec2 size = p1 - p0;
    ImU32 c = GetConstrastColor(~timebox.color);
    ImVec2 textFrameSize = ImGui::CalcTextSize(timebox.frame_index.c_str());
    ImVec2 offsetFrame = (size - textFrameSize) * 0.5f;
    offsetFrame.x = 2.f;
    drawList->AddText(p0 + offsetFrame, c, timebox.frame_index.c_str());

    ImVec2 textNameSize = ImGui::CalcTextSize(timebox.name.c_str());
    ImVec2 offsetName = (size - textFrameSize) * 0.5f;
    offsetName.x = size.x - textNameSize.x - 2.f;
    auto clip = ImVec4(p0.x + textFrameSize.x + 2, p0.y, p1.x, p1.y);
    drawList->AddText(nullptr, 0.f, p0 + offsetName, c, timebox.name.c_str(), nullptr, 0.f, &clip);
}

}

void DrawVisualizer()
{
    bool yes = true;

    ImGui::Begin("Frame Centric Simulation - Options", &yes);
    ImGui::SliderInt("Core Number", &g_CoreNumber, 1, 16);
    ImGui::SliderFloat("TimeBox Size", &g_TimeBoxAverageSize, 10.f, 200.f);

    //static float average = 1.f;
    static float stddev = 0.5f;
    //ImGui::SliderFloat("Average", &average, 0.f, 3.f);
    ImGui::SliderFloat("Standard Deviation", &stddev, 0.0f, 0.9f);
    ImGui::End();

    std::mt19937 gen(0);
    std::uniform_real_distribution<> dis(1.f - stddev, 1.f + stddev);
    auto random = [&gen, &dis] { return dis(gen); };



    ImGui::Begin("Frame Centric Simulation", &yes, ImGuiWindowFlags_HorizontalScrollbar);
    auto origin = ImGui::GetCursorPos() - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    auto max = ImGui::GetCursorPos();
    ImU32 col = 0;
    for (int core = 0; core < g_CoreNumber; core++)
    {
        float time = 0.f;
        for (int t = 0; t < g_TimeBoxNumber; t++)
        {
            float timespan = g_TimeBoxAverageSize * random();
            auto timebox = TimeBox(core, t, time, time + timespan, "Frame", g_Colors[col]);
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