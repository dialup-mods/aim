#pragma once
#include <functional>
#include <mutex>
#include <memory>

#include "IModule.h"

class ILogger;
class AsyncGate;

class TaskQueue : public IModule {
    AIM_INJECTABLE(TaskQueue)

    AIM_INJECT(ILogger, log)
    
  public:
    TaskQueue() = default;
    ~TaskQueue();

    void init();
    std::string getName();
    //void __inject_name(std::string name);

    void queueFunction(std::function<void()> task);
    void shutdown();
    void processFunctions();
    
private:
    std::string name_;
    
    std::vector<std::function<void()>> fnQueue_;
    std::vector<std::function<void()>> nextQueue_;
    std::mutex tasksMutex_;
};