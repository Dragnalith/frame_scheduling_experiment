#include "simulator.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <sstream>

namespace
{


template <class T, int N>
constexpr int array_size(T(&)[N]) { return N; }

ImU32 g_White = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.0f));
ImU32 g_Green = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
ImU32 g_Red = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
ImU32 g_Blue = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
ImU32 g_Yellow = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
ImU32 g_Cyan = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
ImU32 g_Magenta = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
ImU32 g_Grey = ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
ImU32 g_DarkGrey = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
ImU32 g_Black = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

ImU32 g_Colors[] = {
    g_Grey,
    g_Green,
    g_Red,
    g_Blue,
    g_DarkGrey,
    g_Yellow,
    g_Cyan,
    g_Magenta
};

ImU32 PickColor(int index)
{
    return g_Colors[index % array_size(g_Colors)];
}

ImU32 GetConstrastColor(ImU32 color)
{
    int a0 = (color & 0x000000ff) >= 128 ? 1 : 0;
    int a1 = ((color >> 8) & 0x000000ff) >= 128 ? 1 : 0;
    int a2 = ((color >> 16) & 0x000000ff) >= 128 ? 1 : 0;

    if (a0 + a1 + a2 < 2) {
        return g_Black;
    }
    else {
        return g_White;
    }
}

}

void FrameSimulator::DrawOptions(FrameSimulator::Setting& setting)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Options");

    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Core Number", &setting.coreCount, 1, 16);
        ImGui::SliderFloat("GPU Frame Size", &setting.scale, 150.0f, 1500.0f);
    }

    ImGui::End();
}

void FrameSimulator::Draw(const FrameSimulator::Setting& setting)
{
    // Dummy simulation
    m_timeboxes.clear();

    for (int i = 0; i < 100; i++)
    {
        TimeBox g;
        g.startTime = i * TimeBox::GpuFrameDuration;
        g.stopTime = (i + 1) * TimeBox::GpuFrameDuration;
        g.frameIndex = i;
        g.isGpuTimeBox = true;
        g.coreIndex = 0;
        g.name = "GPU";

        TimeBox f;
        f.startTime = g.startTime;
        f.stopTime = g.startTime + (g.stopTime - g.startTime) / 3;
        f.frameIndex = i;
        f.isGpuTimeBox = false;
        f.coreIndex = i % setting.coreCount;
        f.name = "Sim";
        m_timeboxes.push_back(g);
        m_timeboxes.push_back(f);
    }

    // Begin Window
    ImGui::SetNextWindowSize(ImVec2(1900, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(0, 600), ImGuiCond_FirstUseEver);

    bool yes = true;
    ImGui::Begin("Frame Simulator", &yes, ImGuiWindowFlags_HorizontalScrollbar);

    // Create Draw Context
    ImDrawList& drawlist = *ImGui::GetWindowDrawList();
    const ImVec2 cursor = ImGui::GetCursorPos();
    const ImVec2 windowPosition = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();

    DrawContext context(setting, drawlist, cursor, windowPosition, windowSize);

    // Draw
    DrawCoreLine(context);

    float windowMin = ImGui::GetScrollX();
    int timeMin = (int)TimeBox::GpuFrameDuration * (windowMin / setting.scale);
    float windowMax = (windowMin + ImGui::GetWindowSize().x);
    int timeMax = (int)TimeBox::GpuFrameDuration * (windowMax / setting.scale);
    
    for (const auto& t : m_timeboxes)
    {
        if (t.startTime <= timeMax && t.stopTime >= timeMin) {
            DrawTimeBox(context, t);
        }
    }

    DrawCoreLabel(context);

    // End Window
    ImGui::End();
}

void FrameSimulator::DrawCoreLine(const DrawContext& context)
{
    const float lineHeight = context.setting.lineHeight;
    const ImVec2& coreOffset = context.setting.coreOffset;
    int coreCount = context.setting.coreCount;

    {
        auto p1 = context.gpuLineOrigin + coreOffset;
        auto p2 = p1 + ImVec2(context.windowSize.x, lineHeight);

        context.drawlist.AddRectFilled(p1, p2, 0xff171717);
    }
    for (int i = 0; i < coreCount; i++) {
        auto p1 = context.cpuLineOrigin + coreOffset;
        p1.y += i * lineHeight;
        auto p2 = p1 + ImVec2(context.windowSize.x, lineHeight);

        if (i % 2 == 0) {
            context.drawlist.AddRectFilled(p1, p2, 0xff0c0c0c);
        } else {
            context.drawlist.AddRectFilled(p1, p2, 0xff111111);
        }
    }
}

void FrameSimulator::DrawCoreLabel(const DrawContext& context)
{
    const float lineHeight = context.setting.lineHeight;
    const ImVec2& coreOffset = context.setting.coreOffset;
    int coreCount = context.setting.coreCount;
    // GPU
    {
        auto p1 = context.gpuLineOrigin + ImVec2(0.f, coreOffset.y);
        auto p2 = p1 + ImVec2(coreOffset.x, lineHeight);

        context.drawlist.AddRectFilled(p1, p2, 0xff000000);
        context.drawlist.AddText(p1, 0xffffffff, "GPU");
    }

    // CPU
    for (int i = 0; i < coreCount; i++) {
        auto p1 = context.cpuLineOrigin + ImVec2(0.f, coreOffset.y);
        p1.y += i * lineHeight;
        auto p2 = p1 + ImVec2(coreOffset.x, lineHeight);

        context.drawlist.AddRectFilled(p1, p2, 0xff000000);
        std::stringstream s;
        s << "Core " << i;
        context.drawlist.AddText(p1, 0xffffffff, s.str().c_str());
    }
}

float FrameSimulator::DrawTimeBox(const DrawContext& context, const TimeBox& timebox)
{
    ImVec2 origin;
    if (timebox.isGpuTimeBox)
    {
        origin = context.gpuLineOrigin;
    }
    else
    {
        origin = context.cpuLineOrigin + ImVec2(0.f, timebox.coreIndex * context.setting.lineHeight);
    }

    ImVec2 p0 = origin;
    ImVec2 p1 = origin;
    p0.x += timebox.StartPosition(context.setting.scale);
    p1.x += timebox.StopPosition(context.setting.scale);
    p1.y += context.setting.lineHeight;

    ImU32 color = PickColor(timebox.frameIndex);
    context.drawlist.AddRectFilled(p0, p1, color, 3.5f, ImDrawCornerFlags_All);

    ImVec2 size = p1 - p0;
    ImU32 c = GetConstrastColor(~color);

    std::stringstream s;
    s << timebox.name << '(' << timebox.frameIndex << ')';
    std::string str = s.str();
    const char* name = str.c_str();

    ImVec2 textFrameSize = ImGui::CalcTextSize(name);
    ImVec2 offsetFrame = (size - textFrameSize) * 0.5f;
    offsetFrame.x = 2.f;
    context.drawlist.AddText(p0 + offsetFrame, c, name);

    return p1.x;
}
