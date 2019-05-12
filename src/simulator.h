#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <vector>
#include <list>
#include <memory>

struct SimulationContext;

struct FrameSetting
{
    int resolution = 10000;

    float scale = 150.0f;
    bool scaleChanged = false;
    int coreCount = 8;
    float lineHeight = 20.f;
    float latencyLineHeight = 15.f;
    float margin = 10.f;
    ImVec2 coreOffset = ImVec2(50.f, 0.f);
    int deltaTimeSampleCount = 16;
    int perturbationIndex = 0;
    float perturbationSimRatio = 1.0f;
    float perturbationPrepRatio = 1.0f;
    float perturbationGpuRatio = 1.0f;

    bool vsyncEnabled = false;
    float GpuDuration = 1.0f;
    float CpuDuration = 1.0f;
    float CpuSimRatio = 0.5f;
    float CpuSimDuration = 0.5f;
	float CpuPrepDuration = 0.5f;
    int frameCount = 3;
    int maxFrameIndex = 100;

    int inline CpuSimTime(int index) const {
        float pert = perturbationIndex == index ? perturbationSimRatio : 1.0f;

        return static_cast<int>(CpuSimRatio * CpuDuration * resolution * pert);
    }
    int inline CpuPrepTime(int index) const {
        float pert = perturbationIndex == index ? perturbationPrepRatio : 1.0f;

        return static_cast<int>((1.0f - CpuSimRatio) * CpuDuration * resolution * pert);
    }
    int inline GpuTime(int index) const {
        float pert = perturbationIndex == index ? perturbationGpuRatio : 1.0f;

        return static_cast<int>(GpuDuration * resolution * pert);
    }
    int inline ToTime(float position) const {
        return static_cast<int>((position - coreOffset.x) * resolution / scale);
    }

    float inline ToPosition(int time) const {
        return (scale * time) / resolution + coreOffset.x;
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
        int GpuStopTime = -1;
        int GpuPresentTime = -1;

        inline bool IsDone() const { return CpuSimStartTime >= 0 && CpuPrepStartTime >= 0 && GpuStartTime >= 0 && GpuPresentTime >= 0
            && CpuSimCoreIndex >= 0 && CpuPrepCoreIndex >= 0 && GpuStopTime >= 0; }

        inline int Latency() const {
            return GpuPresentTime - CpuSimStartTime;
        }

        inline int RelativePrepTime() const {
            return CpuPrepStartTime - CpuSimStartTime;
        }

        inline int RelativeGpuTime() const {
            return GpuStartTime - CpuSimStartTime;
        }
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
        bool stable = false;
        bool missed = false;
        bool isPerturbation = false;
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
