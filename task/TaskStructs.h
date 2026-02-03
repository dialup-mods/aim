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

// Task Scheduling Note:
//
// RunType::Immediate:
//
//   - Runs synchronously inside the detour (e.g., ProcessEvent, CallFunction)
//   - Can modify parameters and affect the return value
//   - May impact game performance if misused
//
// RunType::Queued:
//
//   - Defers execution until next tick via TaskScheduler::processTasks()
//   - Useful for logging, state tracking, and retry logic
//   - Does NOT guarantee exact-tick timing (e.g., may run ~8.3ms later @120fps)
//   - Should NOT modify stack or live parameters
//
// TL;DR:
//
//   - Use Queued when timing isn't critical and you want to be safe
//       massaging params will likely die with fire and is discouraged
//
//   - Use Immediate when you need in-place logic with low latency

enum class HookPhase { Pre, Post, Gated, Execute };

// template<typename ContextT>
// struct HookEntry {
//     std::function<void(ContextT&)> callback;
//     int refCount = 0;
// };
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

enum class RunType { Immediate, Queued };

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

//struct AIM_API TaskDefinition : public ITask {
//    TaskDefinition() = default;
//
//    int32_t id{ -1 };
//    std::string name{ "unknown" };
//    std::string functionName{ "unknown" };
//    int32_t functionIndex;
//    HookPhase phase;
//    bool isBlocking{ false };
//
//    bool once{ false };
//
//    std::function<void(InvocationContext&)> callback{};
//    std::function<bool(InvocationContext&)> callbackBlocking{};
//
//    // run only if condition is met
//    std::function<bool(InvocationContext&)> runCondition{};
//
//    // These options are only valid when successCondition is set --
//    std::function<bool(InvocationContext&)> successCondition{};
//    std::function<void(InvocationContext&)> preStep{};
//    std::function<void(InvocationContext&)> onSuccessCallback{};
//    std::function<void(InvocationContext&)> onFailureCallback{};
//    std::function<void(InvocationContext&)> afterSuccessCallback{};
//    std::function<void(InvocationContext&)> afterFailureCallback{};
//
//    std::shared_ptr<TaskDefinition> registerTask;
//    bool nextTick{ false };
//
//    int maxAttempts{ 15 };
//    float timeoutSeconds{ 5.0f };
//    // ------------------------------------------------------------
//
//    // do not set. internal vars
//    int attempt{ 0 };
//    float elapsedSeconds{ 0.0f };
//    TaskState state{ TaskState::Pending };
//
//    bool operator==(const TaskDefinition& other) const { return this->id == other.id; }
//
//    // fixme
//    // bool TaskDefinition::deepEquals(const TaskDefinition& other) const {
//    //    return id == other.id &&
//    //           name == other.name &&
//    //           functionName == other.functionName &&
//    //           timeoutSeconds == other.timeoutSeconds &&
//    //           phase == other.phase &&
//    //}
//
//    // ✅ Move-only
//    // for copying for logging/debugging
//    // make a TaskSummary cheap copyable version
//    // i.e. a version with no lambdas
//    //
//    // struct TaskSummary {
//    //     std::string functionName;
//    //     int id;
//    //     // maybe: phase, name, etc.
//    // };
//    //
//    // TaskSummary summary() const {
//    //     return TaskSummary{
//    //         .functionName = functionName,
//    //         .id = id,
//    //         // add more metadata here
//    //     };
//    // }
//
//    //    TaskDefinition(const TaskDefinition&) = delete;
//    //    TaskDefinition& operator=(const TaskDefinition&) = delete;
//    //
//    //    TaskDefinition(TaskDefinition&&) = default;
//    //    TaskDefinition& operator=(TaskDefinition&&) = default;
//    auto describe() const -> std::string {
//        std::string phaseStr;
//        switch (this->phase) {
//            case HookPhase::Pre: phaseStr = "Pre"; break;
//            case HookPhase::Post: phaseStr = "Post"; break;
//            case HookPhase::Gated: phaseStr = "Gated"; break;
//        }
//        return "{name:" + this->name + ", fn:" + this->functionName + ", phase:" + phaseStr + ", id:" + std::to_string(this->id) +
//            ", functionIndex:" + std::to_string(this->functionIndex) + "}";
//    }
//};
