#include "simulator.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <sstream>

void FrameSimulator::DrawOptions(FrameSimulator::Setting& setting)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Options");

    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Core Number", &setting.coreCount, 1, 16);
        ImGui::SliderFloat("Zoom", &setting.scale, 1.0f, 10.0f);
    }

    ImGui::End();
}

void FrameSimulator::Draw(const FrameSimulator::Setting& setting)
{
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