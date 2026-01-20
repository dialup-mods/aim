#include "SDK.h"
#include "ProcessEvent.h"
#include "AIM.h"
#include "Resolver.h"

#include "AsyncGate.h"
#include "ILogger.h"
#include "PatchUtils.h"
#include "PatchBuilder.h"
#include "Dispatch.h"

#include "../task/TaskBuilder.h"
#include "EventContext.h"
#include "TaskStructs.h"

#include <future>

//#include "GameWrapperProvider.h"
#include "PatchManager.h"
#include "MutexGuard.h"
#include "PluginFence.h"

#include "UEModelTypes.h"
#include "ValueResolver.h"
namespace p = patchutils;

ProcessEvent* ProcessEvent::instance_ = nullptr;

bool
ProcessEvent::init() {
    log_->debug("[PE] init()");

    //teardownFence_->block("PE");

    instance_ = this;
    mutex_->setName(getName() + "_Detour");

    auto checkFunc = [this]() {
        if (getRLFn() == nullptr) { return false; }
        if (getBakkesTrampolineFn() == nullptr) { return false; }

        BYTE* func = reinterpret_cast<BYTE*>(getRLFn());
        return (func[0] == 0xE9);  // Check if first byte is JMP rel32
    };

    auto setupFunc = [this]() {
        if (!this->buildPatches()) { return false; }

        this->applyDetour();

        return true;
    };

    std::thread([this, checkFunc, setupFunc]() {
        for (int i = 0; i < 100; ++i) {
            if (checkFunc()) {
                log_->debug("[PE] BakksMod patch detected.");
                setupFunc();
                return;
            }
            Sleep(6000);
        }

        log_->warn("[PE] Timeout waiting for BakkesMod patch.");
        //state_->setStatus(Failed);
    }).detach();

    return true;
}

bool
ProcessEvent::buildPatches() {
    log_->debug("[PE] build patches");
    slackFn_ = reinterpret_cast<uint8_t*>(getBakkesTrampolineFn()) + 0x30;
    
    //log_->logf_debug("[PE] RL base address: 0x{:X}", p::ptr_to_uintptr(patchutils::baseAddress()));
    //log_->logf_debug("[PE] slackSpace address: 0x{:X}", p::ptr_to_uintptr(slackFn_));
    //log_->logf_debug("[PE] detour handler address: 0x{:X}", p::ptr_to_uintptr(&handleFunction));
    //log_->logf_debug("[PE] vTableEntry address: 0x{:X}", p::ptr_to_uintptr(getRLFn()));
    //log_->logf_debug("[PE] getBakkesFunc() = 0x{:X}", p::ptr_to_uintptr(getBakkesTrampolineFn()));

    if (slackFn_ == nullptr || getRLFn() == nullptr) {
        log_->error("[PE] something is null");
        return false;
    }

    applyPatch_ = PatchBuilder()
        .name("Process Event")
    
        // slackFn_ -> handle
        .setPosition(p::ptr_to_uintptr(slackFn_))
        .absoluteJump(p::ptr_to_uintptr(&handleFunction))

        // vtable entry -> slackFn_
        .setPosition(p::ptr_to_uintptr(getRLFn()))
        .shortJump(p::ptr_to_uintptr(slackFn_))
    
        .finalize();

    removePatch_ = PatchBuilder()
        .name("Process Event Remove")

        // vTable entry -> previous jump
        .setPosition(p::ptr_to_uintptr(getRLFn()))
        .shortJump(p::ptr_to_uintptr(getBakkesTrampolineFn()))
        .finalize();
    
    return true;
}

//auto ProcessEvent::getDispatchShutdownGate() -> AsyncGate* {
//    return dispatch_->getShutdownGate();
//}

void ProcessEvent::shutdown() {
    dispatch_->shutdown();
}

void ProcessEvent::registerTask(std::shared_ptr<TaskDefinition> def) {
    dispatch_->registerTask(def);
}

void ProcessEvent::releaseTask(std::shared_ptr<TaskDefinition> def) {
    dispatch_->registerTask(def);
}

void ProcessEvent::clearTasks() {
    dispatch_->clearTasks();
}

void* ProcessEvent::getRLFn() {
    if (processEventVTableFn_ != nullptr) { return processEventVTableFn_; }

    auto fn = reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Ptr)[getVTableIndex()];
    if (fn == nullptr || !safe::memory::isAddressAccessible(fn, sizeof(void*))) {
        log_->error("[PE] inaccessible memory");
        return nullptr;
    }
    return fn;
}

// first jmp after vTable
void* ProcessEvent::getBakkesTrampolineFn() {
    if (bakkesTrampolineFn_ != nullptr) { return bakkesTrampolineFn_; }
    
    log_->debug("[PE] getBMTramp()");

    if (getRLFn() == nullptr) {
        log_->error("[PE] rlFn is null");
        return nullptr;
    }

    auto chain = patchutils::walkTrampolineChain(getRLFn());

    if (chain.size() < 2) {
        log_->error("[PE] Trampoline chain too short, expected at least 2 entries");
        return nullptr;
    }

    if (!safe::memory::isAddressAccessible(chain[1], sizeof(void*))) {
        log_->error("[PE] Chain[1] points to inaccessible memory");
        return nullptr;
    }

    bakkesTrampolineFn_ = chain[1];
    // unused bakkesModRealFn_ = chain[2];
    return bakkesTrampolineFn_;
}

void
ProcessEvent::applyDetour() {
    log_->debug("[PE] applyDetour()");
    
//    ExecuteOnGameWrapper([this](GameWrapper* gw) {
        log_->debug("[PE] applying patch...");

        if (!mutex_->tryAcquire(1/*ms*/)) {
            log_->warn("[PE] Already patched -- nothing to apply");
            return;
        }

        auto pm = PatchManager();
        pm.apply(applyPatch_);

        log_->debug("[PE] patch applied");
        appliedGate_->setReady();
//    });
}

void
ProcessEvent::removeDetour() {
    log_->debug("[PE] removeDetour()");

    mutex_->release();

    PatchManager().apply(removePatch_);

    //teardownFence_->release("PE");

//    ExecuteOnGameWrapper([this](GameWrapper* gw) {
//        printf("[PE] removing detour...\n");
//
//        auto pm = PatchManager();
//        pm.apply(removePatch_);
//
//        printf("[PE] detour removed\n");
//        this->mutex_->release();
//        removedGate_->setReady();
//    });
}

bool ProcessEvent::waitForUnlock(DWORD timeoutMs) const {
    return mutex_->waitForUnlock(timeoutMs);
}

bool ProcessEvent::fastIsAcquired() {
    if (instance_ && instance_->getMutex() && instance_->getMutex()->checkIsNamed()) {
        return instance_->getMutex()->fastIsAcquired();
    }
    return false;
}


void __fastcall ProcessEvent::handleFunction(UObject* self, UFunction* fn, void* params, void* result) {
    if (!fastIsAcquired()) {
        reinterpret_cast<tProcessEvent>(bakkesTrampolineFn_)(self, fn, params, result);
        return;
    }

    // todo: add backpressure monitoring
    // start frame timing
    auto frameStart = std::chrono::steady_clock::now();

    // teardown guard - since handleFunction is static we can't guarantee lifetime
    if (auto dispatch = AIM::getStaticResolver()->resolve<Dispatch>("PE-Dispatch")) {

        // todo: add backpressure monitoring
        //if (dispatch->isOverBudget(frameStart)) {
        //    dispatch->deferTask(fn->ObjectInternalInteger, context);
        //    return;
        //}
        if (fn && fn->GetFullName().find("GetMapName") != std::string::npos) {
            struct P {
                uint32_t bIncludePrefix;
                FString ReturnValue = {};
            };


            printf("[PE HOOK] GetMapName hit on %s\n",
                self->GetFullName().c_str());
            printf("[PE HOOK] GetMapName fn: %s\n", fn->GetFullName().c_str());

            auto model = UEModel::assignClassFromRawPtr(fn);
            auto fnObj = static_cast<UFunctionEntry*>(model.get());
            printf("[PE HOOK] Fn Type: %s\n", model->getFullName().c_str());
            fnObj->iterateDependencies();
            //for (auto param : fnObj->getAllParams()) {
            //    uint8_t* base = static_cast<uint8_t*>(params);
            //    void* valuePtr = base + param->getOffset();
            //    ResolvedValue out;
            //    param->resolveInto(out, valuePtr);
            //    if (!out.primitiveStr.empty()) {
            //        printf(" resolved: %s\n", out.primitiveStr.c_str());
            //    }
            //}
        }

        // run prehooks
        auto context = InvocationContext::makeProcessEventContext(self, fn, params);
        if (auto shouldBlock = dispatch->dispatchPre(fn->ObjectInternalInteger, context)) {
            return;
        }

        // call original function
        reinterpret_cast<tProcessEvent>(bakkesTrampolineFn_)(self, fn, params, result);

        // create a context with fn result
        context = InvocationContext::makeProcessEventContext(self, fn, params, result);
        dispatch->dispatchPost(fn->ObjectInternalInteger, context);

        // conditional hooks
        dispatch->dispatchGated(fn->ObjectInternalInteger, context);
    } else {
        // dispatcher is MIA fallback
        reinterpret_cast<tProcessEvent>(bakkesTrampolineFn_)(self, fn, params, result);
    }
}