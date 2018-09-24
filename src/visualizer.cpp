#include "visualizer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "app.h"

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

template<class T, size_t N>
constexpr size_t array_size(T(&)[N]) { return N; }

ImVec2 TimeBoxP0(const TimeBox& timebox)
{
    return ImVec2(timebox.start_time, timebox.core_index * App::get().DisplayOption.Height);
}
ImVec2 TimeBoxP1(const TimeBox& timebox)
{
    return TimeBoxP0(timebox) + ImVec2(timebox.end_time - timebox.start_time, App::get().DisplayOption.Height);
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
    bool is_frame = timebox.frame_index.size() == 0;

    if (is_frame && !App::get().DisplayOption.ShowFrameTime)
    {
        return;
    }

    auto drawList = ImGui::GetWindowDrawList();
    auto win = ImGui::GetWindowPos();

    auto p0 = (win + origin + TimeBoxP0(timebox));
    auto p1 = (win + origin + TimeBoxP1(timebox));
    p0.x *= App::get().DisplayOption.Scale;
    p1.x *= App::get().DisplayOption.Scale;
    if (is_frame)
    {
        p0.y += 20.f;
        p1.y += (20.f - App::get().DisplayOption.Height * 0.5f);
    }
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

    bool PrepareJob::try_exec(float time)
    {
        if (!m_simulator->frame_pool_empty())
        {
            m_frame->start_time = time - m_duration;
            m_simulator->push_job(std::make_shared<RenderJob>(m_simulator, m_frame));
            auto f = m_simulator->start_frame();
            m_simulator->push_job(std::make_shared<PrepareJob>(m_simulator, f));
            return true;
        }
        else
        {
            return false;
        }
    }

    PatternJob::PatternJob(std::shared_ptr<JobType> type, Simulator* sim, std::shared_ptr<Frame> f)
        : Job(sim, f)
        , m_type(type)
    {
        m_duration = m_type->duration * m_simulator->generate();

    }

    float PatternJob::duration() const
    {
        return m_duration;
    }
    const char* PatternJob::name() const
    {
        return m_type->name;
    }

    bool PatternJob::try_exec(float time)
    {
        bool cond = !m_type->generateNextFrame || m_type->generateNextFrame && !m_simulator->frame_pool_empty();

        if (cond)
        {
            if (m_type->isFirst)
            {
                m_frame->start_time = time - m_duration;
            }
            if (m_type->next)
            {
                m_simulator->push_job(std::make_shared<PatternJob>(m_type->next, m_simulator, m_frame));
            }

            if (m_type->generateNextFrame)
            {
                auto f = m_simulator->start_frame();
                m_simulator->push_job(std::make_shared<PatternJob>(App::get().Pattern.first, m_simulator, f));
            }

            if (m_type->releaseFrame)
            {
                m_frame->end_time = time;
                m_simulator->push_frame(m_frame);
            }
            return true;
        }
        else
        {
            return false;
        }
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

     bool RenderJob::try_exec(float time) 
     {
         m_frame->end_time = time;
         m_simulator->push_frame(m_frame);
         return true;
     }

     bool Core::try_exec()
     {
         if (current_job && current_job->try_exec(time))
         {
             current_job = nullptr;
             return true;
         }

         return false;
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
        push_job(std::make_shared<PatternJob>(App::get().Pattern.first, this, f));
    }

    void Simulator::draw()
    {
        bool yes = true;
        ImGui::SetNextWindowSize(ImVec2(1500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 600), ImGuiCond_FirstUseEver);


        ImGui::Begin(m_name.c_str(), &yes, ImGuiWindowFlags_HorizontalScrollbar);

        auto origin = ImGui::GetCursorPos() - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
        ImU32 col = 0;

        float windowMin = ImGui::GetScrollX();
        float windowMax = windowMin + ImGui::GetWindowSize().x;
        m_diplayed_timebox = 0;
        for (const auto& t : get_timeboxes())
        {
            if (t.start_time <= windowMax && t.end_time >= windowMin)
            {
                DrawTimeBox(origin, t);
                m_diplayed_timebox += 1;
            }
        }
        if (App::get().DisplayOption.ShowCoreTime)
        {
            DrawCore(origin);
        }

        // Add an offset to scroll a bit more than the max of the timeline
        auto cursor = get_max() + ImVec2(100.f, 0.f);
        cursor.x *= App::get().DisplayOption.Scale;
        ImGui::SetCursorPos(cursor);

        ImGui::End();
    }

    void Simulator::step()
    {
        assert(!m_frozen);

        std::sort(m_cores.begin(), m_cores.end(), [](auto& a, auto& b) -> bool {
            return a.time < b.time || (a.time == b.time && a.index < b.index);
        });

        // Find the first core available
        Core* latest_available_core = nullptr;
        for (int i = 0; i < m_cores.size(); i++)
        {
            if (!m_cores[i].current_job)
            {
                latest_available_core = &m_cores[i];
                break;
            }
        }

        if (latest_available_core == nullptr || m_job_queue.empty())
        {
            std::sort(m_cores.begin(), m_cores.end(), [](auto& a, auto& b) -> bool{
                return a.time < b.time || (a.time == b.time && a.index < b.index);
            });

            Core* latest_busy_core = nullptr;
            for (int i = 0; i < m_cores.size(); i++)
            {
                if (m_cores[i].try_exec())
                {
                    latest_busy_core = &m_cores[i];
                    break;
                }
            }

            assert(latest_busy_core != nullptr);
            
            // Advance the time of all the core which has no job to execute
            // to be equal to min_core.time
            for (int i = 0; latest_busy_core != &m_cores[i]; i++)
            {
                m_cores[i].time = latest_busy_core->time;
            }
        }
        else
        {
            assert(latest_available_core != nullptr);

            std::shared_ptr<Job> j = pop_job();

            m_timeboxes.emplace_back(latest_available_core->index, j->frame_index(), latest_available_core->time, latest_available_core->time + j->duration(), j->name(), g_Colors[j->frame_index() % array_size(g_Colors)]);
            m_max = ImMax(m_max, TimeBoxP1(m_timeboxes.back()));
            latest_available_core->time += j->duration();
            latest_available_core->current_job = j;
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
        int index = m_core_count + f->frame_index % m_frame_pool_size;

        std::stringstream s;
        s << f->end_time - f->start_time;
        m_timeboxes.emplace_back(index, -1, f->start_time, f->end_time, s.str(), g_Colors[f->frame_index % array_size(g_Colors)]);

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

static int g_FreezeCount = 0;

void Simulator::freeze(const std::string& name)
{
    m_frozen = true;
    std::stringstream s;
    s << name << " " << g_FreezeCount;
    g_FreezeCount += 1;
    m_name = s.str();
}

void DrawVisualizer()
{
    if (App::get().ControlOption.Restart)
    {
        if (App::get().CurrentSimulation)
        {
            App::get().CurrentSimulation->freeze(App::get().Pattern.first->name);
            App::get().FrozenSimulations.insert(App::get().CurrentSimulation);
        }

        int seed = App::get().SimOption.Seed;
        if (App::get().SimOption.AutoSeed)
        {
            App::get().SimOption.Seed = (int) std::chrono::system_clock::now().time_since_epoch().count();
        }
        App::get().CurrentSimulation = std::make_shared<Simulator>(App::get().SimOption.CoreNum, App::get().SimOption.FramePoolSize, App::get().SimOption.Seed, App::get().SimOption.Random);
    }
    App::get().LastSimOption = App::get().SimOption;

    if (App::get().CurrentSimulation && App::get().ControlOption.Step || App::get().ControlOption.AutoStep)
    {
        App::get().CurrentSimulation->step();
    }

    App::get().CurrentSimulation->draw();

    for (auto& s : App::get().FrozenSimulations)
    {
        s->draw();
    }


    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Options");

    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Core Number", &App::get().SimOption.CoreNum, 1, 16);
        ImGui::SliderInt("FramePool Size", &App::get().SimOption.FramePoolSize, 1, 16);
        ImGui::SliderFloat("Random", &App::get().SimOption.Random, 0.0f, 0.9f);
        PushDisabled(!App::get().SimOption.AutoSeed);
        ImS32 step = 1;
        ImGui::InputScalar("Seed", ImGuiDataType_S32, &App::get().SimOption.Seed, &step, nullptr);
        PopDisabled(!App::get().SimOption.AutoSeed);
        ImGui::Checkbox("Random Seed", &App::get().SimOption.AutoSeed);
    }

    if (ImGui::CollapsingHeader("Control", ImGuiTreeNodeFlags_DefaultOpen))
    {

        App::get().ControlOption.Restart = false;
        if (ImGui::Button("Restart"))
        {
            App::get().ControlOption.Restart = true;
            App::get().ControlOption.Keep = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Restart & Keep"))
        {
            App::get().ControlOption.Restart = true;
            App::get().ControlOption.Keep = true;
        }

        App::get().ControlOption.Step = false;
        PushDisabled(App::get().ControlOption.AutoStep);
        if (ImGui::Button("Step"))
        {
            App::get().ControlOption.Step = true;
        }
        PopDisabled(App::get().ControlOption.AutoStep);

        ImGui::SameLine();
        ImGui::Checkbox("Auto Step", &App::get().ControlOption.AutoStep);
    }

    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Show FrameTime", &App::get().DisplayOption.ShowFrameTime);
        ImGui::Checkbox("Show Core Time", &App::get().DisplayOption.ShowCoreTime);
        ImGui::SliderFloat("Height", &App::get().DisplayOption.Height, 5.f, 40.f);
        ImGui::SliderFloat("Scale", &App::get().DisplayOption.Scale, 1.f, 3.f);
    }

    ImGui::Separator();
    ImGui::Text("Rendered Count %d", App::get().CurrentSimulation->visible_timebox_count());
    ImGui::Text("Job Queue:");
    for (auto& j : App::get().CurrentSimulation->get_queue())
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
        auto p0 = win + origin + ImVec2(c.time, c.index * App::get().DisplayOption.Height);
        p0.x *= App::get().DisplayOption.Scale;
        auto p1 = p0 + ImVec2(2.f, App::get().DisplayOption.Height);

        ImU32 color = c.current_job ? g_Red : g_White;
        drawList->AddRectFilled(p0, p1, color);
    }
}