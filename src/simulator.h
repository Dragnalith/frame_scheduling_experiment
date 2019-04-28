#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <vector>

class FrameSimulator
{
public:
    struct Setting
    {
        float scale = 150.0f;
        int coreCount = 8;
        float lineHeight = 20.f;
        float margin = 10.f;
        ImVec2 coreOffset = ImVec2(50.f, 0.f);
    };

    void DrawOptions(Setting& setting);
    void Draw(const Setting& setting);

private:
    struct TimeBox
    {
        static constexpr int GpuFrameDuration = 10000;

        int startTime;
        int stopTime;
        const char* name;
        int frameIndex;
        bool isGpuTimeBox;
        int coreIndex;

        inline float StartPosition(float scale) const
        {
            return (float)startTime / GpuFrameDuration * scale;
        }
        inline float StopPosition(float scale) const
        {
            return (float)stopTime / GpuFrameDuration * scale;
        }
    };
    struct DrawContext
    {
        DrawContext(const Setting& set, ImDrawList& dl, const ImVec2& cursor, const ImVec2& winPos, const ImVec2& winSize)
            : setting(set)
            , startCursorPosition(cursor)
            , windowPosition(winPos)
            , windowSize(winSize)
            , drawlist(dl)
        {}

        const Setting setting;

        const ImVec2 startCursorPosition;
        const ImVec2 windowPosition;
        const ImVec2 windowSize;
        ImDrawList& drawlist;

        const ImVec2 gpuLineOrigin { startCursorPosition + windowPosition};
        const ImVec2 cpuLineOrigin { gpuLineOrigin + ImVec2(0.f, setting.margin + setting.lineHeight)};
    };

    void DrawCoreLine(const DrawContext& context);
    void DrawCoreLabel(const DrawContext& context);

    // Return the leftest position
    float DrawTimeBox(const DrawContext& context, const TimeBox& timebox);

private:
    std::vector<TimeBox> m_timeboxes;
};