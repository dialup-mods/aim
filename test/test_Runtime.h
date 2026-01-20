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

//static FString SafeFString(std::wstring_view s) {
//    FString str{};
//    str.ArrayCount = static_cast<int32_t>(s.size() + 1);
//    str.ArrayMax   = str.ArrayCount;
//    str.ArrayData  = new wchar_t[str.ArrayCount];
//
//    wmemcpy(str.ArrayData, s.data(), s.size());
//    str.ArrayData[s.size()] = L'\0';
//    return str;
//}

inline auto int_to_hex(uintptr_t value, size_t width) -> std::string {
    std::ostringstream oss;
    oss << "0x" << std::setfill('0') << std::setw(static_cast<int>(width)) << std::hex << std::uppercase << value;
    return oss.str();
}

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

using ProcessEventFn = void(*)(UObject*, UFunction*, void*);

inline void callProcessEvent(UObject* obj, UFunction* fn, void* params) {
    auto cdo_vtable = reinterpret_cast<void**>(Runtime::findClass("Class Core.Object")->VfTableObject.Ptr)[67];
    printf("cdo ptr: %p\n", cdo_vtable);

    auto** vtable = *reinterpret_cast<void***>(obj);
    auto pe = reinterpret_cast<ProcessEventFn>(vtable[67]);
    printf("obj ptr: %p\n", pe);
    pe(obj, fn, params);
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

            if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }

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
            testDataStores();
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

    void testDataStores() {
        printf("[TEST] Call Method on Object\n");

        auto* cls = Runtime::findClass("Class Core.Object");
        //auto* fn = Runtime::findFunction("Function ProjectX.GFxDataStore_X.GetRowCount");




        //{
        //    auto pcs = getAllInstancesOf<APlayerController_TA>();
        //        printf("RegisterCustomPlayerDataStores:");
        //        printf("  LP: %p\n", params->LP);
        //        printf("  DataStoreManager: " + stringutil::toHex(reinterpret_cast<uintptr_t>(params->DataStoreManager)));
        //        printf("  PlayerDataStoreClass: " + safe::ue::name(params->PlayerDataStoreClass));
        //        printf("  PlayerName: " + stringutil::toString(params->PlayerName));
        //}


        auto* fn = Runtime::findFunction("Function TAGame.GFxData_Chat_TA.ClearDistracted");
        {
            auto chats = getAllInstancesOf<UGFxData_Chat_TA>();
            for (auto* chat : chats) {
                printf("  found obj: %s\n", chat->GetFullName().c_str());
                if (!chat->Shell) {
                    printf("no shell\n");
                    continue;
                }
                if (!chat->Shell->DataStore) {
                    printf("no datastore\n");
                    continue;
                }

                printf("  should be transient / live: %s\n", chat->GetFullName().c_str());
                chat->ClearDistracted();
                callProcessEvent(chat, fn, nullptr);

                auto ds = chat->Shell->DataStore;
                //chat->Shell->DataStore->PrintData(L"ChatPresetMessages");
                printf("chat name: %s\n", chat->GetFullName().c_str());
                printf("ds name: %s\n", ds->GetFullName().c_str());

                auto tables = ds->Tables;
                for (auto table : tables) {
                    if (!table.Name.ToString().empty() && table.Name.ToString() != "None") {
                        struct {
                            FName Table;
                            int32_t ReturnValue;
                        } params;

                //        printf("fn at: %p\n", fn);
                        printf("  table name: %s\n", table.Name.ToString().c_str());
                        auto countMaybe = ds->GetRowCount(table.Name);
                        printf("  ->ret: %i\n", countMaybe);
                //        params.Table = table.Name;
                //        //auto res = ds->GetRowCount(table.Name);
                //        callProcessEvent(ds, fn, &params, nullptr);
                //        printf("  res: %i\n", params.ReturnValue);
                    }
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

                if (!obj->EngineVersion.ToString().empty()) {
                    printf("  world time seconds: %f\n", obj->TimeSeconds);
                    printf("  world engine version: %s\n", obj->EngineVersion.ToString().c_str());
                    printf("  world computer name: %s\n", obj->ComputerName.ToString().c_str());
                }
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

        //auto* coreObjStatic = Runtime::findClass("Class Core.Object");
        //if (!coreObjStatic) {
        //    printf("FAIL: No object\n");
        //    return;
        //}

        //{
        //    UFunction* func = Runtime::findFunction("Function Engine.WorldInfo.GetWorldInfo");
        //    if (!func) {
        //        printf("FAIL: Function not found\n");
        //        return;
        //    }

        //    // Call ProcessEvent directly via vtable
        //    printf("Calling PE directly on obj=%p, func=%p\n", coreObjStatic, func);
        //    fflush(stdout);

        //    auto vtable = *reinterpret_cast<void***>(coreObjStatic);
        //    auto peFunc = reinterpret_cast<void(*)(UObject*, UFunction*, void*)>(vtable[67]);

        //    printf("PE function pointer: %p\n", peFunc);
        //    fflush(stdout);

        //    struct { void* ReturnValue; } params{};
        //    peFunc(coreObjStatic, func, &params);

        //    printf("PE returned! Result: %p\n", params.ReturnValue);

        //}

        //{
        //    auto datastores = getAllInstancesOf<UGFxDataStore_X>();
        //    for (auto* ds : datastores) {
        //        printf("  found obj: %s\n", ds->GetFullName().c_str());
        //        auto tables = ds->Tables;
        //        printf("after tables\n");
        //        for (auto table : tables) {
        //            if (!table.Name.ToString().empty() && table.Name.ToString() != "None") {
        //                printf("  table name: %s\n", table.Name.ToString().c_str());
		//                auto* fn = UFunction::FindFunction("Function ProjectX.GFxDataStore_X.GetRowCount");
        //                auto vtable = *reinterpret_cast<void***>(table);
        //                auto peFunc = reinterpret_cast<void(*)(UObject*, UFunction*, void*, void*)>(vtable[67]);
        //                struct {
        //                    FName Table;
        //                    int32_t ReturnValue;
        //                } params;
        //                params.Table = table.Name;
        //                peFunc(, getTimeFunc, &params, nullptr);

        //                if (params.ReturnValue != nullptr) {
        //                    printf("ret: %p\n", params.ReturnValue);
        //                }

        //                auto res = ds->GetRowCount(table.Name);
        //                printf("  res: %i\n", res);
        //            }
        //        }
        //    }

        //}

        //{
        //    auto* obj = getInstanceOf<UObject>();
        //    auto* func = Runtime::findFunction("Function Core.Object.GetEngineVersion");
        //    printf("Calling PE directly on obj=%p, func=%p\n", obj, func);
        //    fflush(stdout);

        //    auto vtable = *reinterpret_cast<void***>(obj);
        //    auto peFunc = reinterpret_cast<void(*)(UObject*, UFunction*, void*, void*)>(vtable[67]);
        //    struct {
        //        uint8_t* ReturnValue;
        //    } params;
        //    peFunc(obj, func, &params, nullptr);

        //    printf("    ret: %p\n", params.ReturnValue);
        //}

        {
            auto* idFn = Runtime::findFunction("Function Core.Object.GetUniqueID");

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

                if (!world->EngineVersion.ToString().empty()) {
                    printf("  world time seconds: %f\n", world->TimeSeconds);
                    printf("  world engine version: %s\n", world->EngineVersion.ToString().c_str());
                    printf("  world computer name: %s\n", world->ComputerName.ToString().c_str());
                    UFunction* getTimeFunc = Runtime::findFunction("Function Engine.WorldInfo.GetConsoleType");
                    UFunction* getWorldInfo = Runtime::findFunction("Function Engine.Engine.GetCurrentWorldInfo");


                    struct {
                        AWorldInfo* ReturnValue;
                    } worldInfoParams;

                    struct {
                        uint32_t bIncludePrefix;
                        FString ReturnValue = {};
                    } params;

                    params.bIncludePrefix = 1;

                    callProcessEvent(world, Runtime::findFunction("Function Engine.WorldInfo.GetMapName"), &params);
                    printf("  ret ptr: %p\n", &params.ReturnValue);
                    printf("  map name: %s\n", params.ReturnValue.ToString().c_str());
                    printf("  map name: %ls\n", params.ReturnValue.ArrayData);

                    callProcessEvent(world, getWorldInfo, &worldInfoParams);
                    if (worldInfoParams.ReturnValue != nullptr) {
                        printf("  ret: %p\n", worldInfoParams.ReturnValue);
                        auto* retWorldInfoObj = worldInfoParams.ReturnValue;
                        printf("  console type: %i\n", retWorldInfoObj->GetConsoleType());
                    }


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
        auto* mgr = getInstanceOf<UNotificationManager_TA>();
        auto* toaster = mgr->PopUpOnlyNotification(UGenericNotification_TA::StaticClass());
        //toaster->SetTitle();
        //toaster->SetBody(L"bar");
        toaster->PopUpDuration = static_cast<float>(5);
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
