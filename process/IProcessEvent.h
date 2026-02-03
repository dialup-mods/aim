#pragma once
#include "IModule.h"

struct TaskDefinition;

struct IProcessEvent : IModule {
    AIM_INJECTABLE(IProcessEvent)

    virtual void enableTask(std::shared_ptr<TaskDefinition> def) = 0;
    virtual void disableTask(std::shared_ptr<TaskDefinition> def) = 0;
};