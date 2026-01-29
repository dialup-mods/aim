#pragma once
#include <memory>

#include "AsyncGate.h"
#include "Dispatch.h"
#include "IRuntime.h"
#include "MockLogger.h"
#include "MutexGuard.h"
#include "PatchManager.h"
#include "ProcessInternal.h"
#include "TaskQueue.h"

struct ProcessInternalFixture {
    std::shared_ptr<MockLogger> logger = std::make_shared<MockLogger>();

    std::shared_ptr<Dispatch> dispatch =
        std::make_shared<Dispatch>();
    std::shared_ptr<TaskQueue> taskQueue =
        std::make_shared<TaskQueue>();
    std::shared_ptr<PatchManager> patchManager =
        std::make_shared<PatchManager>();
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

    ProcessInternal processInternal;

    ProcessInternalFixture() {
        processInternal.__inject_log(logger);
        processInternal.__inject_dispatch(dispatch);
        processInternal.__inject_taskQueue(taskQueue);
        processInternal.__inject_patchManager(patchManager);
        processInternal.__inject_appliedGate(appliedGate);
        processInternal.__inject_removedGate(removedGate);
        processInternal.__inject_readyFence(readyFence);
        processInternal.__inject_teardownFence(teardownFence);
        processInternal.__inject_mutex(mutex);

    }

    ~ProcessInternalFixture() = default;

};