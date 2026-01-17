#pragma once
#include <span>
#include <string>
#include <functional>

#include "IModule.h"
#include "ICallFunction.h"
#include "AsyncGate.h"
#include "PluginFence.h"
#include "PatchDefinition.h"
#include "TaskStructs.h"

using DWORD = unsigned long;
class ILogger;
class ObjectProvider;
class Dispatch;
class PatchManager;
class TaskQueue;
class PluginState;
class ProcessInternal;
class ProcessEvent;
class MutexGuard;

class UObject;
struct FFrame;
class UFunction;

class PreEventContext;
class PostEventContext;

class UProperty;
struct CustomOutParmRec {
    void* ParamAddr;  // Address of the parameter
    UProperty* Property;  // Property type
};

class CallFunction : public ICallFunction {
    AIM_INJECTABLE(CallFunction)

    AIM_INJECT(ILogger, log)
    AIM_INJECT(PluginState, state)
    AIM_INJECT(Dispatch, dispatch)
    AIM_INJECT(TaskQueue, taskQueue)
    AIM_INJECT(ObjectProvider, objectProvider)
    AIM_INJECT(PatchManager, patchManager)
    AIM_INJECT(AsyncGate, appliedGate)
    AIM_INJECT(AsyncGate, removedGate)
    AIM_INJECT(MutexGuard, mutex)
    AIM_INJECT(PluginFence, readyFence)
    AIM_INJECT(PluginFence, teardownFence)

    AIM_INJECT(ProcessEvent, processEvent)
    AIM_INJECT(ProcessInternal, processInternal)
    
    using tCallFunction = void(__thiscall*)(UObject* self, FFrame& stack, void* result, UFunction* fn);

public:
    using BYTE = unsigned char;
    using PatternSpan = std::span<const BYTE>;

private:
    
    // TODO / FIXME
    // store weak ref to self
    // use that in detour

// clang-format off

public:
    std::string getName() const { return "CallFunction"; } // clang-format off
    
    static CallFunction* instance_;
    
    auto getAppliedGate() { return appliedGate_; }
    auto getRemovedGate() { return removedGate_; }

    void onDetoured(const std::function<void()>& callback) const {
        return appliedGate_->onReady(callback);
    }
    
    int  getVTableIndex()       const { return 76; }
    bool shouldUseVTableEntry() const { return false; }
    bool shouldResolveJump()    const { return true; }
    
    static inline PatchDefinition applyPatch_ {};
    static inline PatchDefinition removePatch_ {};
    static inline std::shared_ptr<TaskDefinition> applyTask_ {};
    static inline std::shared_ptr<TaskDefinition> removeTask_ {};
    
    static inline void* slack_ = nullptr;
    static inline void* targetFn_ = nullptr;
    static inline void* callFunctionFn_ = nullptr;
    static inline void* bakkesTrampolineFn_ = nullptr;
    static inline void* cachedVTableFn_ = nullptr;
    
    static constexpr BYTE patternBytes_[] = {
        // prologue (or jump when bakkes is running)
        // 0x40, 0x55, 0x53, 0x56, 0x57,
        0xE9, 0x00, 0x00, 0x00, 0x00,

        // rest of pattern
        0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
        0x48, 0x81, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x48,
        0x8D, 0x6C, 0x24, 0x20
    }; // clang-format on
    /*
    E9 ** ** ** ** 41 54 41 55 41 56 41 57 48 81 EC ** ** ** ** 48 8D 6C 24 20
    40 55 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ** ** ** ** 48 8D 6C 24 20
    
    */
    PatternSpan getPattern() const {
        return PatternSpan(patternBytes_);
    }
    
    // here instead of base class because the logic differs
    // for CallFunction and ProcessInternal
    void* getTargetFn() {
       if (targetFn_ != nullptr) { return targetFn_; }

        return targetFn_;
    }

    void init();
    void buildPatches();
    void shutdown();
    void registerTask(std::shared_ptr<TaskDefinition> def) override;
    void releaseTask(std::shared_ptr<TaskDefinition> def) override;
    void clearTasks() override;
    void* getRLFn();
    void* getBakkesTrampolineFn();
    
    const std::shared_ptr<MutexGuard>& getMutex() const { return mutex_; }
    bool waitForUnlock(DWORD timeoutMs = 500) const;
    static bool fastIsAcquired();
    
    bool applyDetour();
    bool removeDetour();
    
    static void __fastcall handleFunction(UObject* self, FFrame& stack, void* result, UFunction* fn);
};

