#pragma once
#include <cassert>
#include "Runtime.h"
#include "SDK.h"
#include "AIM.h"

#include "fixtures/ObjectProviderFixture.h"
#include "fixtures/ProcessEventFixture.h"
#include "fixtures/ProcessInternalFixture.h"
#include "fixtures/CallFunctionFixture.h"

Resolver* AIM::staticResolver_ = nullptr;

class ProcessInternalTest {
public:
    std::shared_ptr<ProcessEventFixture> processEvent       = std::make_shared<ProcessEventFixture>();
    std::shared_ptr<ProcessInternalFixture> processInternal = std::make_shared<ProcessInternalFixture>();
    std::shared_ptr<CallFunctionFixture> callFunction       = std::make_shared<CallFunctionFixture>();

    void run() {
        printf("\n\nRUNNING PROCESS EVENT TESTS\n\n");
        __try {
            // ProcessEvent
            findAddress();
            findEnginePIAddress();
            buildPatches();
            applyDetour();
            shutdown();
            removeDetour();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("big test exception\n");
        }
    }

    void findEnginePIAddress() {
        void* ret = processInternal->processInternal.findEnginePIAddress();
        printf("  [PE] getRLFunc() ret: %p\n", ret);
    }

    void findAddress() {
        printf("  [PI] finding addresses\n");
        processInternal->processInternal.findAddress();
    }

    void buildPatches() {
        if (!processInternal->processInternal.buildPatches()) {
            printf("  [PE] FAIL building patches\n");
        }
    }

    void shutdown() {
        printf("  [PI] shutting down...");
        processInternal->processInternal.shutdown();
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