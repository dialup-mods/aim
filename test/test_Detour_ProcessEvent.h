#pragma once
#include <cassert>
#include <Resolver.h>

#include "Runtime.h"
#include "SDK.h"
#include "PatchManager.h"
#include "CallFunction.h"
#include "PatchBuilder.h"
#include "PatchUtils.h"
#include "ProcessEvent.h"

#include "fixtures/ObjectProviderFixture.h"
#include "fixtures/ProcessEventFixture.h"
#include "fixtures/ProcessInternalFixture.h"
#include "fixtures/CallFunctionFixture.h"

//namespace AIM {
//    inline Resolver* staticResolver_;
//}

#pragma section(".slack", read, execute)
#pragma comment(linker, "/SECTION:.slack,ERW")
__declspec(allocate(".slack"))
inline unsigned char g_slack[0x4000] = {};
inline volatile void* slack = g_slack;

inline void __fastcall handleFunction(UObject* self, UFunction* fn, void* paramsPtr, void* resultPtr) {
    printf("handle\n");
}

class ProcessEventTest {
public:
    std::shared_ptr<ProcessEventFixture> processEvent       = std::make_shared<ProcessEventFixture>();
    std::shared_ptr<ProcessInternalFixture> processInternal = std::make_shared<ProcessInternalFixture>();
    std::shared_ptr<CallFunctionFixture> callFunction       = std::make_shared<CallFunctionFixture>();

    void run() {
        printf("\n\nRUNNING PROCESS EVENT TESTS\n\n");
        __try {
            // ProcessEvent
            getRLFunc();
            getBakkesTrampolineFn();
            buildPatches();
            applyDetour();
            removeDetour();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("big test exception\n");
        }
    }

    void getRLFunc() {
        auto* ret = processEvent->processEvent.getRLFn();
        printf("  [PE] getRLFunc() ret: %p\n", ret);
    }

    void getBakkesTrampolineFn() {
        auto* ret = processEvent->processEvent.getRLFn();
        printf("  [PE] getBakkesTrampolineFn() ret: %p\n", ret);
    }

    void buildPatches() {
        auto fn = reinterpret_cast<void**>(r::uclass::find("Class Core.Object")->VfTableObject.Ptr)[67];
        if (fn == nullptr || !safe::memory::isAddressAccessible(fn, sizeof(void*))) {
            printf("[PE] inaccessible memory\n");
            return;
        }

        PatchDefinition applyPatch_ = PatchBuilder()
            .name("Process Event")
            // create in reverse order

            .setPosition(patchutils::ptr_to_uintptr(slack))

            // capture and store bytes

            // vtable entry -> handle function
            .setPosition(patchutils::ptr_to_uintptr(fn))
            .absoluteJump(patchutils::ptr_to_uintptr(&handleFunction))

            .finalize();

        auto pm = PatchManager();
        pm.apply(applyPatch_, true);

        PatchDefinition removePatch_ = PatchBuilder()
            .name("Process Event Remove")

            // restore captured old bytes

            .finalize();
    }

    void applyDetour() {
        // check mutex is engaged
        // check fails gracefully when mutex already held
    }

    void removeDetour() {
        // check returns false when mutex is engaged
        // check fails gracefully when mutex is free (aka not detoured)
    }

};