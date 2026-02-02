#pragma once
#include "IModule.h"

struct TaskDefinition;

struct IProcessEvent : IModule {
    AIM_INJECTABLE(IProcessEvent)

    virtual void registerTask(std::shared_ptr<TaskDefinition> def) = 0;
    virtual void releaseTask(std::shared_ptr<TaskDefinition> def) = 0;
    virtual void clearTasks() = 0;
};