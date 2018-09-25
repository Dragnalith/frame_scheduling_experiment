#pragma once

#include "NodeEditor.h"

#include <string>
#include <memory>

#include <unordered_map>
#include <unordered_set>

void DrawNodeEditor(bool* opened);

void OtherNodeEditor(bool *opened);



struct JobType
{
    JobType();

    bool is_first = false;
    bool generate_next = false;
    bool generation_priority = false;
    bool wait_previous = false;
    bool release_frame = false;
    int  count = 1;
    char name[255];
    std::shared_ptr<JobType> next = nullptr;
    float duration = 100.f;

    uint32_t nid;
    uint32_t lid;
    uint32_t iid;
    uint32_t oid;
};

struct FramePattern
{
    void add(std::shared_ptr<JobType> type)
    {
        Types.emplace(type->nid, type);
        Id2Node.emplace(type->iid, type->nid);
        Id2Node.emplace(type->oid, type->nid);
        Id2Node.emplace(type->lid, type->nid);
    }

    std::shared_ptr<JobType> first;
    std::unordered_map<uint32_t, std::shared_ptr<JobType>> Types;
    std::unordered_map<uint32_t, uint32_t> Id2Node;
};



struct ID
{
    ID();

    uint32_t node;
    uint32_t link;
    uint32_t in;
    uint32_t out;
};

struct FrameStage {
    FrameStage(const char* name, float w, int split, bool sync);

    char name[101];
    float weight;
    int split_count;
    bool one_frame_at_a_time;
    bool create_has_priority = false;

    ID id;
};

struct FrameFlow {
    FrameFlow(const char* n);

    std::vector<std::shared_ptr<FrameStage>> stages;
    
    float duration = 500.f;
    int start_next_frame_stage = 0;

    float compute_critical_path_time();

    float stage_duration(int index);

    char name[201];
};

struct Frame;
class Simulator;
void create_job(std::shared_ptr<FrameFlow> flow, int index, Simulator* sim, std::shared_ptr<Frame> frame);

void DrawFrameEditor(std::shared_ptr<FrameFlow> frame_flow);
