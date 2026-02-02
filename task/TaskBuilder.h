#pragma once
#include "CallbackBuilder.h"
#include "TaskStructs.h"
#include "IModule.h"
#include "ITaskBuilder.h"

#include <stdexcept>

enum class ReturnType { Void, Bool };

namespace c = callbackbuilder;

class AIM_API TaskBuilder : ITaskBuilder {
    AIM_INJECTABLE(TaskBuilder)

    std::shared_ptr<TaskDefinition> task_;

    TaskBuilder()
      : task_(std::make_shared<TaskDefinition>()) {}

    auto name(std::string n) -> TaskBuilder& override {
        task_->name = std::move(n);
        return *this;
    }
    auto functionName(std::string fn) -> TaskBuilder& override {
        task_->functionName = std::move(fn);
        return *this;
    }

    auto phase(HookPhase p) -> TaskBuilder& override {
        task_->phase = std::move(p);
        return *this;
    }

    auto isBlocking(const bool isBlocking = true) -> TaskBuilder& {
        if (isBlocking && task_->phase != HookPhase::Pre) {
            // fixme
            // log_->warn("only Pre can be blocked");
        } else {
            task_->isBlocking = isBlocking;
        }
        return *this;
    }

    auto once(bool once = true) -> TaskBuilder& override {
        task_->once = once;
        return *this;
    }

    auto nextTick(bool deferred = false) -> TaskBuilder& override {
        task_->nextTick = deferred;
        return *this;
    }

    template<typename F>
    auto callback(F&& f) -> TaskBuilder& {
        task_->callback = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto callbackBlocking(F&& f) -> TaskBuilder& {
        task_->callbackBlocking = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto preStep(F&& f) -> TaskBuilder& {
        task_->preStep = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto successCondition(F&& f) -> TaskBuilder& {
        task_->successCondition = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto onSuccessCallback(F&& f) -> TaskBuilder& {
        task_->onSuccessCallback = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    template<typename F>
    auto onFailureCallback(F&& f) -> TaskBuilder& {
        task_->onFailureCallback = c::wrapCallback(std::forward<F>(f));
        return *this;
    }
    auto registerTask(std::shared_ptr<TaskDefinition>& task) -> TaskBuilder& {
        task_->registerTask = std::move(task);
        return *this;
    }
    auto maxAttempts(int maxAttempts) -> TaskBuilder& override {
        if (maxAttempts > 0) {
            task_->maxAttempts = std::move(maxAttempts);
        } else {
            // log_->warn("maxAttempts must be greater than 0. defaulting.");
        }
        return *this;
    }
    auto timeoutSeconds(const float timeoutSeconds) -> TaskBuilder& override {
        if (timeoutSeconds > 0) {
            task_->timeoutSeconds = timeoutSeconds;
        } else {
            // log_->warn("timeoutSeconds must be greater than 0. defaulting.");
        }
        return *this;
    }

    auto build() -> std::shared_ptr<TaskDefinition> override {
        static int32_t nextID = 420;
        task_->id = nextID++;

        if (doesSomething()) {
            return std::move(task_);
        } else {
            MessageBoxA(nullptr, "task must do something", "TaskBuilder", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        }
        throw std::runtime_error("RecoveryTaskBuilder: Cannot build a task that does nothing.");
    }

    auto retry(const int attempts, const float seconds) -> TaskBuilder& override {
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

protected:
    void setCallback(CallbackType cb) override {
        task_->callback = std::move(cb);
    }

    void setPreStep(CallbackType cb) override {
        task_->preStep = std::move(cb);
    }

    void setSuccessCondition(CallbackType cb) override {
        task_->preStep = std::move(cb);
    }

    void setSuccessCallback(CallbackType cb) override {
        task_->preStep = std::move(cb);
    }

    void setFailureCallback(CallbackType cb) override {
        task_->preStep = std::move(cb);
    }
};