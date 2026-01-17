#pragma once
#include <unordered_map>
#include <vector>
#include <optional>
#include <array>

#include "AsyncGate.h"
#include "IModule.h"

#include "TaskStructs.h"

class ILogger;
class ObjectProvider;
class TaskQueue;
class AsyncGate;

class PreEventContext;
class PostEventContext;

class Dispatch : public IModule {
    AIM_INJECTABLE(Dispatch)
    
    AIM_INJECT(ILogger, log)
    AIM_INJECT(ObjectProvider, objectProvider)
    AIM_INJECT(TaskQueue, taskQueue)

public:
    Dispatch() = default;
    ~Dispatch();

  private:
    std::unordered_map<int /*fnIdx*/, std::vector<std::shared_ptr<TaskDefinition>>> preTasks_;
    std::unordered_map<int /*fnIdx*/, std::vector<std::shared_ptr<TaskDefinition>>> postTasks_;
    std::unordered_map<int /*fnIdx*/, std::vector<std::shared_ptr<TaskDefinition>>> gatedTasks_;
    std::vector<std::shared_ptr<TaskDefinition>> executeTasks_;

    std::unordered_map<HookKey, int, HookKeyHash> hookRefs_;

    std::unordered_map<int, std::vector<std::shared_ptr<TaskDefinition>>> nextTickPendingInsertion_;
    std::array<int, 5> recentlyActiveFnIndices_{};
    size_t writeIndex = 0;

    std::atomic<int32_t> counter_{420};

public:
//    auto getReadyGate() { return readyGate_->gate(); }
//    auto getShutdownGate() -> AsyncGate* { return shutdownGate_->gate(); }
    
    int32_t getNextID();
    void registerTask(std::shared_ptr<TaskDefinition>& task);
    void releaseTask(std::shared_ptr<TaskDefinition>& task);
    
    void shutdown();
    void clearTasks();
    bool runTasksForPhase(
        std::unordered_map<int, std::vector<std::
        shared_ptr<TaskDefinition>>>& taskMap,
        int fnIndex, InvocationContext& ctx,
        bool checkBlocking);

    bool dispatchPre(int fnIndex, InvocationContext& ctx);
    void dispatchPost(int fnIndex, InvocationContext& ctx);
    void dispatchExecute(InvocationContext& ctx);
    void evaluateTask(std::shared_ptr<TaskDefinition>& taskPtr, InvocationContext& ctx);
    auto bindContext(std::function<void(InvocationContext&)> f, InvocationContext& ctx);
    void executeWithContext(std::function<void(InvocationContext&)> f, InvocationContext& ctx);

    void dispatchGated(int fnId, InvocationContext& context);

    std::atomic<int> activeDispatches_ = 0;
    void acquire() { activeDispatches_++; }
    void release() { activeDispatches_--; }
    int getActiveCount() const { return activeDispatches_.load(); }
};