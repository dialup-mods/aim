#pragma once
#include <stdexcept>

#include "CallbackBuilder.h"
#include "TaskStructs.h"
#include "IModule.h"

namespace c = callbackbuilder;

class ITaskBuilder : public IModule {
    AIM_INJECTABLE(ITaskBuilder);

    std::shared_ptr<TaskDefinition> task_;

    ITaskBuilder()
      : task_(std::make_shared<TaskDefinition>()) {}

    auto name(std::string n) -> ITaskBuilder& {
        task_->name = std::move(n);
        return *this;
    }
    auto functionName(std::string fn) -> ITaskBuilder& {
        task_->functionName = std::move(fn);
        return *this;
    }

    auto phase(HookPhase p) -> ITaskBuilder& {
        task_->phase = std::move(p);
        return *this;
    }

    auto isBlocking(bool isBlocking = true) -> ITaskBuilder& {
        if (isBlocking && task_->phase != HookPhase::Pre) {
            // fixme
            // log_->warn("only Pre can be blocked");
        } else {
            task_->isBlocking = isBlocking;
        }
        return *this;
    }

    auto once(bool once = true) -> ITaskBuilder& {
        task_->once = once;
        return *this;
    }

    auto nextTick(bool deferred = false) -> ITaskBuilder& {
        task_->nextTick = deferred;
        return *this;
    }

    template<typename F>
    auto callback(F&& f) -> ITaskBuilder& {
        task_->callback = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto callbackBlocking(F&& f) -> ITaskBuilder& {
        task_->callbackBlocking = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto preStep(F&& f) -> ITaskBuilder& {
        task_->preStep = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto successCondition(F&& f) -> ITaskBuilder& {
        task_->successCondition = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto onSuccessCallback(F&& f) -> ITaskBuilder& {
        task_->onSuccessCallback = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto onFailureCallback(F&& f) -> ITaskBuilder& {
        task_->onFailureCallback = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    auto registerTask(std::shared_ptr<TaskDefinition>& task) -> ITaskBuilder& {
        task_->registerTask = std::move(task);
        return *this;
    }
    auto maxAttempts(int maxAttempts) -> ITaskBuilder& {
        if (maxAttempts > 0) {
            task_->maxAttempts = std::move(maxAttempts);
        } else if (maxAttempts <= 0) {
            // log_->warn("maxAttempts must be greater than 0. defaulting.");
        }
        return *this;
    }
    auto timeoutSeconds(float timeoutSeconds) -> ITaskBuilder& {
        if (timeoutSeconds > 0) {
            task_->timeoutSeconds = timeoutSeconds;
        } else if (timeoutSeconds <= 0) {
            // log_->warn("timeoutSeconds must be greater than 0. defaulting.");
        }
        return *this;
    }

    auto build() -> std::shared_ptr<TaskDefinition> {
        static int32_t nextID = 420;
        task_->id = nextID++;
        if (!doesSomething()) {
            MessageBoxA(nullptr, "task must do something", "ITaskBuilder", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
            throw std::runtime_error("RecoveryITaskBuilder: Cannot build a task that does nothing.");
        }

        auto out = std::move(task_);
        task_ = std::make_shared<TaskDefinition>();
        return out;
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

    auto retry(int attempts, float seconds) -> ITaskBuilder& { return maxAttempts(attempts).timeoutSeconds(seconds); }

  private:
    bool doesSomething() const {
        return task_->preStep || task_->callbackBlocking || task_->successCondition || task_->onSuccessCallback ||
            task_->afterSuccessCallback || task_->onFailureCallback || task_->callback;
    }
};
