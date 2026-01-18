#pragma once
#include <cassert>

#ifndef SDK_DLL
#error "SDK_DLL must be defined at compile time"
#endif

#include <Windows.h>
#include <filesystem>
#include <Psapi.h>

#include "ILogger.h"
#include "Runtime.h"
#include "SDK.h"

#include "ObjectProvider.h"

inline auto findPattern(HMODULE module, const unsigned char* pattern, const char* mask) -> uintptr_t {
    MODULEINFO info = {};
    GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

    auto start = reinterpret_cast<uintptr_t>(module);
    const size_t length = info.SizeOfImage;

    size_t pos = 0;
    const size_t maskLength = std::strlen(mask) - 1;

    for (uintptr_t retAddress = start; retAddress < start + length; retAddress++) {
        if (*reinterpret_cast<unsigned char*>(retAddress) == pattern[pos] || mask[pos] == '?') {
            if (pos == maskLength) {
                return (retAddress - maskLength);
            }
            pos++;
        } else {
            retAddress -= pos;
            pos = 0;
        }
    }
    return NULL;
}

inline auto getFNameEntriesAddress() -> uintptr_t {
    constexpr unsigned char fNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
    char fNamesMask[] = "??????xx??xxxxxx";

    const uintptr_t fNameEntriesAddress = findPattern(GetModuleHandleW(L"RocketLeague.exe"), fNamesPattern, fNamesMask);
    return fNameEntriesAddress;
}

inline auto getUObjectsAddress() -> uintptr_t {
    const auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"RocketLeague.exe"));
    const auto uObjectsAddress = getFNameEntriesAddress() + 0x48;
    return uObjectsAddress;
}

class RuntimeTest {
public:


    void printName(UObject* uObject) const {
        printf(" GetFullName(): %s\n", uObject->GetFullName().c_str());
    }

    /// test object provider
    template<typename T>
    T* getInstanceOf() const {
        if (!std::is_base_of_v<UObject, T>) { return nullptr; }

        auto& objects = Runtime::getUObjects();
        printf("num objects: %i\n", objects.size());

        for (int i = objects.size() - 1; i > 0; i--) {
            UObject* uObject = Runtime::getUObjectsPtr()->at(i);
            if (!uObject) { continue; }
            //__try {
            //    printName(uObject);
            //} __except (EXCEPTION_EXECUTE_HANDLER) {
            //    printf("exception\n");
            //}
            if (!uObject->IsA<T>()) { continue; }

            //if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }

            printf("found at index: %i/%i\n", i, Runtime::getUObjects().size());
            return static_cast<T*>(uObject);
        }

        return nullptr;
    }

    template<typename T>
    std::vector<T*> getAllInstancesOf() const {
        std::vector<T*> found;
        if (!std::is_base_of_v<UObject, T>) { return {}; }

        auto& objects = Runtime::getUObjects();
        printf("num objects: %i\n", objects.size());

        for (int i = objects.size() - 1; i > 0; i--) {
            UObject* uObject = Runtime::getUObjectsPtr()->at(i);
            if (!uObject) { continue; }
            //__try {
            //    printName(uObject);
            //} __except (EXCEPTION_EXECUTE_HANDLER) {
            //    printf("exception\n");
            //}
            if (!uObject->IsA<T>()) { continue; }

            //if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }

            printf("found at index: %i/%i\n", i, Runtime::getUObjects().size());
            found.emplace_back(static_cast<T*>(uObject));
        }

        return found;
    }

    void run() {
        printf("Initializing Runtime...\n");
        HMODULE sdk = GetModuleHandleW(L"DialUp-SDK.dll");
        if (!sdk) {
            MessageBoxA(nullptr, "SDK not loaded", "Test error", MB_OK);
            return;
        }
        Runtime::create();

        __try {
            populate();
            checkGetUObjectsPtr();
            checkGetFNameEntriesPtr();
            testObjectIteration();
            testProcessEventDirect();
            //testClassLookup();
            //testFindFunction();

            //testGetInstanceOf();
            //testFindFunction();
            //testCallMethodOnObject();
            //testDatastoreDiscovery();

            //testFName();
            //testUObjectUtil();

            // object provider tests
            //testNotification();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("big test exception\n");
        }
    }

    void populate() {
        printf("Get addresses\n");
        printf("gNameEntiresAddr valid: ");
        const uintptr_t fNameEntriesAddr = getFNameEntriesAddress();
        if (!fNameEntriesAddr) {
            printf("fail\n");
        } else {
            printf("pass\n");
        }

        printf("uObjectsAddr valid: ");
        const uintptr_t uObjectsAddr = getUObjectsAddress();
        if (!uObjectsAddr) {
            printf("fail\n");
        } else {
            printf("pass\n");
        }

        Runtime::setFNameEntries(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddr));
        Runtime::setUObjects(reinterpret_cast<TArray<UObject*>*>(uObjectsAddr));

        printf("uObjects are populated: ");
        if (Runtime::areUObjectsPopulated()) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }

        printf("fNameEntries are valid (ish): ");
        if (Runtime::areFNameEntriesValid()) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

    void checkGetUObjectsPtr() {
        printf("Check getUObjectsPtr() is not null: ");
        if (Runtime::getUObjectsPtr() != nullptr) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

    void checkGetFNameEntriesPtr() {
        printf("Check getFNameEntriesPtr() is not null: ");
        if (Runtime::getFNameEntriesPtr() != nullptr) {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

    void testClassLookup() {
        printf("[TEST] class lookup\n");
        {
            printf(" - Core.Object: ");
            auto* uObjCls = Runtime::findClass("Class Core.Object");
            if (uObjCls && uObjCls->GetFullName() == "Class Core.Object") {
                printf("pass\n");
            } else {
                printf("fail\n");
            }
        }

        {
            printf(" - ProjectX.GFxDataStore: ");
            auto* cls = Runtime::findClass("Class ProjectX.GFxDataStore_X");
            if (cls && cls->GetFullName() == "Class ProjectX.GFxDataStore_X") {
                printf("pass\n");
            } else {
                printf("fail\n");
            }
        }

        {
            printf(" - TAGame.GenericNotification_TA: ");
            auto* cls = Runtime::findClass("Class TAGame.GenericNotification_TA");
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
        auto* objs = Runtime::getUObjectsPtr();

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
            auto func = Runtime::findFunction("Function Core.Object.GetAppSeconds");
            printf("  GetFullName: %s\n", func->GetFullName().c_str());
            printf("  NumParams:   %i\n", func->NumParms);
        }

        {
            printf("  find static function\n");
            auto func = Runtime::findFunction("Function Core.Object.GetAppSeconds");
            printf("  GetFullName: %s\n", func->GetFullName().c_str());
            printf("  NumParams:   %i\n", func->NumParms);
        }

        {
            printf("  find function with lots of params\n");
            auto func = Runtime::findFunction("Function Core.Object.GetAppSeconds");
            printf("  GetFullName: %s\n", func->GetFullName().c_str());
            printf("  NumParams:   %i\n", func->NumParms);
        }


        {
            auto* func = Runtime::findFunction("Function Core.Object.ProcessEvent");
            printf(" - ProcessEvent: %s\n", func ? "found" : "NOT FOUND");

            if (func) {
                printf("   Address: %p\n", func);
                printf("   Flags: 0x%llu\n", func->FunctionFlags);
                printf("   Func pointer: %p\n", func->Func.Ptr);
            }
        }

        {
            auto* func = Runtime::findFunction("Function Engine.WorldInfo.GetGameClass");
            auto* uobj = Runtime::findClass("Class Core.Object");
            printf(" - GetGameClass: %s\n", func ? "found" : "NOT FOUND");
            UClass* res;
            uobj->ProcessEvent(func, nullptr, &res);
            if (res != nullptr) {
                printf("  res: %s", res->GetFullName().c_str());
            }
        }

    }

    void testCallMethodOnObject() {
        printf("[TEST] Call Method on Object\n");

        auto* cls = Runtime::findClass("Class Core.Object");

        {
            printf("  searching: GFxDataStore_X\n");
            auto datastores = getAllInstancesOf<UGFxDataStore_X>();
            for (auto* ds : datastores) {
                printf("  found obj: %s\n", ds->GetFullName().c_str());
                auto tables = ds->Tables;
                printf("after tables\n");
                for (auto table : tables) {
                    printf("  table name: %s", table.Name.ToString().c_str());
                    //if (!table.Name.ToString().empty()) {
                    //    auto res = ds->GetRowCount(table.Name);
                    //    printf("  res: %i\n", res);
                    //}
                }
            }

        }

    }

    void testDatastoreDiscovery() {
        printf("[TEST] Datastore discovery\n");
        auto* targetClass = Runtime::findClass("Class ProjectX.GFxDataStore_X");

        int found = 0;
        auto* objs = Runtime::getUObjectsPtr();
        for (int i = objs->size() - 1; i >= 200; --i) {
            UObject* obj = objs->at(i);
            if (obj && obj->Class && obj->IsA(targetClass)) {
                printf("  Found datastore: %s\n", obj->GetFullName().c_str());
                found++;
            }
        }

        printf("  Total datastores: %i\n", found);
        assert(found > 0);
        printf("  ✓ Datastores discovered\n");

    }


    void printName(FNameEntry* entry) {
        printf("entry: %s\n", entry->ToString().c_str());
    }

    bool isMatch(FNameEntry* entry) {
        if (entry->ToString() == "NotificationManager_TA") {
            printf("entry id: %i\n", entry->GetIndex());
            return true;
        }
        return false;
    }

    void testFName() {
        printf("[TEST] FName\n");
        printf("  construct an FName with a wstr arg\n");
        auto name = FName(L"Bump");
        if (name.ToString() == "Bump") {
            printf("  ✓ got Bump\n");
        } else {
            printf("  fail\n");
        }
    }

    void testUObjectUtil() {
        printf("[TEST] ObjectUtil\n");

        auto name = FName(L"NotificationManager_TA");
        printf("%s\n", name.ToString().c_str());
        auto* foundClass = UObjectUtil::FindClass(name);
        printf("after findclass\n");

        //if (foundClass->Class->Name == name) {
        //    printf("  ✓ Class->Name attribute matches given name\n");
        //} else {
        //    printf("  fail\n");
        //}

        printf("  Call GetFullName() on found class\n");
        printf("    res: %s\n", foundClass->GetFullName().c_str());



        //printf("  get instance\n");
        //auto* objectUtil = getInstanceOf<UObjectUtil>();
        //if (objectUtil) {
        //    printf("  ✓ not null\n");
        //} else {
        //    printf("  fail. was null\n");
        //}

        //printf("  get static class\n");
        //auto* objectUtilClass = UObjectUtil::StaticClass();
        //if (objectUtilClass) {
        //    printf("  ✓ not null\n");
        //} else {
        //    printf("  fail. was null\n");
        //    return;
        //}

        //auto* util = reinterpret_cast<UObjectUtil*>(objectUtilClass);
        //UObjectUtil::FindClass();
    }

    void testGetInstanceOf() {
        printf("[TEST] GetInstanceOf\n");
        {
            auto* mgr = getInstanceOf<UNotificationManager_TA>();

            printf("  finding UNotificationManager_TA: ");
            if (mgr) {
                printf("pass, found %s\n", mgr->GetFullName().c_str());
            } else {
                printf("fail\n");
            }
        }

        {
            auto objs = getAllInstancesOf<AWorldInfo>();

            for (auto* obj : objs) {
                printf("  finding AWorldInfo: ");
                if (obj) {
                    printf("pass, found %s\n", obj->GetFullName().c_str());
                } else {
                    printf("fail\n");
                }

                printf("  world time seconds: %f\n", obj->TimeSeconds);
                printf("  world engine version: %s\n", obj->EngineVersion.ToString().c_str());
                printf("  world computer name: %s\n", obj->ComputerName.ToString().c_str());
            }
        }

        //auto res = UEngine::GetCurrentWorldInfo();
        //auto* cls = Runtime::findClass("Class Engine.Engine");

        //printf("  from engine static world time seconds: %f\n", res->TimeSeconds);
        //printf("  from engine static world engine version: %s\n", res->EngineVersion.ToString().c_str());
        //printf("  from engine static world computer name: %s\n", res->ComputerName.ToString().c_str());
    }

    void testProcessEventDirect() {
        printf("[TEST] Direct ProcessEvent call (bypass SDK wrapper)\n");

        auto* obj = Runtime::findClass("Class Core.Object");
        if (!obj) {
            printf("FAIL: No object\n");
            return;
        }

        {
            UFunction* func = Runtime::findFunction("Function Engine.WorldInfo.GetWorldInfo");
            if (!func) {
                printf("FAIL: Function not found\n");
                return;
            }

            // Call ProcessEvent directly via vtable
            printf("Calling PE directly on obj=%p, func=%p\n", obj, func);
            fflush(stdout);

            auto vtable = *reinterpret_cast<void***>(obj);
            auto peFunc = reinterpret_cast<void(*)(UObject*, UFunction*, void*)>(vtable[67]);

            printf("PE function pointer: %p\n", peFunc);
            fflush(stdout);

            struct { void* ReturnValue; } params{};
            peFunc(obj, func, &params);

            printf("PE returned! Result: %p\n", params.ReturnValue);

        }

        {
            auto worlds = getAllInstancesOf<AWorldInfo>();

            struct paramStruct {
                uint8_t* ReturnValue;
            };

            for (auto* world : worlds) {
                printf("  finding AWorldInfo: ");
                if (world) {
                    printf("pass, found %s\n", world->GetFullName().c_str());
                } else {
                    printf("fail\n");
                }

                printf("  world time seconds: %f\n", world->TimeSeconds);
                printf("  world engine version: %s\n", world->EngineVersion.ToString().c_str());
                printf("  world computer name: %s\n", world->ComputerName.ToString().c_str());
                UFunction* getTimeFunc = Runtime::findFunction("Function Engine.WorldInfo.GetConsoleType");

                auto vtable = *reinterpret_cast<void***>(world);
                auto peFunc = reinterpret_cast<void(*)(UObject*, UFunction*, void*, void*)>(vtable[67]);
                auto params = paramStruct{};
                peFunc(world, getTimeFunc, &params, nullptr);

                if (params.ReturnValue != nullptr) {
                    printf("ret: %p\n", params.ReturnValue);
                }
            }
        }
    }

        //// Player controller - should exist
        //UFunction* getPCFunc = Runtime::findFunction("Function Engine.WorldInfo.GetALocalPlayerController");
        //struct { APlayerController* ReturnValue; } pcParams{};
        //peFunc(worldInfo, getPCFunc, &pcParams);
        //printf("PlayerController: %p\n", pcParams.ReturnValue);    }


    void testNotification() {
        //    object: GFxData_TextModerationManager_TA Transient.GFxData_TextModerationManager_TA
        //166914
        //object: GFxData_ThankYouMessageManager_TA Transient.GFxData_ThankYouMessageManager_TA
        printf("[TEST] Notification\n");
        auto* mgr = getInstanceOf<UNotificationManager_TA>();
        auto* mgr2 = getInstanceOf<UGFxData_NotificationManager_TA>();

        auto* genericNotification = getInstanceOf<UNotificationManager_TA>();

        if (mgr) {
            printf("got manager: %s\n", mgr->GetFullName().c_str());

            auto* generic = UGenericNotification_TA::StaticClass();
            printf("name: %s\n", generic->GetFullName().c_str());
            generic->StaticClass();
            //auto* toaster = mgr->PopUpOnlyNotification(UGenericNotification_TA::StaticClass());
            auto* toaster = mgr->PopUpOnlyNotification(generic);
            if (toaster) {
                printf("[TOAST] mgr: %p\n", mgr);
                printf("[TOAST] toaster: %p\n", toaster);
                printf("[TOAST] toaster full name: %s\n", toaster->GetFullName().c_str());
                printf("[TOAST] UGenericNotification_TA::StaticClass(): %p\n", UGenericNotification_TA::StaticClass());
                printf("[TOAST] typeid(*toaster): %s\n", toaster ? typeid(*toaster).name() : "<null>");
                printf("[TOAST] vtable: %p\n", toaster ? *(void**)toaster : 0);
                bool bExpired = toaster->IsExpired();
                printf("expired? %d\n", bExpired);
                //auto title = FString::SafeFString(t);
                //auto message = title;
                //toaster->SetTitle(title);
                //toaster->SetBody(message);
                //    // try something trivial like
                //    toaster->PopUpDuration = 10.0f;
                //} else {
                //    printf("toaster null\n");
                //}
            } else {
                printf("no toast :(\n");
            }
        } else {
            printf("fail no manager\n");
        }

    }




};

inline auto enwiden(const std::string& input) -> std::wstring {
    if (input.empty()) return L"";

    int sizeRequired = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (sizeRequired == 0) return L"[enwiden error]";

    std::wstring result(sizeRequired, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &result[0], sizeRequired);

    // Remove null terminator Windows sticks in there
    result.pop_back();

    return result;
}
