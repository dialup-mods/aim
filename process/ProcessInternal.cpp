#include "ProcessInternal.h"
#include "AIM.h"

#include "Resolver.h"

#include "AsyncGate.h"
#include "Dispatch.h"
#include "EventContext.h"
#include "GameTypes.h" // for FFrame
#include "ILogger.h"
#include "MutexGuard.h"
#include "ObjectProvider.h"
#include "PatchBuilder.h"
#include "PatchManager.h"
#include "PatchUtils.h"
#include "ProcessEvent.h"
#include "TaskBuilder.h"

class UObject;
namespace p = patchutils;

// for mutex
ProcessInternal* ProcessInternal::instance_ = nullptr;

void
ProcessInternal::init() {
    log_->debug("{} init()", getName());
    
    instance_ = this;
    mutex_->setName(getName() + "_Detour");

    findAddress();
    buildPatches();
    applyDetour();

}

void
ProcessInternal::findAddress() {
    processEvent_->registerTask(TaskBuilder()
            .name("[PI] Find")
            .functionName("Function Engine.Interaction.Tick")
            .phase(HookPhase::Gated)
            .maxAttempts(100)
            .timeoutSeconds(120.0f)
            .preStep([this](InvocationContext& ctx) {
                if (!ctx.function() || !ctx.function()->Func) {
                    log_->debug("no ctx fn");
                    return;
                }

                auto isValidAddr = [&](void* fn) -> bool {
                    uintptr_t funcAddr = reinterpret_cast<uintptr_t>(fn);

                    if (funcAddr < 0x10000 || funcAddr > 0x7FFFFFFFFFFF) {
                        //log_->warn("[PI] function is outside valid range: 0x{:X}", funcAddr);
                        log_->warn("[PI] function is outside valid range");
                        return false;
                    }
                    if (!safe::memory::isAddressAccessible(fn, sizeof(void*))) {
                        //log_->logf_debug("[PI] inaccessible memory: 0x{:X}", funcAddr);
                        log_->warn("[PI] inaccessible memory");
                        return false;
                    }
                    return true;
                };

                auto fn = ctx.function()->Func;
                if (!isValidAddr(fn.Ptr)) {
                    return;
                }

                auto chain = patchutils::walkTrampolineChain(fn.Ptr);

                if (chain.size() < 2) {
                    log_->error("[PE] Trampoline chain too short, expected at least 2 entries");
                    MessageBoxA(nullptr, "trampoline too short", "()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
                    return;
                }

                if (!isValidAddr(chain[1])) {
                    return;
                }

                processInternalTargetFn_ = fn.Ptr;
                bakkesTrampolineFn_ = chain[1];
            })
            .successCondition(
                [this](InvocationContext& ctx) { return processInternalTargetFn_ != nullptr && bakkesTrampolineFn_ != nullptr; })
            .onSuccessCallback([this]() {
                printf("[PI] found address\n");
                //this->applyDetour();
            })
            .onFailureCallback([] { MessageBoxA(nullptr, "failed to find address", "()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL); })
            .build());
}

void ProcessInternal::buildPatches() {
    { // apply patch
        slack_ = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(bakkesTrampolineFn_) + 0x20);

        applyPatch_ = PatchBuilder()
            .name("Process Internal")

            // slack -> handle
            .setPosition(p::ptr_to_uintptr(slack_))
            .absoluteJump(p::ptr_to_uintptr(&handleFunction))

            // rl function -> slack
            .setPosition(p::ptr_to_uintptr(processInternalTargetFn_))
            .shortJump(p::ptr_to_uintptr(slack_))

            .finalize();
    }
    {   // remove patch
        // WARNING
        // do not run deferred without delaying PE's unpatch
        // this gets called on the next tick and PE's gone
        log_->debug("[PI] buildRemovePatch()");

        removePatch_ = PatchBuilder()
            .name("Process Internal Remove")
            .setPosition(p::ptr_to_uintptr(processInternalTargetFn_))
            .shortJump(p::ptr_to_uintptr(bakkesTrampolineFn_))
            .finalize();
    }

}

//auto ProcessInternal::getDispatchShutdownGate() -> AsyncGate* {
//    return dispatch_->getShutdownGate();
//}

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

void
ProcessInternal::applyDetour() {
    log_->debug("[PI] applyDetour()");


    //log_->logf_debug("[PI] target:     0x{:X}", p::ptr_to_uintptr(processInternalTargetFn_));
    //log_->logf_debug("[PI] trampoline: 0x{:X}", p::ptr_to_uintptr(bakkesTrampolineFn_));
    //log_->logf_debug("[PI] slack:      0x{:X}", p::ptr_to_uintptr(slack_));
    //log_->logf_debug("[PI] detour:     0x{:X}", p::ptr_to_uintptr(&handleFunction));

    // run inside main event loop
    // to prevent race conditions
    processEvent_->registerTask(TaskBuilder()
            .name("Apply ProcessInternal patch")
            .functionName("Function Engine.Interaction.Tick")
            .phase(HookPhase::Post)
            .callback([this](InvocationContext& ctx) {
                log_->debug("[PI] applying detour...");

                if (!mutex_->tryAcquire(1 /*ms*/)) {
                    log_->warn("[PI] Already patched -- nothing to apply");
                    return;
                }

                auto pm = PatchManager();
                pm.apply(applyPatch_);

                appliedGate_->setReady();
            })
            .once()
            .build());
}

void ProcessInternal::removeDetour() {
    // run inside main event loop
    // to prevent race conditions
    return;
    processEvent_->registerTask(TaskBuilder()
            .name("[PI] Remove")
            .functionName("Function Engine.Interaction.Tick")
            .phase(HookPhase::Post)
            .callback([this](InvocationContext& ctx) {
                log_->debug("[PI] removing patch...\n");

                auto pm = PatchManager();
                pm.apply(removePatch_);
                mutex_->release();
                removedGate_->setReady();

                log_->debug("[PI] removed\n");
            })
            .once()
            .build());
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

void __fastcall ProcessInternal::handleFunction(UObject* self, FFrame& stack, void* result) {
    if (!fastIsAcquired()) {
        reinterpret_cast<tProcessInternal>(bakkesTrampolineFn_)(self, stack, result);
        return;
    }

    auto log = AIM::getStaticResolver()->resolve<ILogger>();
    
    //if (log && self && self->GetFullName().find("SetString") != std::string::npos) {
    //    log->logf_debug("[CF] {:s} -> {:d}", self->GetFullName(), self->ObjectInternalInteger);
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
        reinterpret_cast<tProcessInternal>(bakkesTrampolineFn_)(self, stack, result);

        // run posthooks
        context = InvocationContext::makeProcessInternalContext(self, stack, result);
        dispatch->dispatchPost(self->ObjectInternalInteger, context);
        
        dispatch->dispatchGated(self->ObjectInternalInteger, context);
    } else {
        reinterpret_cast<tProcessInternal>(bakkesTrampolineFn_)(self, stack, result);
    }

    //if(self && self->GetFullName().find("Chat") != std::string::npos) {
    //    if (log) {
    //        log->logf_debug("[PI]: {:d} -> {:s}", self->ObjectInternalInteger, self->GetFullName());
    //    }
    //}
}