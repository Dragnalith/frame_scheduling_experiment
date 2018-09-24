#pragma once

#include <stdint.h>

#include <string>
#include <sstream>
#include <deque>
#include <vector>
#include <memory>
#include <atomic>
#include <random>
#include <unordered_set>

#include "imgui.h"
#include "imgui_internal.h"

struct SimulationOption
{
    int CoreNum = 2;
    int FramePoolSize = 2;
    float Random = 0.f;
    int Seed = 0;
    bool AutoSeed = false;

    bool operator==(const SimulationOption& other)
    {
        return CoreNum == other.CoreNum
            && FramePoolSize == other.FramePoolSize
            && Random == other.Random
            && Seed == other.Seed
            && AutoSeed == other.AutoSeed;
    }

    bool operator!=(const SimulationOption& other)
    {
        return ! (*this == other);
    }
};

struct FramePattern;

struct Preset
{
    virtual const char* name() const = 0;
    virtual std::shared_ptr<FramePattern> pattern() const = 0;
    virtual SimulationOption option() const = 0;
};

struct ControlOption
{
    bool Keep = true;
    bool Restart = true;
    bool AutoStep = true;
    bool Step;
};

struct DisplayOption
{
    bool ShowFrameTime = true;
    bool ShowFrameRate = true;
    bool ShowCoreTime = true;
    bool ShowFramePool = true;
    float Height = 20.f;
    float Scale = 1.f;
};

class Job;

struct Core
{
    int index;
    float time = 0.f;
    std::shared_ptr<Job> current_job;

    bool try_exec();
};

enum class TimeBoxType
{
    Normal,
    In,
    Out,
    FrameTime,
    FrameRate,
};

struct TimeBox
{
    TimeBox(int index, int frame, float start, float end, const std::string& n, uint32_t c, TimeBoxType t)
        : core_index(index)
        , start_time(start)
        , end_time(end)
        , name(n)
        , color(c)
        , type(t)
    {
        if (frame >= 0)
        {
            std::stringstream s;
            s << frame << "-";
            frame_index = s.str();
        }
    }

    int core_index;
    std::string frame_index;
    std::string name;
    uint32_t color;
    TimeBoxType type;

    float start() const;
    float end() const;
private:
    float end_time;
    float start_time;
};

class Simulator;

struct Frame
{
    int frame_index = -1;
    float start_time = -1.f;
    float end_time = -1.f;

    std::unordered_set<uint32_t> finished_node;
};

class Job
{
public:
    virtual ~Job() = default;
    Job(Simulator* sim, std::shared_ptr<Frame> f)
        : m_simulator(sim)
        , m_frame(f)
    {}

    int frame_index() const { return m_frame->frame_index; }

    virtual float duration() const = 0;
    virtual bool try_exec(float time) = 0;
    virtual bool is_ready() const = 0;
    virtual void before_schedule(float time) = 0;
    virtual const char* name() const = 0;
    virtual bool is_first() const = 0;
    virtual bool is_release() const = 0;
protected:
    Simulator* m_simulator;
    std::shared_ptr<Frame> m_frame;
};

struct FramePattern;

class Simulator
{
public:
    Simulator(std::shared_ptr<FramePattern> pattern, int core, int frame_pool, int seed, float stddev);

    void step();
    void draw();

    const std::vector<TimeBox>& get_timeboxes() const { return m_timeboxes; }
    const ImVec2& get_max() const { return m_max; }
    const std::deque<std::shared_ptr<Job>>& get_queue() const { return m_job_queue; }

    bool frame_pool_empty() const { return m_frame_available.empty(); }

    float generate();

    std::shared_ptr<Frame> start_frame(float time);

    void push_job(std::shared_ptr<Job> j);

    void push_frame(std::shared_ptr<Frame> f);

    std::shared_ptr<Frame> get_frame(int index);

    std::shared_ptr<Job> pop_job();

    void DrawCore(ImVec2 origin);

    bool has_ready_job() {

        for (auto j : m_job_queue)
        {
            if (j->is_ready())
            {
                return true;
            }
        }

        return false;
    }

    int visible_timebox_count() const { return m_diplayed_timebox; }

    void freeze(const std::string&name);

private:
    int m_core_count;
    int m_frame_pool_size;
    int m_frame_count;

    ImVec2 m_max;

    std::shared_ptr<FramePattern> m_pattern;

    float m_last_push_time = -1.f;
    int m_diplayed_timebox = 0;
    bool m_frozen = false;

    std::string m_name = "Timeline";

    std::mt19937 m_generator;
    std::uniform_real_distribution<float> m_distribution;

    std::vector<Core> m_cores;
    std::vector<std::shared_ptr<Frame>> m_frames;
    std::deque<std::shared_ptr<Job>> m_job_queue;
    std::deque<std::shared_ptr<Frame>> m_frame_available;
    std::vector<TimeBox> m_timeboxes;
};

struct JobType;

class PatternJob : public Job
{
public:
    PatternJob(std::shared_ptr<JobType> type, Simulator* sim, std::shared_ptr<Frame> f, std::shared_ptr<int> counter = nullptr);

    virtual float duration() const override;
    virtual const char* name() const override;
    virtual bool is_first() const override;
    virtual bool is_release() const override;
    virtual bool is_ready() const override;

    virtual void before_schedule(float time) override;
    virtual bool try_exec(float time) override;


private:
    std::shared_ptr<JobType> m_type;
    float m_duration;

    std::shared_ptr<int> m_counter;
};


void DrawVisualizer();

const Preset& get_default_preset();