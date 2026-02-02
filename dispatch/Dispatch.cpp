#include <list>
#include <ranges>

#include "TaskBuilder.h"
#include "Dispatch.h"
#include "EventContext.h"
#include "v1/ILogger.h"
#include "ObjectQuery.h"
#include "TaskQueue.h"
#include "Runtime.h"

using r = Runtime;

Dispatch::~Dispatch() {
    log_->info("[Dispatch] unloaded");
}

int32_t Dispatch::getNextID() {
    return counter_.fetch_add(1, std::memory_order_relaxed);
}

void Dispatch::registerTask(std::shared_ptr<TaskDefinition>& task) {
    log_->debug("[Dispatch] registering task: {}", TaskBuilder::describe(*task));

    auto fn = r::ufunction::find(task->functionName);
    if (!fn) {
        log_->error("[Dispatch] Failed to find function: {}", task->functionName);
        return;
    }
    task->functionIndex = fn->ObjectInternalInteger;

    switch (task->phase) {
        case HookPhase::Pre: {
            log_->info("[Dispatch] Hook function (pre): {}", task->functionName);
            preTasks_[task->functionIndex].emplace_back(std::move(task));
            break;
        }

        case HookPhase::Post: {
            log_->debug("[Dispatch] Hook function (post): {} -> {}", task->functionName, task->functionIndex);
            postTasks_[task->functionIndex].emplace_back(std::move(task));
            break;
        }

        case HookPhase::Gated: {
            if (task->elapsedSeconds != 0.0f || task->attempt != 0) {
                log_->warn("[Dispatch] Task {}", task->name + " is not clean. Use TaskBuilder to initialize properly.");
            }

            log_->info("[Dispatch] Hook function (deferred): {}", task->functionName);
            gatedTasks_[task->functionIndex].emplace_back(std::move(task));
            break;
        }

    }

}

void Dispatch::releaseTask(std::shared_ptr<TaskDefinition>& task) {
    const HookKey key{ task->functionIndex, task->functionName, task->phase };

    switch (task->phase) {
        case HookPhase::Pre: {
            auto& list = preTasks_[task->functionIndex];
            list.erase(std::ranges::remove_if(list, [&](const std::shared_ptr<TaskDefinition>& t) {
                return t->id == task->id;
            }).begin(), list.end());
            break;
        }

        case HookPhase::Post: {
            auto& list = postTasks_[task->functionIndex];
            list.erase(std::ranges::remove_if(list, [&](const std::shared_ptr<TaskDefinition>& t) {
                return t->id == task->id;
            }).begin(), list.end());
            break;
        }
        
        case HookPhase::Gated: {
            auto& list = gatedTasks_[task->functionIndex];
            list.erase(std::ranges::remove_if(list, [&](const std::shared_ptr<TaskDefinition>& t) {
                return t->id == task->id;
            }).begin(), list.end());
            break;
        }
    }
}

void Dispatch::shutdown() {
    this->clearTasks();
    taskQueue_->shutdown();
}

void Dispatch::clearTasks() {
    preTasks_.clear();
    postTasks_.clear();
    gatedTasks_.clear();
    executeTasks_.clear();

    // fixme check that tasks are actually drained
//    shutdownGate_->setReady();
}

bool Dispatch::runTasksForPhase(
    std::unordered_map<int, std::vector<std::shared_ptr<TaskDefinition>>>& taskMap,
    int fnIndex,
    InvocationContext& ctx,
    bool checkBlocking
) {
    bool shouldBlock = false;

    if (auto it = taskMap.find(fnIndex); it != taskMap.end()) {
        printf("matched at least\n");
        auto& vec = it->second;
        // manual iterator control for thread safety and mutation awareness
        for (auto task = vec.begin(); task != vec.end(); /* manual increment */) {
            std::shared_ptr<TaskDefinition>& currentTask = *task;
            // prevent race conditions
            if (currentTask->state == TaskState::Completed || currentTask->state == TaskState::Failed) {
                ++task;
                continue;
            }
            if (checkBlocking && currentTask->isBlocking) {
                shouldBlock = true;
            }
            if (currentTask->callback) {
                printf("calling callback");
                currentTask->callback(ctx);
            }
            if (currentTask->callbackBlocking) {
                if (currentTask->callbackBlocking(ctx)) {
                    shouldBlock = true;
                }
            }
            if (currentTask->once) {
                currentTask->state = TaskState::Completed;
                task = vec.erase(task);
            } else {
                ++task;
            }
        }
        if (vec.empty()) {
            taskMap.erase(it);
        }
    }

    return shouldBlock;
}

bool Dispatch::dispatchPre(const int fnIndex, InvocationContext& ctx) {
    return runTasksForPhase(preTasks_, fnIndex, ctx, true);
}

void Dispatch::dispatchPost(const int fnIndex, InvocationContext& ctx) {
    runTasksForPhase(postTasks_, fnIndex, ctx, false);
}

void Dispatch::dispatchUnconditionally(InvocationContext& ctx) {
//    runTasksForPhase(executeTasks_, ctx, false);
}

auto Dispatch::bindContext(std::function<void(InvocationContext&)> f, InvocationContext& ctx) {
    return [f = std::move(f), &ctx]() mutable {
        f(ctx);
    };
}

void Dispatch::dispatchGated(const int fnIndex, InvocationContext& context) {
    // these run next tick, we slap them in the front so they don't run immediately after being queued
    taskQueue_->processFunctions();
    
    if (auto it = gatedTasks_.find(fnIndex); it != gatedTasks_.end()) {
        auto& vec = it->second;

        for (auto task = vec.begin(); task != vec.end(); /* manual increment */) {
            std::shared_ptr<TaskDefinition>& currentTask = *task;
            
            if (currentTask->state == TaskState::Failed || currentTask->state == TaskState::Completed) {
                continue;
            }
            
            // RL tick functions called 120x/second, so we use that to calculate how much time has passed
            constexpr float secondsPerTick = 1.0f / 120.0f;
            currentTask->elapsedSeconds += secondsPerTick;
            
            if (currentTask->preStep) {
                currentTask->preStep(context);
            }

            if (currentTask->successCondition) {
                if (currentTask->successCondition(context)) {
                    log_->logNoFmt("[Gated] Success condition met for task: " + currentTask->name);
                    
                    currentTask->state = TaskState::Completed;

                    if (currentTask->callback) {
                        currentTask->callback(context);
                    }
                    
                    if (currentTask->onSuccessCallback) {
                        taskQueue_->queueFunction(bindContext(currentTask->onSuccessCallback, context));
                    }
                }
            } else {
                if (
                    (currentTask->timeoutSeconds > 0.0f && currentTask->elapsedSeconds >= currentTask->timeoutSeconds) ||
                    (currentTask->maxAttempts > 0 && currentTask->attempt + 1 >= currentTask->maxAttempts)
                    ) {
                    log_->logNoFmt("[WARN] Task " + currentTask->name + " failed " + std::to_string(currentTask->elapsedSeconds) + "s");
                    
                    currentTask->state = TaskState::Failed;
                    
                    if (currentTask->onFailureCallback) {
                        taskQueue_->queueFunction(bindContext(currentTask->onFailureCallback, context));
                    }
                }

                currentTask->attempt++;
            }
            
            if (currentTask->state == TaskState::Completed || currentTask->state == TaskState::Failed) {
                task = vec.erase(task);
            } else {
                ++task;
            }
        }
    }
}