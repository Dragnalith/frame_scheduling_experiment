#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <vector>
#include <list>
#include <memory>

struct SimulationContext;

struct FrameSetting
{
    static constexpr int GpuFrameDuration = 10000;

    float scale = 150.0f;
    bool scaleChanged = false;
    int coreCount = 8;
    float lineHeight = 20.f;
    float latencyLineHeight = 15.f;
    float margin = 10.f;
    ImVec2 coreOffset = ImVec2(50.f, 0.f);

    float CpuDuration = 1.0f;
    float CpuSimRatio = 0.5f;
    float CpuSimDuration = 0.5f;
    int frameCount = 3;
    int maxFrameIndex = 100;

    int inline CpuSimTime() const {
        return static_cast<int>(CpuSimRatio * CpuDuration * GpuFrameDuration);
    }
    int inline CpuPrepTime() const {
        return static_cast<int>((1.0f - CpuSimRatio) * CpuDuration * GpuFrameDuration);
    }

    int inline ToTime(float position) const {
        return static_cast<int>((position - coreOffset.x) * GpuFrameDuration / scale);
    }

    float inline ToPosition(int time) const {
        return (scale * time) / GpuFrameDuration + coreOffset.x;
    }
};

class FrameJob
{
public:
    FrameJob(int frameIndex) : m_frameIndex(frameIndex) {}
    virtual ~FrameJob() = default;

    virtual bool IsReady(const SimulationContext& simulator) const = 0;
    virtual void Run(SimulationContext& simulator) = 0;

protected:
    const int m_frameIndex;
};

struct SimulationContext
{
public:
    SimulationContext(const FrameSetting& setting);

    struct Frame
    {
        int FrameIndex = -1;
        int CpuSimStartTime = -1;
        int CpuSimCoreIndex = -1;
        int CpuPrepStartTime = -1;
        int CpuPrepCoreIndex = -1;
        int GpuStartTime = -1;
        int GpuPresentTime = -1;

        inline bool IsDone() const { return CpuSimStartTime >= 0 && CpuPrepStartTime >= 0 && GpuStartTime >= 0 && GpuPresentTime >= 0
            && CpuSimCoreIndex >= 0 && CpuPrepCoreIndex >= 0; }
    };

    struct SchedulingResult
    {
        int coreIndex = -1;
        int schedulingTime = -1;
    };

    SchedulingResult Schedule(int requestTime, int duration);

    const FrameSetting& setting;

    std::vector<Frame> frames;
    std::list<std::unique_ptr<FrameJob>> jobQueue;
    std::vector<int> coreTime;
};

class FrameSimulator
{
public:
    using Setting = FrameSetting;

    void DrawOptions(Setting& setting);
    void Draw(const Setting& setting);
    void Simulate(const Setting& setting); 

private:
    struct TimeBox
    {
        static constexpr int GpuFrameDuration = FrameSetting::GpuFrameDuration;

        int startTime;
        int stopTime;
        const char* name;
        int frameIndex;
        bool isGpuTimeBox;
        int coreIndex;
    };

    struct LatencyBox
    {
        int frameIndex = -1;
        int startTime = -1;
        int stopTime = -1;
    };

    struct FrameRate
    {
        int frameIndex = -1;
        int time = -1;
        int duration = -1;
        bool firstStable = false;
        bool missed = false;
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

        const ImVec2 frameRateOrigin{ startCursorPosition + windowPosition };
        const ImVec2 gpuLineOrigin { frameRateOrigin + ImVec2(0.f, 20.f) };
        const ImVec2 cpuLineOrigin { gpuLineOrigin + ImVec2(0.f, setting.margin + setting.lineHeight)};
        const ImVec2 latencyOrigin{ cpuLineOrigin + ImVec2(0.f, setting.margin + setting.coreCount * setting.lineHeight) };
    };

    void DrawCoreLine(const DrawContext& context);
    void DrawCoreLabel(const DrawContext& context);

    // Return the bottom-leftest position
    void DrawTimeBox(const DrawContext& context, const TimeBox& timebox, const ImVec2& offset);
    void DrawLatencyBox(const DrawContext& context, const LatencyBox& timebox, const ImVec2& offset);
    void DrawFrameRate(const DrawContext& context, const FrameRate& fr, const ImVec2& offset);

private:
    std::vector<TimeBox> m_timeboxes;
    std::vector<LatencyBox> m_latencyBoxes;
    std::vector<FrameRate> m_frameRates;

    int m_previousTimeMin = -1;
};
