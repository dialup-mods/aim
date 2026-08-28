#include "CallFunction.h"

#include "SDK.h"

#include "Resolver.h"

#include "TaskBuilder.h"
#include "AsyncGate.h"
#include "Dispatch.h"
#include "ILogger.h"
#include "MutexGuard.h"
#include "PatchUtils.h"
#include "ProcessEvent.h"
#include "TaskQueue.h"
#include "TaskStructs.h"

#include "AIM.h"

namespace p = patchutils;

CallFunction* CallFunction::instance_ = nullptr;

struct ReentrancyGuard {
    bool& flag;
    ReentrancyGuard(bool& f) : flag(f) { flag = true; }
    ~ReentrancyGuard() { flag = false; }
};

void
CallFunction::init() {
    log_->debug("[CF] init");

    instance_ = this;
    mutex_->setName(getName() + "_Detour");

    applyDetour();
}

void CallFunction::shutdown() {
    dispatch_->shutdown();
}

void CallFunction::registerTask(std::shared_ptr<TaskDefinition> def) {
    dispatch_->registerTask(def);
}

void CallFunction::releaseTask(std::shared_ptr<TaskDefinition> def) {
    dispatch_->releaseTask(def);
}

void CallFunction::clearTasks() {
    dispatch_->clearTasks();
}

void* CallFunction::getAddress() {
    if (callFunctionFn_ == nullptr) {
        callFunctionFn_ = patchutils::patternScan(patternBytes_);
    }
    return callFunctionFn_;
}

bool CallFunction::applyDetour() {
    log_->debug("[CF] applyDetour()");
    
    // run inside main event loop
    // to prevent race conditions
    processEvent_->registerTask(TaskBuilder()
        .name("Apply CallFunction patch")
        .functionName("Function Engine.Interaction.Tick")
        .phase(HookPhase::Post)
        .callback([this](InvocationContext& ctx) {
            printf("[CF] applying detour...");

            if (!mutex_->tryAcquire(1 /*ms*/)) {
                log_->warn("[CF] Already patched -- nothing to apply");
                return;
            }

            printf("[CF] detoured");
            appliedGate_->setReady();
        })
        .once()
        .build());

    return true;
}

bool CallFunction::removeDetour() {
    // run inside main event loop
    // to prevent race conditions
    processEvent_->registerTask(TaskBuilder()
        .name("Apply CallFunction patch [remove]")
        .functionName("Function Engine.Interaction.Tick")
        .phase(HookPhase::Post)
        .callback([this](InvocationContext& ctx) {
            printf("[CF] removing detour...\n");

            printf("[CF] removed\n");
            this->mutex_->release();
            this->removedGate_->setReady();
        })
        .once()
        .build());

    return true;
}

bool CallFunction::waitForUnlock(DWORD timeoutMs) const {
    return mutex_->waitForUnlock(timeoutMs);
}

bool CallFunction::fastIsAcquired() {
    if (instance_ && instance_->getMutex() && instance_->getMutex()->checkIsNamed()) {
        return instance_->getMutex()->fastIsAcquired();
    }
    return false;
}

void* CallFunction::getTrampoline() {
//    if (instance_ && instance_->detour_->getTrampoline()) {
//        return instance_->detour_->getTrampoline();
//    }
    return nullptr;
}

void __fastcall CallFunction::handleFunction(UObject* self, FFrame& stack, void* result, UFunction* fn) {
//    thread_local bool inHandler = false;
//    if (inHandler || !fastIsAcquired()) {
//        reinterpret_cast<tCallFunction>(getTrampoline())(self, stack, result, fn);
//        return;
//    }
//
//    ReentrancyGuard guard(inHandler);
//
//    if (auto dispatch = AIM::getStaticResolver()->resolve<Dispatch>("CF-Dispatch")) {
//        auto ctx = InvocationContext::makeCallFunctionContext(self, stack, nullptr, fn);
//        if (dispatch->dispatchPre(fn->ObjectInternalInteger, ctx)) {
//            // blocking was requested
//            printf("[CF] should block. Returning");
//            return;
//        }
//
//        reinterpret_cast<tCallFunction>(getTrampoline())(self, stack, result, fn);
//
//        ctx = InvocationContext::makeCallFunctionContext(self, stack, result, fn);
//        dispatch->dispatchPost(fn->ObjectInternalInteger, ctx);
//
//        dispatch->dispatchGated(fn->ObjectInternalInteger, ctx);
//    } else {
//        reinterpret_cast<tCallFunction>(getTrampoline())(self, stack, result, fn);
//    }
}