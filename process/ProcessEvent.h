#pragma once

#include "IModule.h"
#include "AsyncGate.h"
#include "IProcessEvent.h"

#include <PluginFence.h>

using DWORD = unsigned long;
//#include <functional>
//class GameWrapper;
//__declspec(dllimport)
//void __cdecl ExecuteOnGameWrapper(std::function<void(GameWrapper*)> fn);

#include <span>
#include <string>
#include <utility>

class ILogger;
class Dispatch;
class TaskQueue;
class PluginState;

class PreEventContext;
class PostEventContext;
class AsyncGate;
//class GameWrapperProvider;

class UObject;
class UFunction;
class MutexGuard;
class PluginFence;
class Detour;

#include "TaskStructs.h"

// clang-format off
    
class ProcessEvent : public IProcessEvent {
    AIM_INJECTABLE(ProcessEvent)

    AIM_INJECT(ILogger, log)
    AIM_INJECT(Dispatch, dispatch)
    AIM_INJECT(TaskQueue, taskQueue)
    AIM_INJECT(AsyncGate, appliedGate)
    AIM_INJECT(AsyncGate, removedGate)
    AIM_INJECT(PluginFence, readyFence)
    AIM_INJECT(PluginFence, teardownFence)
    AIM_INJECT(MutexGuard, mutex)
    AIM_INJECT(Detour, detour)
//    AIM_INJECT(GameWrapperProvider, gameWrapperProvider)

    using tProcessEvent = void(__fastcall*)(UObject* self, UFunction* fn, void* params, void* result);

    using BYTE = unsigned char;
    using PatternSpan = std::span<const BYTE>;

    std::string getName() const { return "ProcessEvent"; }
    
    static ProcessEvent* instance_;

    bool init();
    bool buildPatches();

    void onDetoured(const std::function<void()>& callback) const {
        printf("ON DETOURED\n");
        return appliedGate_->onReady(callback);
    }

    void onDetourRemoved(const std::function<void()>& callback) const {
        printf("ON DETOURED REMOVED\n");
        return removedGate_->onReady(callback);
    }

    int  getVTableIndex()       const { return 67; }
    bool shouldUseVTableEntry() const { return true; }
    bool shouldResolveJump()    const { return true; }

    static inline std::shared_ptr<TaskDefinition> applyTask_{};
    static inline std::shared_ptr<TaskDefinition> removeTask_{};
    
    static inline uint8_t* slackFn_ = nullptr;
    static inline void* processEventVTableFn_ = nullptr;
    static inline void* targetFn = nullptr;
    static inline void* resumePtr_ = nullptr;
    static inline void* bakkesTrampolineFn_ = nullptr;
    static inline void* processEventTargetFn = nullptr;
    static inline void* addressOfFirstJumpFromVTable = nullptr;
    static inline void* bakkesModRealFn_ = nullptr;
    static inline void* cachedVTableFn = nullptr;
    
    static constexpr BYTE patternBytes[] = {
        // 0x40, 0x55, 0x41, 0x56, 0x41, 0x57 // prologue when bakkes not running
        0xE9, 0x00, 0x00, 0x00, 0x00, 0xCC, // bakkesmod detour jump overwrites first 6 bytes
        0x48, 0x81, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x48,
        0x8D, 0x6C, 0x24, 0x20, 0x48, 0x89, 0x9D, 0x00,
        0x00, 0x00, 0x00, 0x48, 0x89, 0xB5, 0x00, 0x00,
        0x00, 0x00, 0x48, 0x89, 0xBD, 0x00, 0x00, 0x00,
        0x00, 0x4C, 0x89, 0xA5
    };
    // 40 55 41 56 41 57 E9 ** ** ** ** CC 48 81 EC ** ** ** ** 48 8D 6C 24 20 48 89 9D ** ** ** ** 48 89 B5 ** ** ** ** 48 89 BD ** ** **    ** 4C 89 A5
    // 40 55 41 ** ** ** ** ** ** ** ** CC 48 81 EC ** ** ** ** 48 8D 6C 24 20 48 89 9D ** ** ** ** 48 89 B5 ** ** ** ** 48 89 BD ** ** **    ** 4C 89 A5
    //48 81 EC ** ** ** ** 48 8D 6C 24 20 48 89 9D ** ** ** ** 48 89 B5 ** ** ** ** 48 89 BD ** ** ** ** 4C 89 A5
    PatternSpan getPattern() const {return PatternSpan(patternBytes);
    }

    void shutdown();
    void registerTask(std::shared_ptr<TaskDefinition> def) override;
    void releaseTask(std::shared_ptr<TaskDefinition> def) override;
    void clearTasks();
    void* getRLFn();

    void *getResumeAddress();

    void* getBakkesTrampolineFn();
    
    const std::shared_ptr<MutexGuard>& getMutex() const { return mutex_; }
    bool waitForUnlock(DWORD timeoutMs = 500) const;
    static bool fastIsAcquired();

    static void *getTrampoline();

    static void printStr(const std::string&, const std::string&);

    static auto convert(void *params) -> uint8_t *;

    bool applyDetour();

    bool removeDetour();

    static void __fastcall handleFunction(UObject* self, UFunction* fn, void* paramsPtr, void* resultPtr);
};

// clang-format on
