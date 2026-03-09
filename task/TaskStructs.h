#pragma once
#include <functional>
#include <memory>
#include <string>

#include "CallbackWrapper.h"
#include "EventContext.h"

#ifdef AIM_BUILD
#define AIM_API __declspec(dllexport)
#else
#define AIM_API __declspec(dllimport)
#endif

enum class HookPhase { Pre, Post, Gated, Execute };

struct HookCount {
    int refCount = 0;
};

struct HookKey {
    int functionIndex;
    std::string functionName;
    HookPhase phase;

    bool operator==(const HookKey& other) const { return functionIndex == other.functionIndex && phase == other.phase; }
};

struct HookKeyHash {
    std::size_t operator()(const HookKey& k) const {
        return std::hash<std::string>()(k.functionName) ^ std::hash<int>()(static_cast<int>(k.phase));
    }
};

class HookIDGenerator {
  public:
    static int nextID() {
        static int current = 0;
        return current++;
    }
};

enum class TaskState {
    Pending,   // Default state: waiting on success condition
    Waiting,   // Success condition met, waiting to run callbacks
    Running,   // Callback(s) executing
    Completed, // All callbacks done, task fully resolved
    Cancelled, // Task was removed before completion
    Failed,    // it's dead, Jim
};

struct ITask {
    virtual ~ITask() = default;
    virtual void invoke(InvocationContext&) = 0;
    virtual bool once() const = 0;
    virtual HookPhase phase() const = 0;
};

struct TaskDefinition {
    TaskDefinition() = default;

    int32_t id{ -1 };
    std::string name{ "unknown" };
    std::string functionName{ "unknown" };
    int32_t functionIndex;
    HookPhase phase;
    bool isBlocking{ false };
    bool once{ false };

    CallbackWrapper callback{};
    CallbackWrapper callbackBlocking{};  // returns bool

    CallbackWithReturnWrapper runCondition{};
    CallbackWithReturnWrapper successCondition{};

    CallbackWrapper preStep{};
    CallbackWrapper onSuccessCallback{};
    CallbackWrapper onFailureCallback{};
    CallbackWrapper afterSuccessCallback{};
    CallbackWrapper afterFailureCallback{};

    std::shared_ptr<TaskDefinition> registerTask;
    bool nextTick{ false };

    int maxAttempts{ 15 };
    float timeoutSeconds{ 5.0f };

    int attempt{ 0 };
    float elapsedSeconds{ 0.0f };
    TaskState state{ TaskState::Pending };

    bool operator==(const TaskDefinition& other) const { return this->id == other.id; }

    // Still move-only (CallbackWrappers are move-only)
    TaskDefinition(const TaskDefinition&) = delete;
    TaskDefinition& operator=(const TaskDefinition&) = delete;

    TaskDefinition(TaskDefinition&&) = default;
    TaskDefinition& operator=(TaskDefinition&&) = default;

    auto describe() const -> std::string {
        std::string phaseStr;
        switch (this->phase) {
            case HookPhase::Pre: phaseStr = "Pre"; break;
            case HookPhase::Post: phaseStr = "Post"; break;
            case HookPhase::Gated: phaseStr = "Gated"; break;
        }
        return "{name:" + this->name + ", fn:" + this->functionName + ", phase:" + phaseStr + ", id:" + std::to_string(this->id) +
            ", functionIndex:" + std::to_string(this->functionIndex) + "}";
    }
};
