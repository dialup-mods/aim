#pragma once
#include <cassert>
#include <Windows.h>
#include <filesystem>
#include <Psapi.h>
#include "fmt/format.h"

#include "Runtime.h"
#include "SDK.h"
#include "ObjectProvider.h"

#include "fixtures/ObjectProviderFixture.h"
#include "fixtures/EngineLocatorFixture.h"

class RuntimeTest {
public:
    void run() {
        printf("Initializing Runtime...\n");
        HMODULE sdk = GetModuleHandleW(L"DialUp-SDK.dll");
        if (!sdk) {
            MessageBoxA(nullptr, "SDK not loaded", "Test error", MB_OK);
            return;
        }
        printf("\n\nRUNNING TESTS\n\n");

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

    std::shared_ptr<EngineLocatorFixture> engineLocator = std::make_shared<EngineLocatorFixture>();

    void populate() {
        printf("Get addresses\n");
        printf("gNameEntiresAddr valid: ");
        const uintptr_t fNameEntriesAddr = engineLocator->engineLocator.getFNameEntriesAddress();
        if (!fNameEntriesAddr) {
            printf("fail\n");
        } else {
            printf("pass\n");
        }

        printf("uObjectsAddr valid: ");
        const uintptr_t uObjectsAddr = engineLocator->engineLocator.getUObjectsAddress();
        if (!uObjectsAddr) {
            printf("fail\n");
        } else {
            printf("pass\n");
        }

        r::fname::game_pool::set(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddr));
        r::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(uObjectsAddr));

        printf("uObjects are populated: ");
        if (r::uobject::game_pool::isPopulated()) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }

        printf("fNameEntries are valid (ish): ");
        if (r::fname::game_pool::isValid()) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

    void checkGetUObjectsPtr() {
        printf("Check getUObjectsPtr() is not null: ");
        if (r::uobject::game_pool::ptr() != nullptr) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

    void checkGetFNameEntriesPtr() {
        printf("Check getFNameEntriesPtr() is not null: ");
        if (r::fname::game_pool::ptr() != nullptr) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

    void testClassLookup() {
        printf("[TEST] class lookup\n");
        {
            printf(" - Core.Object: ");
            auto* uObjCls = r::uclass::find("Class Core.Object");
            if (uObjCls && uObjCls->GetFullName() == "Class Core.Object") {
                printf("pass\n");
            } else {
                printf("fail\n");
            }
        }

        {
            printf(" - ProjectX.GFxDataStore: ");
            auto* cls = r::uclass::find("Class ProjectX.GFxDataStore_X");
            if (cls && cls->GetFullName() == "Class ProjectX.GFxDataStore_X") {
                printf("pass\n");
            } else {
                printf("fail\n");
            }
        }

        {
            printf(" - TAGame.GenericNotification_TA: ");
            auto* cls = r::uclass::find("Class TAGame.GenericNotification_TA");
            if (cls && cls->GetFullName() == "Class TAGame.GenericNotification_TA") {
                printf("pass\n");
            } else {
                printf("fail\n");
            }
        }
    }

    void testObjectIteration() {
        printf("[TEST] Object iteration\n");
        int count = 0;
        auto* objs = r::uobject::game_pool::ptr();

        for (int i = objs->size() - 1; i > 0; i--) {
            UObject* obj = objs->at(i);
            if (obj && obj->Class) {
                count++;
                if (count == objs->size() - 1) {
                    assert(obj->GetFullName() == "Class Core.Config_ORS");
                    printf("  First valid object: %s\n", obj->GetFullName().c_str());
                }
            }
        }
        printf("  Valid objects found: %i\n", count);
        assert(count > 0);
        printf("  ✓ Iteration works\n");
    }

    void testFindClass() {
        printf("[TEST] FindClass\n");
    }

    void testFindFunction() {
        printf("[TEST] FindFunction\n");

        {
            printf("  find method: \n");
            auto func = r::ufunction::find("Function Core.Object.GetAppSeconds");
            printf("  GetFullName: %s\n", func->GetFullName().c_str());
            if (func->NumParms == 1) {
                printf("  -> pass, returned:   %i\n", func->NumParms);
            } else {
                printf("  -> Fail. Expected: 1, Returned:   %i\n", func->NumParms);
            }
        }

        {
            printf("  find static function\n");
            auto func = r::ufunction::find("Function TAGame.OnlinePlayer_TA.ConvertError");
            printf("  GetFullName: %s\n", func->GetFullName().c_str());
            if (func->NumParms == 2) {
                printf("  -> pass, returned:   %i\n", func->NumParms);
            } else {
                printf("  -> Fail. Expected: 2, Returned:   %i\n", func->NumParms);
            }
        }

        {
            printf("  find function with lots of params\n");
            auto func = r::ufunction::find("Function TAGame.Tutorial_TA.NotifyKeyInput");
            printf("  GetFullName: %s\n", func->GetFullName().c_str());
            if (func->NumParms == 6) {
                printf("  -> pass, returned:   %i\n", func->NumParms);
            } else {
                printf("  -> Fail. Expected: 6, Returned:   %i\n", func->NumParms);
            }
        }
    }

    void testFName() {
        printf("[TEST] FName\n");
        printf("  construct an FName with a wstr arg\n");
        auto name = r::fname::game_pool::find(L"Bump");
        if (name.value().get().ToString() == "Bump") {
            printf("  ✓ got Bump\n");
        } else {
            printf("  fail\n");
        }
    }
};