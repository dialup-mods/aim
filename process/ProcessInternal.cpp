#include "ProcessInternal.h"
#include "ProcessEvent.h"
#include "AIM.h"

#include "AsyncGate.h"
#include "Detour.h"
#include "Dispatch.h"
#include "EventContext.h"
#include "ILogger.h"
#include "MutexGuard.h"
#include "ObjectQuery.h"
#include "PatchUtils.h"
#include "Resolver.h"
#include "Runtime.h"

using r = Runtime;

class UObject;
namespace p = patchutils;

ProcessInternal* ProcessInternal::instance_ = nullptr;

void
ProcessInternal::init() {
    log_->debug("{} init()", getName());
    
    instance_ = this;
    mutex_->setName(getName() + "_Detour");
    findAddress();
    applyDetour();
}

void* ProcessInternal::findAddress() {
    auto fn = r::ufunction::find("Function Engine.HUD.PostRender")->Func.Ptr;
    printf(" found possible PI: %p\n", fn);
    return fn;
}

void ProcessInternal::shutdown() {
    dispatch_->shutdown();
}

void ProcessInternal::registerTask(std::shared_ptr<TaskDefinition> def) {
    dispatch_->registerTask(def);
}

void ProcessInternal::releaseTask(std::shared_ptr<TaskDefinition> def) {
    dispatch_->releaseTask(def);
}

void ProcessInternal::clearTasks() {
    dispatch_->clearTasks();
}

bool
ProcessInternal::applyDetour() {
    log_->debug("[PI] applyDetour()");

    teardownFence_->block("PI");

    if (!mutex_->tryAcquire(1/*ms*/)) {
        log_->warn("[PI] Already patched -- nothing to apply");
        return false;
    }

    log_->debug("[PI] applying detour at: {}", findAddress());
    if (!detour_->attach(findAddress(), (void*)&handleFunction)) {
        log_->debug("[PI] failed to apply detour at: {}", findAddress());
        return false;
    }

    log_->debug("[PI] detoured");
    appliedGate_->setReady();
    return true;
}

bool ProcessInternal::removeDetour() {
    log_->debug("[PI] removeDetour()");
    dispatch_->shutdown();
    mutex_->release();
    // wait for any in-flight handlers to finish
    if (!mutex_->waitForUnlock(5000)) {
        log_->error("Timeout waiting for ProcessEvent handlers to finish");
        return false;
    }
    if (!detour_->detach()) {
        log_->error("[PI] Failed to detach detour");
        return false;
    }
    //removedGate_->setReady();
    teardownFence_->release("PI");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return true;
}

bool ProcessInternal::waitForUnlock(DWORD timeoutMs) const {
    return mutex_->waitForUnlock(timeoutMs);
}

bool ProcessInternal::fastIsAcquired() {
    if (instance_ && instance_->getMutex() && instance_->getMutex()->checkIsNamed()) {
        return instance_->getMutex()->fastIsAcquired();
    }
    return false;
}

void* ProcessInternal::getTrampoline() {
    if (instance_ && instance_->detour_->getTrampoline()) {
        return instance_->detour_->getTrampoline();
    }
    return nullptr;
}

void __fastcall ProcessInternal::handleFunction(UObject* self, FFrame& stack, void* result) {
    if (!fastIsAcquired()) {
        reinterpret_cast<tProcessInternal>(getTrampoline())(self, stack, result);
        return;
    }

    //if(self && self->GetFullName().find("HUD") != std::string::npos) {
    //    printf("[PI] %s -> %d\n", self->GetFullName().c_str(), self->ObjectInternalInteger);
    //    printf("[PI] %s -> %d\n", self->GetFullName().c_str(), self->ObjectInternalInteger);
    //}

    auto dispatch = AIM::getStaticResolver()->resolve<Dispatch>("PI-Dispatch");

    if (dispatch != nullptr) {
        auto context = InvocationContext::makeProcessInternalContext(self, stack, nullptr);
        
        // run prehooks
        if (dispatch->dispatchPre(self->ObjectInternalInteger, context)) {
            // if dispatchPre returns true, block the original call
            return;
        }

        // call original function
        reinterpret_cast<tProcessInternal>(getTrampoline())(self, stack, result);

        // run posthooks
        context = InvocationContext::makeProcessInternalContext(self, stack, result);
        dispatch->dispatchPost(self->ObjectInternalInteger, context);
        
        dispatch->dispatchGated(self->ObjectInternalInteger, context);

    } else { // fallback if no dispatcher
        reinterpret_cast<tProcessInternal>(getTrampoline())(self, stack, result);
    }
}