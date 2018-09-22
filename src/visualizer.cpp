#include "visualizer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <random>
#include <memory>
#include <assert.h>

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
int g_FramePoolSize = 4;
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

struct Job
{
    float duration;
    int frame_index;
};

struct Core
{
    int index;
    float time = 0.f;
};

class Simulator
{
public:
    Simulator(int core, int frame_pool, float stddev)
        : m_core_count(core)
        , m_frame_pool_size(frame_pool)
        , m_frame_count(0)
        , m_generator(0)
        , m_distribution(100.f * (1.f - stddev), 100.f *(1.f + stddev))
    {
        for (int i = 0; i < m_core_count; i++)
        {
            m_cores.emplace_back();
        }
        for (int i = 0; i < m_core_count; i++)
        {
            m_cores[i].index = i;
        }

        for (int i = 0; i < m_frame_pool_size; i++)
        {
            m_job_queue.push_back(generate());
        }
    }

    void step()
    {
        auto* min_core = &m_cores[0];
        for (auto& c : m_cores)
        {
            if (c.time < min_core->time)
            {
                min_core = &c;
            }
        }

        Job j = pop_job();

        m_timeboxes.emplace_back(min_core->index, j.frame_index, min_core->time, min_core->time + j.duration, "Job", g_Colors[j.frame_index % array_size(g_Colors)]);
        m_max = ImMax(m_max, TimeBoxP1(m_timeboxes.back()));
        min_core->time += j.duration;
    }

    const std::vector<TimeBox>& get_timeboxes() const { return m_timeboxes; }
    const ImVec2& get_max() const { return m_max; }
    const std::deque<Job>& get_queue() const { return m_job_queue; }

private:
    Job generate()
    {
        Job j;
        j.duration = m_distribution(m_generator);
        j.frame_index = m_frame_count;
        m_frame_count += 1;
        return j;
    }

    Job pop_job()
    {
        assert(!m_job_queue.empty());
        Job j = m_job_queue.front();
        m_job_queue.pop_front();
        m_job_queue.push_back(generate());
        return j;
    }


private:
    int m_core_count;
    int m_frame_pool_size;
    int m_frame_count;
    ImVec2 m_max;

    std::mt19937 m_generator;
    std::uniform_real_distribution<> m_distribution;

    std::vector<Core> m_cores;
    std::deque<Job> m_job_queue;
    std::deque<int> m_frame_pool;
    std::vector<TimeBox> m_timeboxes;
};


void DrawVisualizer()
{
    bool yes = true;
    static float stddev = 0.5f;
    static bool generate = true;
    static bool step = false;
    static bool autoStep = false;
    static std::unique_ptr<Simulator> simulator;

    std::mt19937 gen(0);
    std::uniform_real_distribution<> dis(1.f - stddev, 1.f + stddev);
    auto random = [&gen, &dis] { return dis(gen); };



    ImGui::Begin("Frame Centric Simulation", &yes, ImGuiWindowFlags_HorizontalScrollbar);
    auto origin = ImGui::GetCursorPos() - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    ImU32 col = 0;

    if (generate)
    {
        simulator = std::make_unique<Simulator>(g_CoreNumber, g_FramePoolSize, stddev);
        generate = false;
    }
    if (step || autoStep)
    {
        simulator->step();
        step = false;
    }

    float windowMin = ImGui::GetScrollX();
    float windowMax = windowMin + ImGui::GetWindowSize().x;
    int count = 0;
    for (const auto& t : simulator->get_timeboxes())
    {
        if (t.start_time <= windowMax && t.end_time >= windowMin)
        {
            DrawTimeBox(origin, t);
            count += 1;
        }
    }

    // Add an offset to scroll a bit more than the max of the timeline
    ImGui::SetCursorPos(simulator->get_max() + ImVec2(100.f, 0.f));

    ImGui::End();


    ImGui::Begin("Frame Centric Simulation - Options", &yes);
    ImGui::SliderInt("Core Number", &g_CoreNumber, 1, 16);
    ImGui::SliderInt("Frame Pool Size", &g_FramePoolSize, 1, 16);
    ImGui::SliderFloat("TimeBox Size", &g_TimeBoxAverageSize, 10.f, 200.f);
    ImGui::SliderInt("TimeBox Number", &g_TimeBoxNumber, 0, 10000);
    ImGui::Text("Rendered Count %d", count);

    //static float average = 1.f;
    //ImGui::SliderFloat("Average", &average, 0.f, 3.f);
    ImGui::SliderFloat("Standard Deviation", &stddev, 0.0f, 0.9f);
    if (ImGui::Button("Simulate"))
    {
        generate = true;
    }
    if (ImGui::Button("Step"))
    {
        step = true;
    }
    ImGui::Checkbox("Auto Step", &autoStep);

    for (auto& j : simulator->get_queue())
    {
        ImGui::BulletText("Job: %d, duration = %f", j.frame_index, j.duration);
    }
    ImGui::End();
}
