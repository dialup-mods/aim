#include <Windows.h>
#include "TaskQueue.h"

#include "AsyncGate.h"
#include "Exception.h"
#include "ILogger.h"

#include <mutex>

TaskQueue::~TaskQueue() {
    log_->debug("TaskQueue unloaded");
}

void
TaskQueue::init() {
    log_->debug("TaskQueue initializing...");
}

void
TaskQueue::queueFunction(std::function<void()> func) {
    std::lock_guard lock(tasksMutex_);
    fnQueue_.push_back(std::move(func));
    printf("[QF] queueFunction() called from thread %p\n", GetCurrentThread());
}

void
TaskQueue::shutdown() {
    std::lock_guard lock(tasksMutex_);
    fnQueue_.clear();
    nextQueue_.clear();

    fnQueue_.shrink_to_fit();
    nextQueue_.shrink_to_fit();
}

void TaskQueue::processFunctions() {
    {
        std::lock_guard lock(tasksMutex_);
        if (!nextQueue_.empty()) {
            fnQueue_.insert(fnQueue_.end(),
                            std::make_move_iterator(nextQueue_.begin()),
                            std::make_move_iterator(nextQueue_.end()));
            nextQueue_.clear();
        }
    }

    std::vector<std::function<void()>> toRun;
    {
        std::lock_guard lock(tasksMutex_);
        std::swap(toRun, fnQueue_);
    }

    for (auto& func : toRun) {
        printf("[PF] processFunctions() on thread %p\n", GetCurrentThread());
        safe::makeSEHSafe([&]() {
            func();
        });
    }
}