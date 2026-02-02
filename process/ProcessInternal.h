#pragma once
#include <span>
#include <string>
#include <functional>

#include "IModule.h"
#include "IProcessInternal.h"

#include "AsyncGate.h"
#include "PluginFence.h"
#include "TaskStructs.h"

using DWORD = unsigned long;

class ILogger;
class ObjectProvider;
class Dispatch;
class TaskQueue;
class PluginState;
class ProcessEvent;
class MutexGuard;
class Detour;

class PreEventContext;
class PostEventContext;

class UObject;
struct FFrame;

class ProcessInternal : public IProcessInternal
{
    AIM_INJECTABLE(ProcessInternal)

    AIM_INJECT(ILogger, log)
    AIM_INJECT(PluginState, state)
    AIM_INJECT(Dispatch, dispatch)
    AIM_INJECT(TaskQueue, taskQueue)
    AIM_INJECT(ObjectProvider, objectProvider)
    AIM_INJECT(AsyncGate, appliedGate)
    AIM_INJECT(AsyncGate, removedGate)
    AIM_INJECT(MutexGuard, mutex)
    AIM_INJECT(PluginFence, readyFence)
    AIM_INJECT(PluginFence, teardownFence)
    AIM_INJECT(Detour, detour)

    AIM_INJECT(ProcessEvent, processEvent)
    
    using BYTE = unsigned char;
    using PatternSpan = std::span<const BYTE>;
    
    using tProcessInternal = void(__fastcall*)(UObject* self, FFrame& stack, void* result);
        
private:

    // TODO / FIXME
    // store weak ref to self
    // use that in detour

// clang-format off

public:
    ProcessInternal() = default;
    
    std::string getName() const { return "ProcessInternal"; }
    
    static ProcessInternal* instance_;
    
    auto getAppliedGate() { return appliedGate_; }
    auto getRemovedGate() { return removedGate_; }

    void onDetoured(const std::function<void()>& callback) const {
        return appliedGate_->onReady(callback);
    }

    int  getVTableIndex()       const { return -1; }
    bool shouldUseVTableEntry() const { return false; }
    bool shouldResolveJump()    const { return true; }
    
    static inline std::shared_ptr<TaskDefinition> applyTask_{};
    static inline std::shared_ptr<TaskDefinition> removeTask_{};
    
    static inline void* slack_ = nullptr;
    static inline void* processInternalTargetFn_ = nullptr;
    static inline void* newProcessInternalFn_ = nullptr;
    static inline void* bakkesTrampolineFn_ = nullptr;
    static inline void* cachedVTableFn_ = nullptr;

    // 00007FF7A8BDAE60
    static constexpr BYTE patternBytes_[] = {
        0xE9, 0x00, 0x00, 0x00, 0x00, 0x48, 0x81, 0xEC, 0xC8, 0x00
        , 0x00, 0x00, 0x48, 0x8B, 0x05, 0xCD, 0x68, 0xEA, 0x01, 0x48
        , 0x33, 0xC4, 0x48, 0x89, 0x84, 0x24, 0x00, 0x00, 0x00, 0x00
        , 0x48, 0x8B, 0x01, 0x48, 0x8B, 0xDA, 0x48, 0x8B, 0x52, 0x18

    };
    /*
     * with bakkesmod patch
        E9 ** ** ** ** 48 81 EC C8 ** ** ** 48 8B 05 CD 68 EA 01 48 33 C4 48 89 84 24 ** ** ** ** 48 8B 01 48 8B DA 48 8B 52 18
    */
    
    PatternSpan getPattern() const { return PatternSpan(patternBytes_); }

    void init();

    void *findEnginePIAddress();

    void *findAddress();

    bool buildPatches();
    void getPIFn();
    void* getTargetFn();
    void shutdown();
    void registerTask(std::shared_ptr<TaskDefinition> def) override;
    void releaseTask(std::shared_ptr<TaskDefinition> def) override;
    void clearTasks();
    void* getRLFn();

    const std::shared_ptr<MutexGuard>& getMutex() const { return mutex_; }
    bool waitForUnlock(DWORD timeoutMs = 500) const;
    static bool fastIsAcquired();

    static void *getTrampoline();

    void applyDetour();

    void removeDetour();
    
    static void __fastcall handleFunction(UObject* self, FFrame& stack, void* result);
};

// clang-format on