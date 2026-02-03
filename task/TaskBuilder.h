#pragma once
#include <cassert>

#include "CallbackBuilder.h"
#include "TaskStructs.h"
#include "IModule.h"

#include <stdexcept>

enum class ReturnType { Void, Bool };

namespace c = callbackbuilder;

class TaskBuilder : IModule {
    AIM_INJECTABLE(TaskBuilder)

    std::shared_ptr<TaskDefinition> task_;

    TaskBuilder()
      : task_(std::make_shared<TaskDefinition>()) {}

    auto name(std::string n) -> TaskBuilder& {
        task_->name = std::move(n);
        return *this;
    }
    auto functionName(std::string fn) -> TaskBuilder& {
        task_->functionName = std::move(fn);
        return *this;
    }

    auto phase(HookPhase p) -> TaskBuilder& {
        task_->phase = std::move(p);
        return *this;
    }

    // fixme blocker - move blocking outside of public view
    class AIMTaskBuilderAccess {
    public:
        template<typename F>
        static TaskBuilder& callbackBlocking(TaskBuilder& b, F&& f) {
            return b.callbackBlocking(std::forward<F>(f));
        }

        static TaskBuilder& isBlocking(TaskBuilder& b, bool v = true) {
            return b.isBlocking(v);
        }
    };

    auto isBlocking(const bool isBlocking = true) -> TaskBuilder& {
        if (isBlocking && task_->phase != HookPhase::Pre) {
            // fixme
            // log_->warn("only Pre can be blocked");
        } else {
            task_->isBlocking = isBlocking;
        }
        return *this;
    }

    auto preStep(auto&& f) -> TaskBuilder& {
        task_->preStep = callbackbuilder::wrapCallback(std::forward<decltype(f)>(f));
        return *this;
    }

    auto callback(auto&& f) -> TaskBuilder& {
        task_->callback = callbackbuilder::wrapCallback(std::forward<decltype(f)>(f));
        return *this;
    }

    auto callbackBlocking(auto&& f) -> TaskBuilder& {
        task_->callbackBlocking = callbackbuilder::wrapWithReturn(std::forward<decltype(f)>(f));
        task_->isBlocking = true;
        return *this;
    }

    auto runCondition(auto&& f) -> TaskBuilder& {
        task_->runCondition = callbackbuilder::wrapWithReturn(std::forward<decltype(f)>(f));
        return *this;
    }

    auto successCondition(auto&& f) -> TaskBuilder& {
        task_->successCondition = callbackbuilder::wrapWithReturn(std::forward<decltype(f)>(f));
        return *this;
    }

    auto onSuccessCallback(auto&& f) -> TaskBuilder& {
        task_->onSuccessCallback = callbackbuilder::wrapCallback(std::forward<decltype(f)>(f));
        return *this;
    }

    auto onFailureCallback(auto&& f) -> TaskBuilder& {
        task_->onFailureCallback = callbackbuilder::wrapCallback(std::forward<decltype(f)>(f));
        return *this;
    }

    auto nextTick(bool deferred = false) -> TaskBuilder& {
        task_->nextTick = deferred;
        return *this;
    }

    auto once(bool once = true) -> TaskBuilder& {
        task_->once = once;
        return *this;
    }

    auto registerTask(std::shared_ptr<TaskDefinition>& task) -> TaskBuilder& {
        task_->registerTask = std::move(task);
        return *this;
    }

    auto maxAttempts(int maxAttempts) -> TaskBuilder& {
        if (maxAttempts > 0) {
            task_->maxAttempts = std::move(maxAttempts);
        } else {
            // log_->warn("maxAttempts must be greater than 0. defaulting.");
        }
        return *this;
    }

    auto timeoutSeconds(const float timeoutSeconds) -> TaskBuilder& {
        if (timeoutSeconds > 0) {
            task_->timeoutSeconds = timeoutSeconds;
        } else {
            // log_->warn("timeoutSeconds must be greater than 0. defaulting.");
        }
        return *this;
    }

    auto build() -> std::shared_ptr<TaskDefinition> {
        static int32_t nextID = 420;
        task_->id = nextID++;

        if (doesSomething()) {
            return std::move(task_);
        }

        assert("TaskBuilder: Cannot build a task that does nothing.");
        return nullptr;
    }

    auto retry(const int attempts, const float seconds) -> TaskBuilder& {
        return maxAttempts(attempts).timeoutSeconds(seconds);
    }

    static auto describe(const TaskDefinition& task) -> std::string {
        std::string phaseStr;
        switch (task.phase) {
            case HookPhase::Pre: phaseStr = "Pre"; break;
            case HookPhase::Post: phaseStr = "Post"; break;
            case HookPhase::Gated: phaseStr = "Gated"; break;
        }
        return "{name:" + task.name + ", fn:" + task.functionName + ", phase:" + phaseStr + ", id:" + std::to_string(task.id) +
            ", functionIndex:" + std::to_string(task.functionIndex) + "}";
    }

  private:
    bool doesSomething() const {
        return task_->preStep || task_->callbackBlocking || task_->successCondition || task_->onSuccessCallback ||
            task_->afterSuccessCallback || task_->onFailureCallback || task_->callback;
    }
};