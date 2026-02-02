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

void
CallFunction::init() {
    log_->debug("[CF] init");

    instance_ = this;
    mutex_->setName(getName() + "_Detour");

    buildPatches();
    applyDetour();
}

void
CallFunction::buildPatches() {
    slack_ = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(getBakkesTrampolineFn()) + 0x20);

    //log_->logf_debug("[CF] RL base address: 0x{:X}", p::ptr_to_uintptr(patchutils::baseAddress()));
    //log_->logf_debug("[CF] slackSpace address: 0x{:X}", p::ptr_to_uintptr(slack_));
    //log_->logf_debug("[CF] detour target address: 0x{:X}", p::ptr_to_uintptr(&handleFunction));
    //log_->logf_debug("[CF] vTableEntry address: 0x{:X}", p::ptr_to_uintptr(getRLFn()));
    //log_->logf_debug("[CF] bakkes address: 0x{:X}", p::ptr_to_uintptr(getBakkesTrampolineFn()));
}

//auto CallFunction::getDispatchShutdownGate() -> AsyncGate* {
//    return dispatch_->getShutdownGate();
//}

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

void* CallFunction::getRLFn() {
    if (callFunctionFn_ == nullptr) {
        callFunctionFn_ = patchutils::patternScan(patternBytes_);
    }
    return callFunctionFn_;
}

void* CallFunction::getBakkesTrampolineFn() {
    if (bakkesTrampolineFn_ == nullptr) {
        bakkesTrampolineFn_ = patchutils::patternScan(patternBytes_, true); // resolve jmp
    }
    return bakkesTrampolineFn_;
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

static bool chat = false;
void __fastcall CallFunction::handleFunction(UObject* self, FFrame& stack, void* result, UFunction* fn) {
    if (!fastIsAcquired()) {
        reinterpret_cast<tCallFunction>(bakkesTrampolineFn_)(self, stack, result, fn);
        return;
    }
    auto log = AIM::getStaticResolver()->resolve<ILogger>();

    //        bool dump = false;
    //        if(!fn->GetFullName().find("DataStore") != std::string::npos) {
    //            // fixme g_log
    //            //log_->debug(fn->GetFullName() + " found");
    //            dump = true;
    //            ::dumpRegisterSpy();
    //        }

    //            logf_debug("Stack Address: {:#018X}", reinterpret_cast<uintptr_t>(&stack));
    //            logf_debug("First bytes: {:#04X} {:#04X} {:#04X} {:#04X}", 
    //                        reinterpret_cast<uint8_t*>(&stack)[0], 
    //                        reinterpret_cast<uint8_t*>(&stack)[1], 
    //                        reinterpret_cast<uint8_t*>(&stack)[2], 
    //                        reinterpret_cast<uint8_t*>(&stack)[3]);
    //            
    //            logf_debug("PreviousFrame: {:#018X}", reinterpret_cast<uintptr_t>(stack.PreviousFrame));
    //            logf_debug("Locals Address: {:#018X}", reinterpret_cast<uintptr_t>(stack.Locals));
    //
    //            logf_debug("Function flags: 0x{:X}", fn->FunctionFlags);
    //            if (fn->FunctionFlags & FUNC_HasOutParms) {
    //                logger->debug("has output params");
    //            }
    //
    //            std::vector<CustomOutParmRec> paramData;
    //            for (UField* field = fn->Children; field; field = field->Next) {
    //                // Check if the field is a UProperty
    //                if (!field->IsA(UProperty::StaticClass())) {
    //                    logf_debug("Skipping non-UProperty field: {}", field->GetName());
    //                    continue;
    //                }
    //            
    //                // Cast to UProperty
    //                UProperty* prop = reinterpret_cast<UProperty*>(field);
    //                logf_debug("Checking property: {}", prop->GetName());
    //            
    //                // Check if the property is a UBoolProperty
    //                if (prop->IsA(UBoolProperty::StaticClass())) {
    ////                    UBoolProperty* boolProp = static_cast<UBoolProperty*>(prop);
    //            
    //                    // Read the value from the BitMask (this is the boolean value stored bitwise)
    ////                    bool value = (*reinterpret_cast<uint64_t*>(paramData) & boolProp->BitMask) != 0;
    //            
    //                    logf_debug("→ bool property: {} = {}", prop->GetName());//, value);
    //                }
    //                else {
    //                    logf_debug("→ Unknown property: {}", prop->GetName());
    //                }
    //            }
        
    /* 
     for (auto& outParm : OutParms) {
         // Manually copy the output values to the tracked addresses
         if (outParm.Property->PropertyFlags & UEPropertyFlags::CPF_Int) {
             *reinterpret_cast<int32_t*>(outParm.ParamAddr) = *(reinterpret_cast<int32_t*>(NewStack.Locals + outParm.Property->Offset));
         }
         else if (outParm.Property->PropertyFlags & CPF_Float) {
             *reinterpret_cast<float*>(outParm.ParamAddr) = *(reinterpret_cast<float*>(NewStack.Locals + outParm.Property->Offset));
         }
         else if (outParm.Property->PropertyFlags & CPF_Bool) {
             *reinterpret_cast<bool*>(outParm.ParamAddr) = *(reinterpret_cast<bool*>(NewStack.Locals + outParm.Property->Offset)) != 0;
         }
         // Add other types as needed...
     }
     */
        
    //            if (stack.Locals) {
    //                logger->debug("stack has locals: " + stringutil::toHex(reinterpret_cast<uintptr_t>(stack.Locals)));
    //                
    //                uint8_t* base = stack.Locals;
    //                
    //                if (fn->Children == nullptr) {
    //                    logger->debug("children null");
    //                } else {
    //                    for (UField* field = fn->Children; field; field = field->Next) {
    //                        if (field == nullptr) {
    //                            logger->debug("field null");
    //                            continue;
    //                        }
    //                        if (!field->IsA(UProperty::StaticClass())) {
    //                            logger->debug("field is not a class property");
    //                            continue;
    //                        }
    //                    
    //                        UProperty* prop = reinterpret_cast<UProperty*>(field);
    //                        if (!prop) {
    //                            logf_debug("  (not a property)");
    //                            continue;
    //                        }
    //        
    //                        // Safeguard: Only proceed if the offset is reasonable
    //                        if (prop->Offset <= 0 || prop->Offset > 0x1000) {
    //                            logf_debug("  Invalid property offset: {}. Skipping.", prop->Offset);
    //                            continue;
    //                        }
    //        
    //                        logf_debug("Prop: {} Offset: {}", prop->GetName(), prop->Offset);
    //        
    //                        uint8_t* paramData = base + prop->Offset;
    //        
    //                        // Check if paramData is valid
    //                        if (paramData == nullptr) {
    //                            logf_debug("  Invalid parameter data (paramData is null). Skipping.");
    //                            continue;
    //                        }
    //        
    //                        if (prop->PropertyFlags & 0x8) {  // CPF_Int
    //                            int32_t value = *reinterpret_cast<int32_t*>(paramData);
    //                            logf_debug(" → int {} = {}", prop->GetName(), value);
    //                        }
    //                        else if (prop->PropertyFlags & 0x10) {  // CPF_Float
    //                            float value = *reinterpret_cast<float*>(paramData);
    //                            logf_debug(" → float {} = {}", prop->GetName(), value);
    //                        }
    //                        else if (prop->PropertyFlags & 0x20) {  // CPF_Bool
    //                            bool value = *reinterpret_cast<uint8_t*>(paramData) != 0;
    //                            logf_debug(" → bool {} = {}", prop->GetName(), value);
    //                        }
    //                        else {
    //                            logf_debug(" → unknown {} at offset {}", prop->GetName(), prop->Offset);
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //if (log && fn && fn->GetFullName().find("Chat") != std::string::npos) {
    //    log->logf_debug("[CF] {:s} -> {:d}", fn->GetFullName(), fn->ObjectInternalInteger);
    //}

    if (auto dispatch = AIM::getStaticResolver()->resolve<Dispatch>("CF-Dispatch")) {
        auto ctx = InvocationContext::makeCallFunctionContext(self, stack, nullptr, fn);
        if (dispatch->dispatchPre(fn->ObjectInternalInteger, ctx)) {
            // blocking was requested
            printf("[CF] should block. Returning");
            return;
        }
    
        reinterpret_cast<tCallFunction>(bakkesTrampolineFn_)(self, stack, result, fn);
    
        ctx = InvocationContext::makeCallFunctionContext(self, stack, result, fn);
        dispatch->dispatchPost(fn->ObjectInternalInteger, ctx);
        
        dispatch->dispatchGated(fn->ObjectInternalInteger, ctx);
    } else {
        reinterpret_cast<tCallFunction>(bakkesTrampolineFn_)(self, stack, result, fn);
    }
}