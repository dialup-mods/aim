#pragma once
#include <cassert>
#include <Windows.h>
#include <filesystem>
#include <Psapi.h>
#include "fmt/format.h"

#include "Runtime.h"
#include "SDK.h"

#include "fixtures/EngineLocatorFixture.h"
using r = Runtime;

class RuntimeTest {
public:
    void run() {
        printf("\n\nRUNNING TESTS\n\n");

        HMODULE sdk = GetModuleHandleW(L"DialUp-SDK.dll");
        if (!sdk) {
            MessageBoxA(nullptr, "SDK not loaded", "Test error", MB_OK);
            return;
        }

        // create runtime
        r::create();

        __try {
            populate();
            checkGetUObjectsPtr();
            checkGetFNameEntriesPtr();
            testObjectIteration();
            testClassLookup();
            testFindFunction();
            testFName();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("big test exception\n");
        }

        printf("\n\nTESTING COMPLETE\n\n");
    }

    void populate() {
        printf("\n[TEST] Populate Runtime\n");

        std::shared_ptr<EngineLocatorFixture> engineLocator = std::make_shared<EngineLocatorFixture>();

        printf("    - fNameEntiresAddr valid: ");
        const uintptr_t fNameEntriesAddr = engineLocator->engineLocator.getFNameEntriesAddress();
        if (!fNameEntriesAddr) {
            printf("FAIL\n");
        } else {
            printf("pass\n");
        }

        printf("    - uObjectsAddr valid: ");
        const uintptr_t uObjectsAddr = engineLocator->engineLocator.getUObjectsAddress();
        if (!uObjectsAddr) {
            printf("FAIL\n");
        } else {
            printf("pass\n");
        }

        r::fname::game_pool::set(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddr));
        r::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(uObjectsAddr));

        printf("    - uObjects are populated: ");
        if (r::uobject::game_pool::isPopulated()) {
            printf("pass\n");
        } else {
            printf("FAIL\n");
        }

        printf("    - fNameEntries are valid (ish): ");
        if (r::fname::game_pool::isValid()) {
            printf("pass\n");
        } else {
            printf("FAIL\n");
        }
    }

    void checkGetUObjectsPtr() {
        printf("    - getUObjectsPtr() is not null: ");
        if (r::uobject::game_pool::ptr() != nullptr) {
            printf("pass\n");
        } else {
            printf("FAIL\n");
        }
    }

    void checkGetFNameEntriesPtr() {
        printf("    - getFNameEntriesPtr() is not null: ");
        if (r::fname::game_pool::ptr() != nullptr) {
            printf("pass\n");
        } else {
            printf("FAIL\n");
        }
    }

    void testClassLookup() {
        printf("\n[TEST] class lookup\n");
        {
            printf("    - Core.Object: ");
            auto* uObjCls = r::uclass::find("Class Core.Object");
            if (uObjCls && uObjCls->GetFullName() == "Class Core.Object") {
                printf("pass\n");
            } else {
                printf("FAIL\n");
            }
        }

        {
            printf("    - ProjectX.GFxDataStore: ");
            auto* cls = r::uclass::find("Class ProjectX.GFxDataStore_X");
            if (cls && cls->GetFullName() == "Class ProjectX.GFxDataStore_X") {
                printf("pass\n");
            } else {
                printf("FAIL\n");
            }
        }

        {
            printf("    - TAGame.GenericNotification_TA: ");
            auto* cls = r::uclass::find("Class TAGame.GenericNotification_TA");
            if (cls && cls->GetFullName() == "Class TAGame.GenericNotification_TA") {
                printf("pass\n");
            } else {
                printf("FAIL\n");
            }
        }
    }

    void testObjectIteration() {
        printf("\n[TEST] Object iteration\n");
        int count = 0;
        auto* objs = r::uobject::game_pool::ptr();

        for (int i = objs->size() - 1; i > 0; i--) {
            UObject* obj = objs->at(i);
            if (obj && obj->Class) {
                count++;
                if (count == objs->size() - 1) {
                    assert(obj->GetFullName() == "Class Core.Config_ORS");
                    printf("    First valid object: %s\n", obj->GetFullName().c_str());
                }
            }
        }
        printf("    Valid objects found: %i\n", count);
        assert(count > 0);
        printf("    ✓ Iteration works\n");
    }

    void testFindFunction() {
        printf("\n[TEST] FindFunction\n");
        {
            printf("    - find method: \n");
            auto func = r::ufunction::find("Function Core.Object.GetAppSeconds");
            printf("      -> GetFullName: %s\n", func->GetFullName().c_str());
            if (func->NumParms == 1) {
                printf("  -> pass, returned:   %i\n", func->NumParms);
            } else {
                printf("  -> FAIL. Expected: 1, Returned:   %i\n", func->NumParms);
            }
        }

        {
            printf("    find static function\n");
            auto func = r::ufunction::find("Function TAGame.OnlinePlayer_TA.ConvertError");
            printf("    -> GetFullName: %s\n", func->GetFullName().c_str());
            if (func->NumParms == 2) {
                printf("    -> pass, returned:   %i\n", func->NumParms);
            } else {
                printf("    -> FAIL. Expected: 2, Returned:   %i\n", func->NumParms);
            }
        }

        {
            printf("  find function with lots of params\n");
            auto func = r::ufunction::find("Function TAGame.Tutorial_TA.NotifyKeyInput");
            printf("    -> GetFullName: %s\n", func->GetFullName().c_str());
            if (func->NumParms == 6) {
                printf("    -> pass, returned:   %i\n", func->NumParms);
            } else {
                printf("    -> FAIL. Expected: 6, Returned:   %i\n", func->NumParms);
            }
        }
    }

    void testFName() {
        printf("\n[TEST] FName\n");
        printf("  construct an FName with a wstr arg\n");
        auto name = r::fname::game_pool::find(L"Bump");
        if (name.value().get().ToString() == "Bump") {
            printf("    ✓ got Bump\n");
        } else {
            printf("    FAIL\n");
        }
    }
};