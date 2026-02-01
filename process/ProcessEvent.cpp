#include "AIM.h"
#include "Exception.h"
#include "Printer.h"
#include "SDK.h"
#include "ProcessEvent.h"
#include "Resolver.h"
#include "Detour.h"

#include "AsyncGate.h"
#include "ILogger.h"
#include "PatchUtils.h"
#include "Dispatch.h"

#include "TaskBuilder.h"
#include "EventContext.h"
#include "TaskStructs.h"

#include <future>

//#include "GameWrapperProvider.h"
#include "MutexGuard.h"
#include "PluginFence.h"

#include "SDK.h"
#include "AllModelTypes.h"
#include "UEModel.h"
#include "ValueResolver.h"
#include "Runtime.h"
#include "IModule.h"
namespace p = patchutils;
using r = Runtime;

ProcessEvent* ProcessEvent::instance_ = nullptr;

struct ReentrancyGuard {
    bool& flag;
    ReentrancyGuard(bool& f) : flag(f) { flag = true; }
    ~ReentrancyGuard() { flag = false; }
};

bool
ProcessEvent::init() {
    log_->debug("[PE] init()");

    instance_ = this;
    mutex_->setName(getName() + "_Detour");

    auto checkFunc = [this]() {
        if (getRLFn() == nullptr) { return false; }
#ifdef PATCH_WITH_BAKKESMOD
        if (getBakkesTrampolineFn() == nullptr) { return false; }

        BYTE* func = reinterpret_cast<BYTE*>(getRLFn());
        return (func[0] == 0xE9);  // Check if first byte is JMP rel32
#else
        return true;
#endif
    };

    auto setupFunc = [this]() {
        if (!this->buildPatches()) { return false; }

        this->applyDetour();

        return true;
    };

    std::thread([this, checkFunc, setupFunc]() {
        for (int i = 0; i < 100; ++i) {
            if (checkFunc()) {
                log_->debug("[PE] BakkesMod patch detected.");
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
    return true;
}

//auto ProcessEvent::getDispatchShutdownGate() -> AsyncGate* {
//    return dispatch_->getShutdownGate();
//}

void ProcessEvent::shutdown() {
    removeDetour();
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

    auto fn = reinterpret_cast<void**>(r::uclass::find("Class Core.Object")->VfTableObject.Ptr)[getVTableIndex()];
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

    if (chain.size() < 3) {
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

    teardownFence_->block("PE");

    if (!mutex_->tryAcquire(1/*ms*/)) {
        log_->warn("[PE] Already patched -- nothing to apply");
        return;
    }

    log_->debug("[PE] applying detour at: {}", getRLFn());
    detour_->attach(getRLFn(), (void*)&handleFunction);

    log_->debug("[PE] detoured");
    appliedGate_->setReady();
}

void
ProcessEvent::removeDetour() {
    log_->debug("[PE] removeDetour()");
    dispatch_->shutdown();
    mutex_->release();
    // wait for any in-flight handlers to finish
    if (!mutex_->waitForUnlock(5000)) {
        log_->error("Timeout waiting for ProcessEvent handlers to finish");
        return;
    }
    detour_->detach();
    //removedGate_->setReady();
    teardownFence_->release("PE");
    std::this_thread::sleep_for(std::chrono::seconds(1));
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

void* ProcessEvent::getTrampoline() {
    if (instance_ && instance_->detour_->getTrampoline()) {
        return instance_->detour_->getTrampoline();
    }
    return nullptr;
}

void ProcessEvent::printStr(const std::string& prefix, const std::string& str) {
    __try {
        printf("  -> %s: %s\n", prefix.c_str(), str.c_str());
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("[SEH] assploded\n");
    }
}

auto ProcessEvent::convert(void* params) -> uint8_t* {
    __try {
        return static_cast<uint8_t*>(params);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("[SEH] assploded on convert\n");
        return nullptr;
    }
}

// Parms is just a blob
// UProperty::Offset is the meaning
// The function object provides the schema
// PE source: https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Core/Src/UnCorSc.cpp#L6270
void __fastcall ProcessEvent::handleFunction(UObject* self, UFunction* fn, void* paramsPtr, void* resultPtr) {
    static thread_local bool inHandler = false;
    if (inHandler || !fastIsAcquired()) {
        reinterpret_cast<tProcessEvent>(getTrampoline())(self, fn, paramsPtr, resultPtr);
        return;
    }

    ReentrancyGuard guard(inHandler);

    auto selfName = r::uobject_utils::getFullName(self);
    auto fnName = r::uobject_utils::getFullName(fn);

    if (fn && fnName == "Function ProjectX.EOSMetrics_X.HandleCrash") {
        return;
    }

    if (fn && fnName.find("ProjectX.CrashReport") != std::string::npos) {
        return;
    }

    //if (fn && fn->GetFullName() == "Function TAGame.GFxData_Chat_TA.OnQuickChatAdded") {
    if (fn && fnName.find("Notification") != std::string::npos) {
        printf("[PE] (pre):\n");
        printf("     -> fn     %s\n", fnName.c_str());
        printf("     -> ptr    %p\n", fn->VfTableObject.Ptr);
        printf("     -> self   %s\n", selfName.c_str());
        printf("     -> ptr    %p\n", self->VfTableObject.Ptr);
        printf("     -> params %p\n", static_cast<void*>(&paramsPtr));

    //    const auto base = static_cast<uint8_t*>(paramsPtr);

    //    for (auto prop = static_cast<UProperty*>(fn->Children);
    //         prop;
    //         prop = static_cast<UProperty*>(prop->Next))
    //    {
    //        if (!(prop->PropertyFlags & CPF_Parm)) { continue; }

    //        for (int i = 0; i < prop->ArrayDim; ++i) {
    //            printf("i: %i\n", i);
    //            void* valuePtr = base + prop->Offset + i * prop->ElementSize;
    //            ResolvedValue out;
    //            auto propEntry = UEModel::assignClass(prop);
    //            printf(" %s\n", propEntry->getCanonicalTypeStr().c_str());
    //            propEntry->resolveInto(out, valuePtr);
    //            Printer::debugPrint(out);
    //        }
    //    }
    //    //{
    //    //    const auto fnObj = UEModel::assignClassFromRawPtr(fn).get()->as<UFunctionEntry>();
    //    //    int i = 0;
    //    //    for (auto param : fnObj->getAllParams()) {
    //    //        printf(" sdk i: %i\n", i++);
    //    //        auto valuePtr = param->getValuePtr(paramsPtr);
    //    //        ResolvedValue out;
    //    //        printf(" %s\n", param->getCanonicalTypeStr().c_str());
    //    //        param->resolveInto(out, valuePtr);
    //    //        Printer::debugPrint(out);
    //    //}
    //}

    //{
        //auto lambda = ([&]() {
        //    if (fn && fn->GetFullName() == "Function TAGame.GFxData_Chat_TA.OnQuickChatAdded")
        //    { // fn params
        //        const auto model = UEModel::assignClassFromRawPtr(fn);
        //        const auto fnObj = static_cast<UFunctionEntry*>(model.get());
        //        printStr("pre fn", fnObj->getFullName());
        //        for (const auto fnParam : fnObj->getAllParams()) {
        //            const auto base = convert(params);
        //            void* valuePtr = base + fnParam->getOffset();
        //            ResolvedValue out;
        //            fnParam->resolveInto(out, valuePtr);
        //            Printer::debugPrint(out);
        //        }
        //    }
        //});
        //auto func = lambda;
        //auto safeFunc = safe::makeSEHSafe([&]() {
        //    func();
        //});
        //safeFunc();
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
        // run prehooks
        auto context = InvocationContext::makeProcessEventContext(self, fn, paramsPtr);
        if (auto shouldBlock = dispatch->dispatchPre(fn->ObjectInternalInteger, context)) {
            return;
        }

        // call original function
        reinterpret_cast<tProcessEvent>(getTrampoline())(self, fn, paramsPtr, resultPtr);

        // create a context with fn result
        context = InvocationContext::makeProcessEventContext(self, fn, paramsPtr, resultPtr);
        dispatch->dispatchPost(fn->ObjectInternalInteger, context);

        // conditional hooks
        dispatch->dispatchGated(fn->ObjectInternalInteger, context);

        if (fn && fnName.find("Notification") != std::string::npos) {
            printf("[PE] (post):\n");
            printf("     -> fn     %s\n", fnName.c_str());
            printf("     -> ptr    %p\n", fn->VfTableObject.Ptr);
            printf("     -> self   %s\n", selfName.c_str());
            printf("     -> ptr    %p\n", self->VfTableObject.Ptr);
            printf("     -> params %p\n", static_cast<void*>(&paramsPtr));

            if (fn && paramsPtr && fn->ParmsSize != 0) {
                const auto base = static_cast<uint8_t*>(paramsPtr);

                for (UProperty* prop = static_cast<UProperty*>(fn->Children);
                     prop;
                     prop = static_cast<UProperty*>(prop->Next))
                {
                    if (!(prop->PropertyFlags & CPF_Parm)) { continue; }

                    for (int i = 0; i < prop->ArrayDim; ++i) {
                        printf("i: %i\n", i);
                        void* valuePtr = base + prop->Offset + i * prop->ElementSize;
                        ResolvedValue out;
                        auto propEntry = UEModel::assignClass(prop);
                        printf(" %s\n", propEntry->getCanonicalTypeStr().c_str());
                        propEntry->resolveInto(out, valuePtr);
                        Printer::debugPrint(out);
                    }
                }
            }
        }

    } else {
        // dispatcher is MIA fallback
        reinterpret_cast<tProcessEvent>(getTrampoline())(self, fn, paramsPtr, resultPtr);
    }
}