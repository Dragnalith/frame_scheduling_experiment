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
ImU32 g_DarkGrey = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
ImU32 g_Black = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));


class FullSyncPreset : public Preset
{
    virtual const char* name() const {
        return "0/ Full Sync";
    }

    virtual std::shared_ptr<FramePattern> pattern() const {
        auto& app = App::get();
        auto pos = ImVec2(50.f, 50.f);
        float offset = 300.f;
        auto pattern = std::make_shared<FramePattern>();

        auto prepare = create_job_type("Prepare", 150.f, true, false, false);
        pattern->add(prepare);
        ed::SetNodePosition(prepare->nid, pos);
        pos.x += offset;

        auto render = create_job_type("Render", 250.f, false, true, true);
        pattern->add(render);
        prepare->next = render;

        ed::SetNodePosition(render->nid, pos);
        pos.x += offset;

        pattern->first = prepare;

        return pattern;
    }

    virtual SimulationOption option() const {
        return SimulationOption();
    };
};

class RenderAsyncPreset : public Preset
{
    virtual const char* name() const {
        return "1/ Render Async";
    }

    virtual std::shared_ptr<FramePattern> pattern() const {
        auto& app = App::get();
        auto pos = ImVec2(50.f, 50.f);
        float offset = 300.f;
        auto pattern = std::make_shared<FramePattern>();

        auto prepare = create_job_type("Prepare", 150.f, true, true, false);
        pattern->add(prepare);
        ed::SetNodePosition(prepare->nid, pos);
        pos.x += offset;

        auto render = create_job_type("Render", 200.f, false, false, false);
        pattern->add(render);
        prepare->next = render;

        ed::SetNodePosition(render->nid, pos);
        pos.x += offset;

        auto kick = create_job_type("Kick", 50.f, false, false, true);
        pattern->add(kick);
        render->next = kick;
        ed::SetNodePosition(kick->nid, pos);
        pos.x += offset;

        pattern->first = prepare;

        return pattern;
    }

    virtual SimulationOption option() const {
        return SimulationOption();
    };
};

std::unique_ptr<Preset> g_Presets[] = {
    std::make_unique<FullSyncPreset>(),
    std::make_unique<RenderAsyncPreset>()
};



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

ImU32 ScaleColor(ImU32 color, float ratio)
{
    auto c = ImColor(color);
    if (ratio > 0)
    {
        c.Value.x += (1.f - c.Value.x) * ratio;
        c.Value.y += (1.f - c.Value.y) * ratio;
        c.Value.z += (1.f - c.Value.z) * ratio;
    }
    else
    {
        c.Value.x += c.Value.x * ratio;
        c.Value.y += c.Value.y * ratio;
        c.Value.z += c.Value.z * ratio;
    }

    return ImU32(c);
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
    bool is_frame_time = timebox.type == TimeBoxType::FrameTime;
    bool is_frame_rate = timebox.type == TimeBoxType::FrameRate;

    if (is_frame_time && !App::get().DisplayOption.ShowFrameTime)
    {
        return;
    }
    if (is_frame_rate && !App::get().DisplayOption.ShowFrameRate)
    {
        return;
    }

    auto drawList = ImGui::GetWindowDrawList();
    auto win = ImGui::GetWindowPos();

    auto p0 = (win + origin + TimeBoxP0(timebox));
    auto p1 = (win + origin + TimeBoxP1(timebox));
    p0.x *= App::get().DisplayOption.Scale;
    p1.x *= App::get().DisplayOption.Scale;
    if (is_frame_time)
    {
        p0.y += 20.f;
        p1.y += (20.f - App::get().DisplayOption.Height * 0.5f);
    }
    if (is_frame_rate)
    {
        p0.y += 10.f;
        p1.y += (20.f - App::get().DisplayOption.Height * 0.7f);
    }

    if (is_frame_rate)
    {
        drawList->AddRect(p0, p1, 0xffffffff, 3.5f, ImDrawCornerFlags_All);
    }
    else
    {
        drawList->AddRectFilled(p0, p1, timebox.color, 3.5f, ImDrawCornerFlags_All);
    }
    if (timebox.type == TimeBoxType::In)
    {
        auto p2 = p1;
        p2.x -= (p1.x - p0.x) * 0.3f;
        auto newCol = GetConstrastColor(GetConstrastColor(timebox.color));
        drawList->AddRectFilledMultiColor(p0, p2, newCol, timebox.color, timebox.color, newCol);
    }
    else if (timebox.type == TimeBoxType::Out)
    {
        auto p2 = p0;
        p2.x += (p1.x - p0.x) * 0.3f;
        auto newCol = GetConstrastColor(GetConstrastColor(timebox.color));
        drawList->AddRectFilledMultiColor(p2, p1, timebox.color, newCol, newCol, timebox.color);
    }

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
    bool PatternJob::is_first() const
    {
        return m_type->is_first;
    }
    bool PatternJob::is_release() const
    {
        return m_type->release_frame;
    }

    void PatternJob::before_schedule(float time)
    {
        if (m_type->is_first)
        {
            m_frame->start_time = time;
        }
    }

    bool PatternJob::try_exec(float time)
    {
        bool cond = !m_type->generate_next || m_type->generate_next && !m_simulator->frame_pool_empty();

        if (cond)
        {
            if (m_type->next)
            {
                m_simulator->push_job(std::make_shared<PatternJob>(m_type->next, m_simulator, m_frame));
            }

            if (m_type->generate_next)
            {
                auto f = m_simulator->start_frame(time);
                m_simulator->push_job(std::make_shared<PatternJob>(App::get().Pattern->first, m_simulator, f));
            }

            if (m_type->release_frame)
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
     bool Core::try_exec()
     {
         if (current_job && current_job->try_exec(time))
         {
             current_job = nullptr;
             return true;
         }

         return false;
     }


    Simulator::Simulator(std::shared_ptr<FramePattern> pattern, int core, int frame_pool, int seed, float stddev)
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

        auto f = start_frame(0.f);
        push_job(std::make_shared<PatternJob>(pattern->first, this, f));
    }

    void Simulator::draw()
    {
        bool yes = true;
        ImGui::SetNextWindowSize(ImVec2(1500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 600), ImGuiCond_FirstUseEver);


        ImGui::Begin(m_name.c_str(), &yes, ImGuiWindowFlags_HorizontalScrollbar);

        auto coreOffset = ImVec2(50.f, 30.f);
        auto timelineOrigin = ImGui::GetCursorPos() - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()) + coreOffset;
        ImU32 col = 0;

        auto drawlist = ImGui::GetWindowDrawList();
        auto winPos = ImGui::GetWindowPos();
        auto winSize = ImGui::GetWindowSize();
        if (App::get().DisplayOption.ShowFramePool)
        {
            auto pos = winPos + ImVec2(10.f, 30.f);
            auto size = ImVec2(10.f, 10.f);
            float offset = 15.f;
            for (int i = 0; i < m_frame_pool.size(); i++)
            {
                drawlist->AddRectFilled(pos, pos + size, 0xffaaaaaa);
                pos.x += offset;
            }
        }

        auto corelineOrigin = ImGui::GetCursorPos() + winPos;
        for (int i = 0; i < m_core_count; i++)
        {
            auto p1 = corelineOrigin + coreOffset;
            p1.y += i * App::get().DisplayOption.Height;
            auto p2 = p1 + ImVec2(winSize.x, App::get().DisplayOption.Height);

            if (i % 2 == 0)
            {
                drawlist->AddRectFilled(p1, p2, 0xff0c0c0c);
            }
            else
            {
                drawlist->AddRectFilled(p1, p2, 0xff111111);
            }
        }

        float windowMin = ImGui::GetScrollX();
        float windowMax = windowMin + ImGui::GetWindowSize().x;
        m_diplayed_timebox = 0;
        for (const auto& t : get_timeboxes())
        {
            if (t.start_time <= windowMax && t.end_time >= windowMin)
            {
                DrawTimeBox(timelineOrigin, t);
                m_diplayed_timebox += 1;
            }
        }
        if (App::get().DisplayOption.ShowCoreTime)
        {
            DrawCore(timelineOrigin);
        }

        for (int i = 0; i < m_core_count; i++)
        {
            auto p1 = corelineOrigin + ImVec2(0.f, coreOffset.y);
            p1.y += i * App::get().DisplayOption.Height;
            auto p2 = p1 + ImVec2(coreOffset.x, App::get().DisplayOption.Height);

            drawlist->AddRectFilled(p1, p2, 0xff000000);
            std::stringstream s;
            s << "Core " << i;
            drawlist->AddText(p1, 0xffffffff, s.str().c_str());
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

            j->before_schedule(latest_available_core->time);
            
            auto timebox_color = g_Colors[j->frame_index() % array_size(g_Colors)];
            auto type = TimeBoxType::Normal;
            if (j->is_first())
            {
                type = TimeBoxType::In;
            }
            if (j->is_release())
            {
                type = TimeBoxType::Out;
            }
            m_timeboxes.emplace_back(latest_available_core->index, j->frame_index(), latest_available_core->time, latest_available_core->time + j->duration(), j->name(), timebox_color, type);
            m_max = ImMax(m_max, TimeBoxP1(m_timeboxes.back()));
            latest_available_core->time += j->duration();
            latest_available_core->current_job = j;
        }
    }

    float Simulator::generate()
    {
        return m_distribution(m_generator);
    }

    std::shared_ptr<Frame> Simulator::start_frame(float time)
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
        int frame_time_core_index = m_core_count + 2 + f->frame_index % m_frame_pool_size;

        if (m_last_push_time >= 0.f)
        {
            std::stringstream sss;
            sss << f->end_time - m_last_push_time;
            m_timeboxes.emplace_back(m_core_count + 1, -1, m_last_push_time, f->end_time, sss.str(), 0xff000000, TimeBoxType::FrameRate);
        }


        std::stringstream s;
        s << f->end_time - f->start_time;
        m_timeboxes.emplace_back(frame_time_core_index, -1, f->start_time, f->end_time, s.str(), g_Colors[f->frame_index % array_size(g_Colors)], TimeBoxType::FrameTime);

        m_frame_pool.push_back(f);
        m_last_push_time = f->end_time;
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
    s << name << " " << g_FreezeCount << " (Core = " << m_core_count << ", Frame Pool = " << m_frame_pool_size << ")";
    g_FreezeCount += 1;
    m_name = s.str();
}

void DrawVisualizer()
{
    auto& app = App::get();

    if (App::get().ControlOption.Restart)
    {
        if (App::get().CurrentSimulation && app.ControlOption.Keep)
        {
            App::get().CurrentSimulation->freeze(App::get().Pattern->first->name);
            App::get().FrozenSimulations.insert(App::get().CurrentSimulation);
        }

        int seed = App::get().SimOption.Seed;
        if (App::get().SimOption.AutoSeed)
        {
            App::get().SimOption.Seed = (int) std::chrono::system_clock::now().time_since_epoch().count();
        }
        App::get().CurrentSimulation = std::make_shared<Simulator>(app.Pattern, App::get().SimOption.CoreNum, App::get().SimOption.FramePoolSize, App::get().SimOption.Seed, App::get().SimOption.Random);
    }

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

    if (ImGui::BeginCombo("Preset", g_Presets[App::get().SelectedPreset]->name())) // The second parameter is the label previewed before opening the combo.
    {
        for (int n = 0; n < array_size(g_Presets); n++)
        {
            bool is_selected = (App::get().SelectedPreset == n);
            if (ImGui::Selectable(g_Presets[n]->name(), is_selected))
            {
                App::get().SelectedPreset = n;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Restore Preset"))
    {
        App::set_preset(*g_Presets[App::get().SelectedPreset]);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Only Frame Pattern", &App::get().OnlyFramePattern);

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
        ImGui::Checkbox("Show Frame Rate", &App::get().DisplayOption.ShowFrameRate);
        ImGui::Checkbox("Show Frame Time", &App::get().DisplayOption.ShowFrameTime);
        ImGui::Checkbox("Show Core Time", &App::get().DisplayOption.ShowCoreTime);
        ImGui::Checkbox("Show Frame Pool", &App::get().DisplayOption.ShowFramePool);
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

const Preset& get_default_preset()
{
    return *g_Presets[0];
}