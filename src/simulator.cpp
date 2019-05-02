#include "simulator.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <sstream>
#include <algorithm>

#include <assert.h>

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
        setting.scaleChanged = ImGui::SliderFloat("GPU Frame Size", &setting.scale, 150.0f, 1500.0f);
        ImGui::SliderFloat("Cpu Ratio", &setting.CpuRatio, 0.01f, 3.0f);
        ImGui::SliderFloat("Simulation Ratio", &setting.CpuSimRatio, 0.f, 1.0f);
        ImGui::SliderInt("Frame Count", &setting.frameCount, 1, 16);
    }

    ImGui::End();
}

namespace
{

class CpuSimJob;
class CpuPrepJob;
class GpuJob;

class GpuJob : public FrameJob
{
public:
    GpuJob(int frameIndex) : FrameJob(frameIndex) {}
    bool IsReady(const SimulationContext& context) const override {
        return m_frameIndex == 0 || context.frames[m_frameIndex - 1].IsDone();
    }

    void Run(SimulationContext& context) override {
        SimulationContext::Frame& frame = context.frames[m_frameIndex];

        int previousGpuPresentTime = 0;
        if (m_frameIndex > 0)
        {
            previousGpuPresentTime = context.frames[m_frameIndex - 1].GpuPresentTime;
        }
        assert(frame.CpuPrepStartTime >= 0);
        int cpuPrepEndTime = frame.CpuPrepStartTime + context.setting.CpuPrepTime();
        frame.GpuStartTime = std::max(cpuPrepEndTime, previousGpuPresentTime);
        frame.GpuPresentTime = frame.GpuStartTime + FrameSetting::GpuFrameDuration;
    }
};
class CpuPrepJob : public FrameJob
{
public:
    CpuPrepJob(int frameIndex) : FrameJob(frameIndex) {}

    bool IsReady(const SimulationContext& context) const override {
        return true;
    }

    void Run(SimulationContext& context) override {
        SimulationContext::Frame& frame = context.frames[m_frameIndex];
        assert(frame.CpuSimStartTime >= 0);
        int requestTime = frame.CpuSimStartTime + context.setting.CpuSimTime();
        auto result = context.Schedule(requestTime, context.setting.CpuPrepTime());
        frame.CpuPrepStartTime = result.schedulingTime;
        frame.CpuPrepCoreIndex = result.coreIndex;
        context.jobQueue.push_back(std::move(std::make_unique<GpuJob>(m_frameIndex)));
    }
};
class CpuSimJob : public FrameJob
{
public:
    CpuSimJob(int frameIndex) : FrameJob(frameIndex) {}

    bool IsReady(const SimulationContext& context) const override {
        return m_frameIndex < context.setting.frameCount || context.frames[m_frameIndex - context.setting.frameCount].IsDone();
    }

    void Run(SimulationContext& context) override {
        SimulationContext::Frame& frame = context.frames[m_frameIndex];
        if (m_frameIndex == 0)
        {
            int requestTime = 0;
            auto result = context.Schedule(requestTime, context.setting.CpuSimTime());
            frame.CpuSimStartTime = result.schedulingTime;
            frame.CpuSimCoreIndex = result.coreIndex;
        }
        else
        {
            int endSim = context.frames[m_frameIndex - 1].CpuSimStartTime + context.setting.CpuSimTime();
            int prevGpuPresentTime = 0;
            if (m_frameIndex >= context.setting.frameCount)
            {
                SimulationContext::Frame& prevFrame = context.frames[m_frameIndex - context.setting.frameCount];
                assert(prevFrame.IsDone());
                prevGpuPresentTime = prevFrame.GpuPresentTime;
            }
            int requestTime = std::max(endSim, prevGpuPresentTime);
            auto result = context.Schedule(requestTime, context.setting.CpuSimTime());
            frame.CpuSimStartTime = result.schedulingTime;
            frame.CpuSimCoreIndex = result.coreIndex;
        }
        context.jobQueue.push_back(std::move(std::make_unique<CpuPrepJob>(m_frameIndex)));
        if (m_frameIndex < context.setting.maxFrameIndex)
        {
            context.jobQueue.push_back(std::move(std::make_unique<CpuSimJob>(m_frameIndex + 1)));
        }
    }
};
}

SimulationContext::SimulationContext(const FrameSetting& s) : setting(s) {
    coreTime.resize(setting.coreCount);
    frames.resize(setting.maxFrameIndex + 1);
    for (int i = 0; i < setting.coreCount; i++)
    {
        coreTime[i] = 0;
    }
}
SimulationContext::SchedulingResult SimulationContext::Schedule(int requestTime, int duration)
{
    SchedulingResult result;
    for (int i = 0; i < setting.coreCount; i++)
    {
        if (coreTime[i] <= requestTime)
        {
            result.coreIndex = i;
            result.schedulingTime = requestTime;
            coreTime[i] = requestTime + duration;
            return result;
        }
    }

    result.coreIndex = 0;
    result.schedulingTime = coreTime[0];
    for (int i = 1; i < setting.coreCount; i++)
    {
        if (coreTime[i] < result.schedulingTime)
        {
            result.coreIndex = i;
            result.schedulingTime = coreTime[i];
        }
    }

    coreTime[result.coreIndex] = result.schedulingTime + duration;
    return result;
}



void FrameSimulator::Simulate(const FrameSimulator::Setting& setting)
{
    SimulationContext context(setting);

    context.jobQueue.push_back(std::move(std::make_unique<CpuSimJob>(0)));
    while (!context.jobQueue.empty())
    {
        bool doBreak = false;
        for (auto iter = context.jobQueue.begin(); iter != context.jobQueue.end(); iter++)
        {
            FrameJob* j = (*iter).get();
            assert(j != nullptr);
            if (j->IsReady(context))
            {
                j->Run(context);
                context.jobQueue.erase(iter);
                doBreak = true;
                break;
            }
        }
        assert(doBreak);
    }

    // Dummy simulation
    m_timeboxes.clear();

    for (int i = 0; i < (int) context.frames.size(); i++)
    {
        const SimulationContext::Frame& frame = context.frames[i];
        assert(frame.IsDone());
        TimeBox cpuSim;
        cpuSim.frameIndex = i;
        cpuSim.startTime = frame.CpuSimStartTime;
        cpuSim.stopTime = cpuSim.startTime + setting.CpuSimTime();
        cpuSim.coreIndex = frame.CpuSimCoreIndex;
        cpuSim.isGpuTimeBox = false;
        cpuSim.name = "CpuSim";
        TimeBox cpuPrep;
        cpuPrep.frameIndex = i;
        cpuPrep.startTime = frame.CpuPrepStartTime;
        cpuPrep.stopTime = cpuPrep.startTime + setting.CpuPrepTime();
        cpuPrep.coreIndex = frame.CpuPrepCoreIndex;
        cpuPrep.isGpuTimeBox = false;
        cpuPrep.name = "CpuPrep";
        TimeBox gpu;
        gpu.frameIndex = i;
        gpu.startTime = frame.GpuStartTime;
        gpu.stopTime = gpu.startTime + FrameSetting::GpuFrameDuration;
        gpu.isGpuTimeBox = true;
        gpu.name = "GPU";
        m_timeboxes.push_back(cpuSim);
        if (cpuPrep.stopTime > cpuPrep.startTime)
        {
            m_timeboxes.push_back(cpuPrep);
        }
        m_timeboxes.push_back(gpu);
    }
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

    ImVec2 offset(-ImGui::GetScrollX(), 0.f);
    int timeMin = setting.ToPosition(ImGui::GetScrollX());
    int timeMax = setting.ToPosition(ImGui::GetScrollX() + ImGui::GetWindowSize().x);

    int maxTime = 0;
    for (const auto& t : m_timeboxes)
    {
        assert(t.startTime <= t.stopTime);
        if (t.startTime <= timeMax && t.stopTime >= timeMin) {
            DrawTimeBox(context, t, offset);
        }
        maxTime = std::max(maxTime, t.stopTime);
    }

    DrawCoreLabel(context);

    ImVec2 endCursor = context.startCursorPosition;
    endCursor.x = setting.ToPosition(maxTime);
    ImGui::SetCursorPos(endCursor);

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

void FrameSimulator::DrawTimeBox(const DrawContext& context, const TimeBox& timebox, const ImVec2& offset)
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
    p0.x += context.setting.ToPosition(timebox.startTime);
    p1.x += context.setting.ToPosition(timebox.stopTime);
    p1.y += context.setting.lineHeight;

    ImU32 color = PickColor(timebox.frameIndex);
    context.drawlist.AddRectFilled(p0 + offset, p1 + offset, color, 3.5f, ImDrawCornerFlags_All);

    ImVec2 size = p1 - p0;
    ImU32 c = GetConstrastColor(~color);

    std::stringstream s;
    s << timebox.name << '(' << timebox.frameIndex << ')';
    std::string str = s.str();
    const char* name = str.c_str();

    ImVec2 textFrameSize = ImGui::CalcTextSize(name);
    ImVec2 offsetFrame = (size - textFrameSize) * 0.5f;
    offsetFrame.x = 2.f;
    context.drawlist.AddText(p0 + offsetFrame + offset, c, name);
}
