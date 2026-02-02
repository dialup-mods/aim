#pragma once
#include "IModule.h"

struct TaskDefinition;

class ICallFunction : public IModule {
    AIM_INJECTABLE(ICallFunction)

    virtual void registerTask(std::shared_ptr<TaskDefinition> def) = 0;
    virtual void releaseTask(std::shared_ptr<TaskDefinition> def) = 0;
    virtual void clearTasks() = 0;
};
