#include "simulator.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "debug.h"

#include <sstream>
#include <algorithm>

#include <assert.h>
#include <iostream>

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

namespace
{
    void Round100(float& f)
    {
        float r = static_cast<float>(static_cast<int>(f * 100.f)) / 100.f;
        f = r;
    }
}

void FrameSimulator::DrawOptions(FrameSimulator::Setting& setting)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Options");

    const float f32_0 = 0.f;
    const float f32_1 = 1.0f;
    const float f32_2 = 2.0f;
    const float f32_3 = 3.0f;
    const float f32_4 = 4.0f;
    const int s32_1 = 1;
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Core Count", &setting.coreCount, 1, 16);
        ImGui::SliderInt("Frame Count", &setting.frameCount, 1, 16);

        setting.scaleChanged = ImGui::SliderFloat("Vsync Period", &setting.scale, 20.0f, 350.0f);

        ImGui::DragScalar("Gpu Duration", ImGuiDataType_Float, &setting.GpuDuration, 0.01f, &f32_0, &f32_2, "%f", 1.0f);
        bool cpuDurationChanged = ImGui::DragScalar("All Cpu Duration", ImGuiDataType_Float, &setting.CpuDuration, 0.01f, &f32_0, &f32_4, "%f", 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            cpuDurationChanged = true;
            setting.CpuDuration = 1.0f;
        }
        if (cpuDurationChanged) {
            if (setting.CpuDuration > 0.f)
            {
                setting.CpuSimDuration = setting.CpuSimRatio * setting.CpuDuration;
            }
            else {
                setting.CpuSimDuration = 0.f;
            }
        }
        bool cpuSimRatioChanged = ImGui::DragScalar("CpuSim Ratio", ImGuiDataType_Float, &setting.CpuSimRatio, 0.01f, &f32_0, &f32_1, "%f", 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset1"))
        {
            cpuSimRatioChanged = true;
            setting.CpuSimRatio = 0.5f;
        }
        if (cpuSimRatioChanged) {
            setting.CpuSimDuration = setting.CpuSimRatio * setting.CpuDuration;
        }
        bool cpuSimDurationChanged = ImGui::DragScalar("CpuSim Duration", ImGuiDataType_Float, &setting.CpuSimDuration, 0.01f, &f32_0, &f32_4, "%f", 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset2"))
        {
            cpuSimDurationChanged = true;
            setting.CpuSimDuration = 0.5f;
        }
        if (cpuSimDurationChanged)
        {
            float old = setting.CpuSimRatio;
            float oldPrepDuration = (1.0f - setting.CpuSimRatio) * setting.CpuDuration;
            setting.CpuDuration = setting.CpuSimDuration + oldPrepDuration;
            setting.CpuSimRatio = setting.CpuSimDuration / setting.CpuDuration;
            std::cout << "old: " << old << ", new: " << setting.CpuSimRatio << '\n';
        }
        ImGui::Checkbox("Vsync Enabled", &setting.vsyncEnabled);
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
        frame.GpuStopTime = frame.GpuStartTime + context.setting.GpuTime();
        if (context.setting.vsyncEnabled)
        {
            frame.GpuPresentTime = (((frame.GpuStopTime - 1) / context.setting.GpuFrameDuration) + 1 )* context.setting.GpuFrameDuration;
        }
        else
        {
            frame.GpuPresentTime = frame.GpuStopTime;
        }
        DRGN_ASSERT(frame.GpuStopTime <= frame.GpuPresentTime);

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

    int stableFrameIndex = 0;
    for (int i = 1; i < (int)context.frames.size(); i++)
    {
        const SimulationContext::Frame& frame = context.frames[i];
        const SimulationContext::Frame& prev = context.frames[i - 1];

        int fr = frame.GpuPresentTime - prev.GpuPresentTime;
        int prevFr = fr;
        if (i > 1) {
            const SimulationContext::Frame& prev2 = context.frames[i-2];
            prevFr = prev.GpuPresentTime - prev2.GpuPresentTime;
        }
        if (!(frame.Latency() == prev.Latency()
            && frame.RelativePrepTime() == prev.RelativePrepTime()
            && frame.RelativeGpuTime() == prev.RelativeGpuTime()
            && fr == prevFr))
        {
            stableFrameIndex = i;
        }
    }
    // Dummy simulation
    m_timeboxes.clear();
    m_latencyBoxes.clear();
    m_frameRates.clear();
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
        gpu.stopTime = frame.GpuStopTime;
        gpu.isGpuTimeBox = true;
        gpu.name = "Render";
        TimeBox gpuPresent;
        gpuPresent.frameIndex = i;
        gpuPresent.startTime = frame.GpuStopTime;
        gpuPresent.stopTime = frame.GpuPresentTime;
        gpuPresent.isGpuTimeBox = true;
        gpuPresent.name = "Present";
        m_timeboxes.push_back(cpuSim);
        if (cpuPrep.stopTime > cpuPrep.startTime)
        {
            m_timeboxes.push_back(cpuPrep);
        }
        m_timeboxes.push_back(gpu);
        if (gpuPresent.stopTime > gpuPresent.startTime)
        {
            m_timeboxes.push_back(gpuPresent);
        }

        LatencyBox l;
        l.frameIndex = i;
        l.startTime = frame.CpuSimStartTime;
        l.stopTime = frame.GpuPresentTime;
        m_latencyBoxes.push_back(l);

        FrameRate fr;
        fr.frameIndex = i;
        fr.time = frame.GpuPresentTime;
        fr.firstStable = i == stableFrameIndex;
        fr.stable = i >= stableFrameIndex;
        if (i > 0)
        {
            fr.duration = frame.GpuPresentTime - context.frames[i - 1].GpuPresentTime;
        }
        else
        {
            fr.duration = frame.GpuPresentTime;
        }
        m_frameRates.push_back(fr);
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

    float scroll = ImGui::GetScrollX();

    if (setting.scaleChanged)
    {
        scroll = setting.ToPosition(m_previousTimeMin);
        ImGui::SetScrollX(scroll);
        int timeMin = setting.ToTime(scroll);
        std::cout << "new scroll: " << scroll << ", expected timeMin: " << timeMin;
    }
    ImVec2 offset(-scroll, 0.f);
    int timeMin = setting.ToTime(scroll);
    float s = setting.ToPosition(timeMin);
    m_previousTimeMin = timeMin;
    if (setting.scaleChanged)
    {
        std::cout << ", timeMin: " << m_previousTimeMin << '\n';
    }
    int timeMax = setting.ToTime(scroll + ImGui::GetWindowSize().x);

    for (const auto& f : m_frameRates)
    {
        if (timeMin <= f.time && f.time <= timeMax) {
            DrawFrameRate(context, f, offset);
        }
    }
    int maxTime = 0;
    for (const auto& t : m_timeboxes)
    {
        DRGN_ASSERT(t.startTime <= t.stopTime);
        if (t.startTime <= timeMax && t.stopTime >= timeMin) {
            DrawTimeBox(context, t, offset);
        }
        maxTime = std::max(maxTime, t.stopTime);
    }

    DrawCoreLabel(context);

    for (const auto& t : m_latencyBoxes)
    {
        DRGN_ASSERT(t.startTime <= t.stopTime);
        if (t.startTime <= timeMax && t.stopTime >= timeMin) {
            DrawLatencyBox(context, t, offset);
        }
        maxTime = std::max(maxTime, t.stopTime);
    }

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
    ImVec4 clip(p0.x + offset.x, p0.y + offset.y, p1.x + offset.x, p1.y + offset.y);
    context.drawlist.AddText(NULL, 0.0f, p0 + offsetFrame + offset, c, name, NULL, 0.f, &clip);
}

void FrameSimulator::DrawLatencyBox(const DrawContext& context, const LatencyBox& box, const ImVec2& offset)
{
    ImVec2 origin = context.latencyOrigin + ImVec2(0.f, (box.frameIndex % context.setting.frameCount)* context.setting.latencyLineHeight);

    ImVec2 p0 = origin;
    ImVec2 p1 = origin;
    p0.x += context.setting.ToPosition(box.startTime);
    p1.x += context.setting.ToPosition(box.stopTime);
    p1.y += context.setting.latencyLineHeight;

    ImU32 color = PickColor(box.frameIndex);
    context.drawlist.AddRectFilled(p0 + offset, p1 + offset, color, 6.f, ImDrawCornerFlags_All);

    ImVec2 size = p1 - p0;
    ImU32 c = GetConstrastColor(~color);

    float latency = (float(box.stopTime - box.startTime)) / context.setting.GpuFrameDuration;
    std::stringstream s;
    s << latency;
    std::string str = s.str();
    const char* name = str.c_str();

    ImVec2 textFrameSize = ImGui::CalcTextSize(name);
    ImVec2 offsetFrame = (size - textFrameSize) * 0.5f;
    offsetFrame.x = 2.f;
    context.drawlist.AddText(p0 + offsetFrame + offset, c, name);
}

void FrameSimulator::DrawFrameRate(const DrawContext& context, const FrameRate& fr, const ImVec2& offset)
{
    ImVec2 origin = context.frameRateOrigin;

    ImVec2 p0 = origin;
    ImVec2 p1 = origin;
    p0.x += context.setting.ToPosition(fr.time);
    p1.x = p0.x;
    p1.y += context.windowSize.y;

    ImU32 color = g_DarkGrey;
    if (fr.stable)
    {
        color = g_Red;
    }
    float duration = (float(fr.duration)) / context.setting.GpuFrameDuration;
    context.drawlist.AddLine(p0 + offset, p1 + offset, color, 1.f);
    std::stringstream framerateText;
    framerateText << '[' << fr.frameIndex << "] " << duration;
    std::string text_str = framerateText.str();
    const char* text = text_str.c_str();
    ImVec2 textFrameSize = ImGui::CalcTextSize(text);
    context.drawlist.AddText(p0 + ImVec2(-textFrameSize.x - 5.f, 0.f) + offset, g_Grey, text);
}
