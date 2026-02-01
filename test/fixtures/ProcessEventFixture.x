#pragma once
#include <memory>

#include "AsyncGate.h"
#include "Dispatch.h"
#include "IRuntime.h"
#include "MockLogger.h"
#include "MutexGuard.h"
#include "ProcessEvent.h"
#include "TaskQueue.h"

struct ProcessEventFixture {
    std::shared_ptr<MockLogger> logger = std::make_shared<MockLogger>();

    std::shared_ptr<Dispatch> dispatch =
        std::make_shared<Dispatch>();
    std::shared_ptr<TaskQueue> taskQueue =
        std::make_shared<TaskQueue>();
    std::shared_ptr<AsyncGate> appliedGate =
        std::make_shared<AsyncGate>();
    std::shared_ptr<AsyncGate> removedGate =
        std::make_shared<AsyncGate>();
    std::shared_ptr<PluginFence> readyFence =
        std::make_shared<PluginFence>();
    std::shared_ptr<PluginFence> teardownFence =
        std::make_shared<PluginFence>();
    std::shared_ptr<MutexGuard> mutex =
        std::make_shared<MutexGuard>();

    std::shared_ptr<IRuntime> runtime;

    ProcessEvent processEvent;

    ProcessEventFixture() {
        processEvent.__inject_log(logger);
        processEvent.__inject_dispatch(dispatch);
        processEvent.__inject_taskQueue(taskQueue);
        processEvent.__inject_appliedGate(appliedGate);
        processEvent.__inject_removedGate(removedGate);
        processEvent.__inject_readyFence(readyFence);
        processEvent.__inject_teardownFence(teardownFence);
        processEvent.__inject_mutex(mutex);

    }

    ~ProcessEventFixture() = default;
};