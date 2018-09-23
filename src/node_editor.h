#pragma once

#include "NodeEditor.h"

#include <string>
#include <memory>

void ShowExampleAppCustomNodeGraph(bool* opened, ax::NodeEditor::EditorContext* context);

void OtherNodeEditor(bool *opened);


struct JobType
{
    JobType();

    bool isFirst = false;
    bool generateNextFrame = false;
    bool releaseFrame = false;
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
    std::shared_ptr<JobType> first;
};