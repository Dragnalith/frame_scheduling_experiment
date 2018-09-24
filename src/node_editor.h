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