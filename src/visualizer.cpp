#include "visualizer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <random>
#include <memory>
#include <assert.h>
#include <chrono>

namespace
{

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

SimulationOption g_SimOption;
SimulationOption g_LastSimOption;

ControlOption g_ControlOption;

DisplayOption g_DisplayOption;

template<class T, size_t N>
constexpr size_t array_size(T(&)[N]) { return N; }

ImVec2 TimeBoxP0(const TimeBox& timebox)
{
    return ImVec2(timebox.start_time, timebox.core_index * g_DisplayOption.Height);
}
ImVec2 TimeBoxP1(const TimeBox& timebox)
{
    return TimeBoxP0(timebox) + ImVec2(timebox.end_time - timebox.start_time, g_DisplayOption.Height);
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

    auto p0 = (win + origin + TimeBoxP0(timebox));
    auto p1 = (win + origin + TimeBoxP1(timebox));
    p0.x *= g_DisplayOption.Scale;
    p1.x *= g_DisplayOption.Scale;
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


    PrepareJob::PrepareJob(Simulator* sim, std::shared_ptr<Frame> f)
        : Job(sim, f)
    {
        m_duration = 100.f * m_simulator->generate();
    }

    float PrepareJob::duration() const
    {
        return m_duration;
    }
    const char* PrepareJob::name() const 
    {
        return "Prepare";
    }

    void PrepareJob::exec() 
    {
        auto sim = m_simulator;
        sim->push_job(std::make_shared<RenderJob>(m_simulator, m_frame));
        auto f = sim->start_frame();
        m_simulator->push_job(std::make_shared<PrepareJob>(m_simulator, f));
    }

    RenderJob::RenderJob(Simulator* sim, std::shared_ptr<Frame> f)
        : Job(sim, f)
    {
        m_duration = 100.f * m_simulator->generate();
    }

      float RenderJob::duration() const 
     {
         return m_duration;
     }

      const char* RenderJob::name() const 
     {
         return "Render";
     }

     void RenderJob::exec() 
     {
        m_simulator->push_frame(m_frame);
    }


    Simulator::Simulator(int core, int frame_pool, int seed, float stddev)
        : m_core_count(core)
        , m_frame_pool_size(frame_pool)
        , m_frame_count(0)
        , m_generator(seed)
        , m_distribution((1.f - stddev), (1.f + stddev))
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
            m_frame_pool.push_back(std::make_shared<Frame>());
        }

        auto f = start_frame();
        push_job(std::make_shared<PrepareJob>(this, f));
    }

    void Simulator::step()
    {
        if (m_job_queue.empty())
        {
            // Find the latest core which has a job to execute
            Core* min_core = nullptr;
            for (auto& c : m_cores)
            {
                if (c.current_job)
                {
                    if (min_core == nullptr)
                    {
                        min_core = &c;
                    }
                    else if
                    (c.time < min_core->time)
                    {
                        min_core = &c;
                    }
                }
            }
            
            // If assert break, it is a deadlock
            assert(min_core != nullptr);

            min_core->current_job->exec();
            min_core->current_job = nullptr;
            
            // Advance the time of all the core which has no job to execute
            // to be equal to min_core.time
            for (auto& c : m_cores)
            {
                if (!c.current_job)
                {
                    c.time = min_core->time;
                }
            }
        }
        else
        {
            auto* min_core = &m_cores[0];
            for (auto& c : m_cores)
            {
                if (c.time < min_core->time)
                {
                    min_core = &c;
                }
            }

            if (min_core->current_job)
            {
                min_core->current_job->exec();
                min_core->current_job = nullptr;
            }

            std::shared_ptr<Job> j = pop_job();

            m_timeboxes.emplace_back(min_core->index, j->frame_index(), min_core->time, min_core->time + j->duration(), j->name(), g_Colors[j->frame_index() % array_size(g_Colors)]);
            m_max = ImMax(m_max, TimeBoxP1(m_timeboxes.back()));
            min_core->time += j->duration();
            min_core->current_job = j;
        }
    }

    float Simulator::generate()
    {
        return m_distribution(m_generator);
    }

    std::shared_ptr<Frame> Simulator::start_frame()
    {
        assert(!m_frame_pool.empty());
        std::shared_ptr<Frame> f = m_frame_pool.back();
        m_frame_pool.pop_back();

        f->frame_index = m_frame_count;
        m_frame_count += 1;
        return f;
    }

    void Simulator::push_job(std::shared_ptr<Job> j)
    {
        m_job_queue.push_back(j);
    }

    void Simulator::push_frame(std::shared_ptr<Frame> f)
    {
        m_frame_pool.push_back(f);
    }

    std::shared_ptr<Job> Simulator::pop_job()
    {
        assert(!m_job_queue.empty());
        std::shared_ptr<Job> j = m_job_queue.front();
        m_job_queue.pop_front();
        return j;
    }

void PushDisabled(bool disabled)
{
    if (disabled)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
}    
    
void PopDisabled(bool disabled)
{
    if (disabled)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}


void DrawVisualizer()
{
    static std::unique_ptr<Simulator> simulator;

    if (g_ControlOption.Restart || g_ControlOption.AutoRestart && (g_LastSimOption != g_SimOption))
    {
        int seed = g_SimOption.Seed;
        if (g_SimOption.AutoSeed)
        {
            g_SimOption.Seed = (int) std::chrono::system_clock::now().time_since_epoch().count();
        }
        simulator = std::make_unique<Simulator>(g_SimOption.CoreNum, g_SimOption.FramePoolSize, g_SimOption.Seed, g_SimOption.Random);
    }
    g_LastSimOption = g_SimOption;

    if (g_ControlOption.Step || g_ControlOption.AutoStep)
    {
        simulator->step();
    }

    bool yes = true;
    ImGui::Begin("Timeline", &yes, ImGuiWindowFlags_HorizontalScrollbar);
    auto origin = ImGui::GetCursorPos() - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    ImU32 col = 0;

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
    if (g_DisplayOption.ShowCoreTime)
    {
        simulator->DrawCore(origin);
    }

    // Add an offset to scroll a bit more than the max of the timeline
    auto cursor = simulator->get_max() + ImVec2(100.f, 0.f);
    cursor.x *= g_DisplayOption.Scale;
    ImGui::SetCursorPos(cursor);

    ImGui::End();


    ImGui::Begin("Options", &yes);

    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Core Number", &g_SimOption.CoreNum, 1, 16);
        ImGui::SliderInt("FramePool Size", &g_SimOption.FramePoolSize, 1, 16);
        ImGui::SliderFloat("Random", &g_SimOption.Random, 0.0f, 0.9f);
        PushDisabled(!g_SimOption.AutoSeed);
        ImS32 step = 1;
        ImGui::InputScalar("Seed", ImGuiDataType_S32, &g_SimOption.Seed, &step, nullptr);
        PopDisabled(!g_SimOption.AutoSeed);
        ImGui::Checkbox("Random Seed", &g_SimOption.AutoSeed);
    }

    if (ImGui::CollapsingHeader("Control", ImGuiTreeNodeFlags_DefaultOpen))
    {

        g_ControlOption.Restart = false;
        PushDisabled(g_ControlOption.AutoRestart);
        if (ImGui::Button("Restart"))
        {
            g_ControlOption.Restart = true;
        }
        PopDisabled(g_ControlOption.AutoRestart);

        ImGui::SameLine();
        ImGui::Checkbox("Auto Restart", &g_ControlOption.AutoRestart);

        g_ControlOption.Step = false;
        PushDisabled(g_ControlOption.AutoStep);
        if (ImGui::Button("Step"))
        {
            g_ControlOption.Step = true;
        }
        PopDisabled(g_ControlOption.AutoStep);

        ImGui::SameLine();
        ImGui::Checkbox("Auto Step", &g_ControlOption.AutoStep);
    }

    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Show Core Time", &g_DisplayOption.ShowCoreTime);
        ImGui::SliderFloat("Height", &g_DisplayOption.Height, 5.f, 40.f);
        ImGui::SliderFloat("Scale", &g_DisplayOption.Scale, 1.f, 3.f);
    }

    ImGui::Separator();
    ImGui::Text("Rendered Count %d", count);
    ImGui::Text("Job Queue:");
    for (auto& j : simulator->get_queue())
    {
        ImGui::BulletText("Job: %d, %s (%f)", j->frame_index(), j->name(), j->duration());
    }

    ImGui::End();
}

void Simulator::DrawCore(ImVec2 origin)
{
    auto drawList = ImGui::GetWindowDrawList();
    auto win = ImGui::GetWindowPos();

    for (const auto& c : m_cores)
    {
        auto p0 = win + origin + ImVec2(c.time, c.index * g_DisplayOption.Height);
        p0.x *= g_DisplayOption.Scale;
        auto p1 = p0 + ImVec2(2.f, g_DisplayOption.Height);

        ImU32 color = c.current_job ? g_Red : g_White;
        drawList->AddRectFilled(p0, p1, color);
    }
}