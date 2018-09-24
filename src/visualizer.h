#pragma once

#include <stdint.h>

#include <string>
#include <sstream>
#include <deque>
#include <vector>
#include <memory>
#include <random>

#include "imgui.h"
#include "imgui_internal.h"

struct SimulationOption
{
    int CoreNum = 2;
    int FramePoolSize = 2;
    float Random = 0.f;
    int Seed = 0;
    bool AutoSeed = true;

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

struct ControlOption
{
    bool Keep = true;
    bool Restart = true;
    bool AutoStep = false;
    bool Step;
};

struct DisplayOption
{
    bool ShowFrameTime = true;
    bool ShowCoreTime = true;
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

struct TimeBox
{
    TimeBox(int index, int frame, float start, float end, const std::string& n, uint32_t c = 0xffffffff)
        : core_index(index)
        , start_time(start)
        , end_time(end)
        , name(n)
        , color(c)
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
    float start_time;
    float end_time;
    std::string name;
    uint32_t color;
};

class Simulator;

struct Frame
{
    int frame_index;
    float start_time;
    float end_time;
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
    virtual const char* name() const = 0;
protected:
    Simulator* m_simulator;
    std::shared_ptr<Frame> m_frame;
};

class Simulator
{
public:
    Simulator(int core, int frame_pool, int seed, float stddev);

    void step();

    const std::vector<TimeBox>& get_timeboxes() const { return m_timeboxes; }
    const ImVec2& get_max() const { return m_max; }
    const std::deque<std::shared_ptr<Job>>& get_queue() const { return m_job_queue; }

    bool frame_pool_empty() const { return m_frame_pool.empty(); }

    float generate();

    std::shared_ptr<Frame> start_frame();

    void push_job(std::shared_ptr<Job> j);

    void push_frame(std::shared_ptr<Frame> f);

    std::shared_ptr<Job> pop_job();

    void DrawCore(ImVec2 origin);


private:
    int m_core_count;
    int m_frame_pool_size;
    int m_frame_count;
    ImVec2 m_max;

    std::mt19937 m_generator;
    std::uniform_real_distribution<float> m_distribution;

    std::vector<Core> m_cores;
    std::deque<std::shared_ptr<Job>> m_job_queue;
    std::deque<std::shared_ptr<Frame>> m_frame_pool;
    std::vector<TimeBox> m_timeboxes;
};

class PrepareJob : public Job
{
public:
    PrepareJob(Simulator* sim, std::shared_ptr<Frame> f);

    virtual float duration() const override;
    virtual const char* name() const override;

    virtual bool try_exec(float time) override;

private:
    float m_duration;
};

struct JobType;

class PatternJob : public Job
{
public:
    PatternJob(std::shared_ptr<JobType> type, Simulator* sim, std::shared_ptr<Frame> f);

    virtual float duration() const override;
    virtual const char* name() const override;

    virtual bool try_exec(float time) override;

private:
    std::shared_ptr<JobType> m_type;
    float m_duration;
};

class RenderJob : public Job
{
public:
    RenderJob(Simulator* sim, std::shared_ptr<Frame> f);

    virtual float duration() const override;

    virtual const char* name() const override;

    virtual bool try_exec(float time) override;

private:
    float m_duration;
};


void DrawVisualizer();