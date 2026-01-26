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
#include "StringUtil.h"

using r = Runtime;

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

template<typename T, typename Predicate>
void forEachObject(Predicate&& pred) {
    auto& objects = r::uobject::game_pool::ref();

    for (int32_t i = objects.size() - 100; i >= 0; --i) {
        UObject* obj = objects.at(i);

        if (!obj || !obj->Class || !obj->IsA(T::StaticClass())) {
            continue;
        }

        pred(static_cast<T*>(obj));
    }
}

template<typename T, typename Predicate>
void forEachValidLiveObject(Predicate&& pred) {
    static_assert(std::is_base_of_v<UObject, T>, "T must be a UObject-derived type");

    auto& objects = r::uobject::game_pool::ref();

    for (int32_t i = objects.size() - 100; i >= 0; --i) {
        UObject* obj = objects.at(i);
        if (!obj) { continue; }

        uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
        if (addr < 0x10000 || addr > 0xFFFFFFFFFF) { continue; } // skip obviously bad memory

        if (!obj->Class) { continue; }

        if (!obj->IsA(T::StaticClass())) { continue; }

        if (obj->HasAnyFlags(static_cast<EObjectFlags>(RF_BeginDestroyed || RF_FinishDestroyed || RF_DefaultOrArchetypeFlags))) { continue; }

        pred(static_cast<T*>(obj));
    }
}







class RuntimeTest {
public:
    void printName(UObject* uObject) const {
        printf(" GetFullName(): %s\n", uObject->GetFullName().c_str());
    }

    /// test object provider
    //template<typename T>
    //T* getInstanceOf() const {
    //    if (!std::is_base_of_v<UObject, T>) { return nullptr; }

    //    auto& objects = r::uobject::game_pool::ref();
    //    printf("num objects: %i\n", objects.size());

    //    for (int i = objects.size(); i > 0; i--) {
    //        UObject* uObject = r::uobject::game_pool::ref().at(i);
    //        if (!uObject) { continue; }
    //        //__try {
    //        //    printName(uObject);
    //        //} __except (EXCEPTION_EXECUTE_HANDLER) {
    //        //    printf("exception\n");
    //        //}
    //        if (!uObject->IsA<T>()) { continue; }

    //        if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }

    //        printf("found at index: %i/%i\n", i, r::uobject::game_pool::ref().size());
    //        return static_cast<T*>(uObject);
    //    }

    //    return nullptr;
    //}

    //template<typename T>
    //std::vector<T*> getAllInstancesOf() const {
    //    std::vector<T*> found;
    //    if (!std::is_base_of_v<UObject, T>) { return {}; }

    //    auto& objects = r::uobject::game_pool::ref();
    //    printf("num objects: %i\n", objects.size());

    //    for (int i = objects.size() - 1; i > 0; i--) {
    //        UObject* uObject = r::uobject::game_pool::ref().at(i);
    //        if (!uObject) { continue; }
    //        //__try {
    //        //    printName(uObject);
    //        //} __except (EXCEPTION_EXECUTE_HANDLER) {
    //        //    printf("exception\n");
    //        //}
    //        if (!uObject->IsA<T>()) { continue; }

    //        //if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }

    //        printf("found at index: %i/%i\n", i, r::uobject::game_pool::ref().size());
    //        found.emplace_back(static_cast<T*>(uObject));
    //    }

    //    return found;
    //}

    //template<typename T>
    //auto getLiveInstanceOf() -> T* {
    //    static_assert(std::is_base_of_v<UObject, T>, "T must be a UObject-derived type");
    //    auto& objects = r::uobject::game_pool::ref();

    //    for (int32_t i = objects.size() - 1; i >= 0; --i) {
    //        UObject* obj = objects.at(i);
    //        if (!obj) { continue; }
    //        if (obj->HasAnyFlags(static_cast<EObjectFlags>(RF_ClassDefaultObject | RF_ArchetypeObject | RF_Transient))) { continue; }
    //        if (!obj->Class) { continue; }
    //        if (!obj->IsA(T::StaticClass())) { continue; }
    //        if (obj->IsPendingKill()) { continue; }
    //        if (obj->IsArchetype()) { continue; }

    //        T* candidate = static_cast<T*>(obj);

    //        // Special case if you're looking for UGFxData_Chat_TA
    //        if constexpr (std::is_same_v<T, UGFxData_Chat_TA>) {
    //            if (!candidate->Shell || !candidate->Shell->DataStore)
    //                continue; // not fully initialized, skip
    //        }

    //        printf("Found live instance: %s\n", obj->GetFullName().c_str());
    //        return candidate;
    //    }

    //    printf("[WARN] No valid live instance found");
    //    return nullptr;
    //}

    void run() {
        printf("Initializing Runtime...\n");
        HMODULE sdk = GetModuleHandleW(L"DialUp-SDK.dll");
        if (!sdk) {
            MessageBoxA(nullptr, "SDK not loaded", "Test error", MB_OK);
            return;
        }
        r::create();
        printf("\n\nRUNNING TESTS\n\n");

        __try {
            populate();
            checkGetUObjectsPtr();
            checkGetFNameEntriesPtr();
            testFName();
            testObjectIteration();
            testClassLookup();
            testFindFunction();
            //testReturnValue();
//
//            //testGetInstanceOf();
//            //testFindFunction();
//            //testDataStores();
//            //testDatastoreDiscovery();
//
//            //testProcessEventDirect();
//            //testUObjectUtil();
//
//            // object provider tests
//            //testNotification();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("big test exception\n");
        }

        printf("\n\nTESTING COMPLETE\n\n");
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

    //void testReturnValue() {
    //    printf("[TEST] return value\n");
    //    printf("Note: this must be done inside of PE, in the same frame, or return is destroyed before we have a chance to read it\n");
    //    printf("this assumes you have debug printing from within the PE loop and you're checking the return yourself");
    //    auto* mgr = getInstanceOf<UNotificationManager_TA>();
    //    printf("  testing on found object: %s\n", mgr->GetFullName().c_str());
    //    if (!mgr) {
    //        printf("  -> fail, unable to get instance\n");
    //        return;
    //    }
    //    TAGame::UNotificationManager_TA_PopUpOnlyNotification_Params params{};
    //    printf("  sending params %p\n", static_cast<void*>(&params));
    //    params.NotificationClass = r::uclass::find("Class TAGame.GenericNotification_TA");
    //    r::process_event::call(mgr, r::ufunction::find("Function TAGame.NotificationManager_TA.PopUpOnlyNotification"), &params);
    //    //auto* ret = params.ReturnValue;
    //    //printf("  -> ret value  %p\n", static_cast<void*>(params.ReturnValue));
    //}

    void testDataStores() {
        printf("[TEST] Call Method on Object\n");

        auto* cls = r::uclass::find("Class Core.Object");
        //auto* fn = r::ufunction::find("Function ProjectX.GFxDataStore_X.GetRowCount");

        //{
        //    auto pcs = getAllInstancesOf<APlayerController_TA>();
        //        printf("RegisterCustomPlayerDataStores:");
        //        printf("  LP: %p\n", params->LP);
        //        printf("  DataStoreManager: " + stringutil::toHex(reinterpret_cast<uintptr_t>(params->DataStoreManager)));
        //        printf("  PlayerDataStoreClass: " + safe::ue::name(params->PlayerDataStoreClass));
        //        printf("  PlayerName: " + stringutil::toString(params->PlayerName));
        //}


        //auto* fn = r::ufunction::find("Function TAGame.GFxData_Chat_TA.ClearDistracted");
        //{
        //    auto chats = getAllInstancesOf<UGFxData_Chat_TA>();
        //    for (auto* chat : chats) {
        //        //printf("  found obj: %s\n", chat->GetFullName().c_str());
        //        if (!chat->Shell) {
        //            //printf("no shell\n");
        //            continue;
        //        }
        //        if (!chat->Shell->DataStore) {
        //            //printf("no datastore\n");
        //            continue;
        //        }

        //        //printf("  should be transient / live: %s\n", chat->GetFullName().c_str());
        //        chat->ClearDistracted();
        //        r::process_event::call(chat, fn, nullptr);

        //        auto ds = chat->Shell->DataStore;
        //        //chat->Shell->DataStore->PrintData(L"ChatPresetMessages");
        //        printf("chat name: %s\n", chat->GetFullName().c_str());
        //        printf("ds name: %s\n", ds->GetFullName().c_str());

        //        auto tables = ds->Tables;
        //        for (auto table : tables) {
        //            if (table.Name.FNameEntryId && r::fname::game_pool::getString(table.Name.FNameEntryId) != "None") {
        //                struct {
        //                    FName Table;
        //                    int32_t ReturnValue;
        //                } params;

        //        //        printf("fn at: %p\n", fn);
        //                auto tableNameStr = r::fname::game_pool::getString(table.Name.FNameEntryId).value_or("[ BAD ]");
        //                printf("  table name: %s\n", tableNameStr.c_str());
        //                auto countMaybe = ds->GetRowCount(table.Name);
        //        //        printf("  ->ret: %i\n", countMaybe);
        //        //        params.Table = table.Name;
        //        //        //auto res = ds->GetRowCount(table.Name);
        //        //        r::process_event::call(ds, fn, &params, nullptr);
        //        //        printf("  res: %i\n", params.ReturnValue);
        //            }
        //        }
        //    }

        //}

    }

    void testDatastoreDiscovery() {
        //printf("[TEST] Datastore discovery\n");
        //auto* targetClass = r::uclass::find("Class ProjectX.GFxDataStore_X");

        //int found = 0;
        //auto* objs = r::uobject::game_pool::ref();
        //?? for (int i = objs->size() - 1; i >= 200; --i) {
        //    UObject* obj = objs->at(i);
        //    if (obj && obj->Class && obj->IsA(targetClass)) {
        //        printf("  Found datastore: %s\n", obj->GetFullName().c_str());
        //        found++;
        //    }
        //}

        //printf("  Total datastores: %i\n", found);
        //assert(found > 0);
        //printf("  ✓ Datastores discovered\n");
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
        auto name = r::fname::game_pool::find(L"Bump");
        if (name.value().get().ToString() == "Bump") {
            printf("  ✓ got Bump\n");
        } else {
            printf("  fail\n");
        }
    }

    void testUObjectUtil() {
        //printf("[TEST] ObjectUtil\n");

        //auto name = r::fname::game_pool::getString(L"NotificationManager_TA");
        //printf("%s\n", name.ToString().c_str());
        //auto* foundClass = UObjectUtil::FindClass(name);
        //printf("after findclass\n");

        ////if (foundClass->Class->Name == name) {
        ////    printf("  ✓ Class->Name attribute matches given name\n");
        ////} else {
        ////    printf("  fail\n");
        ////}

        //printf("  Call GetFullName() on found class\n");
        //printf("    res: %s\n", foundClass->GetFullName().c_str());



        ////printf("  get instance\n");
        ////auto* objectUtil = getInstanceOf<UObjectUtil>();
        ////if (objectUtil) {
        ////    printf("  ✓ not null\n");
        ////} else {
        ////    printf("  fail. was null\n");
        ////}

        ////printf("  get static class\n");
        ////auto* objectUtilClass = UObjectUtil::StaticClass();
        ////if (objectUtilClass) {
        ////    printf("  ✓ not null\n");
        ////} else {
        ////    printf("  fail. was null\n");
        ////    return;
        ////}

        ////auto* util = reinterpret_cast<UObjectUtil*>(objectUtilClass);
        ////UObjectUtil::FindClass();
    }

    //void testGetInstanceOf() {
    //    printf("[TEST] GetInstanceOf\n");
    //    {
    //        auto* mgr = getInstanceOf<UNotificationManager_TA>();

    //        printf("  finding UNotificationManager_TA: ");
    //        if (mgr) {
    //            printf("pass, found %s\n", mgr->GetFullName().c_str());
    //        } else {
    //            printf("fail\n");
    //        }
    //    }

    //    {
    //        auto objs = getAllInstancesOf<AWorldInfo>();

    //        for (auto* obj : objs) {
    //            printf("  finding AWorldInfo: ");
    //            if (obj) {
    //                printf("pass, found %s\n", obj->GetFullName().c_str());
    //            } else {
    //                printf("fail\n");
    //            }

    //            if (!obj->EngineVersion.ToString().empty()) {
    //                printf("  world time seconds: %f\n", obj->TimeSeconds);
    //                printf("  world engine version: %s\n", obj->EngineVersion.ToString().c_str());
    //                printf("  world computer name: %s\n", obj->ComputerName.ToString().c_str());
    //            }
    //        }
    //    }

    //    //auto res = UEngine::GetCurrentWorldInfo();
    //    //auto* cls = r::uclass::find("Class Engine.Engine");

    //    //printf("  from engine static world time seconds: %f\n", res->TimeSeconds);
    //    //printf("  from engine static world engine version: %s\n", res->EngineVersion.ToString().c_str());
    //    //printf("  from engine static world computer name: %s\n", res->ComputerName.ToString().c_str());
    //}


    void testProcessEventDirect() {
        printf("[TEST] Direct ProcessEvent call (bypass SDK wrapper)\n");


        //auto* coreObjStatic = r::uclass::find("Class Core.Object");
        //if (!coreObjStatic) {
        //    printf("FAIL: No object\n");
        //    return;
        //}

        //{
        //    UFunction* func = r::ufunction::find("Function Engine.WorldInfo.GetWorldInfo");
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
        //            UFunction* getTimeFunc = r::ufunction::find("Function Engine.WorldInfo.GetConsoleType");
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
        //    auto* func = r::ufunction::find("Function Core.Object.GetEngineVersion");
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

        //{
        //    auto worlds = getAllInstancesOf<AWorldInfo>();

        //    struct paramStruct {
        //        uint8_t* ReturnValue;
        //    };

        //    for (auto* world : worlds) {
        //        printf("  finding AWorldInfo: ");
        //        if (world) {
        //            printf("pass, found %s\n", world->GetFullName().c_str());
        //        } else {
        //            printf("fail\n");
        //        }

        //        if (!world->EngineVersion.ToString().empty()) {
        //            printf("  world time seconds: %f\n", world->TimeSeconds);
        //            printf("  world engine version: %s\n", world->EngineVersion.ToString().c_str());
        //            printf("  world computer name: %s\n", world->ComputerName.ToString().c_str());

        //            struct {
        //                uint32_t bIncludePrefix;
        //                FString ReturnValue = {};
        //            } params;
        //            params.bIncludePrefix = 1;

        //            r::process_event::call(world, r::ufunction::find("Function Engine.WorldInfo.GetMapName"), &params);
        //            printf("  ret ptr: %p\n", &params.ReturnValue);
        //            printf("  map name: %s\n", params.ReturnValue.ToString().c_str());
        //            printf("  map name: %ls\n", params.ReturnValue.ArrayData);

        //            struct {
        //                AWorldInfo* ReturnValue;
        //            } worldInfoParams;
        //            UFunction* getWorldInfo = r::ufunction::find("Function Engine.Engine.GetCurrentWorldInfo");
        //            r::process_event::call(world, getWorldInfo, &worldInfoParams);
        //            if (worldInfoParams.ReturnValue != nullptr) {
        //                printf("  ret: %p\n", worldInfoParams.ReturnValue);
        //                auto* retWorldInfoObj = worldInfoParams.ReturnValue;
        //                printf("  console type: %i\n", retWorldInfoObj->GetConsoleType());
        //            }
        //        }
        //    }
        //}
   }

        //// Player controller - should exist
        //UFunction* getPCFunc = r::ufunction::find("Function Engine.WorldInfo.GetALocalPlayerController");
        //struct { APlayerController* ReturnValue; } pcParams{};
        //peFunc(worldInfo, getPCFunc, &pcParams);
        //printf("PlayerController: %p\n", pcParams.ReturnValue);    }

    //auto MakeEngineFString(wchar_t* s) -> FString {
    //    // Borrowed input FString (stack-only, never escapes)
    //    FString borrowed;
    //    borrowed.ArrayData  = s;
    //    borrowed.ArrayCount = s ? static_cast<int32_t>(wcslen(s) + 1) : 0;
    //    borrowed.ArrayMax   = borrowed.ArrayCount;

    //    // Engine allocates + returns a real FString
    //    return UObject::RepeatString(borrowed, 1);
    //}

//    void testNotification() {
//        printf("[TEST] Notification\n");
//            auto* mgr = getInstanceOf<UNotificationManager_TA>();
//            printf("  found: %s\n", mgr->GetFullName().c_str());
//
////            printf("    ptr: %p\n", mgr->AddNotification(mgr, 0));
//            printf("    creating toaster: \n");
//            //auto* toaster = mgr->PopUpOnlyNotification(UGenericNotification_TA::StaticClass());
//            auto* notificationClass = r::uclass::find("Class TAGame.GenericNotification_TA");
//            printf("    -> notification class: %s\n", notificationClass->GetFullName().c_str());
//            auto* toaster = mgr->PopUpOnlyNotification(notificationClass);
//
//            //if (toaster) {
//            //    printf("[toaster] %s\n", toaster->GetFullName().c_str());
//            //    toaster->SetTitle(L"Super");
//            //    toaster->SetBody(L"fooo");
//            //    toaster->PopUpDuration = 4;
//            //    printf("    pass: %s\n", toaster->GetFullName().c_str());
//            //} else {
//            //    printf("    fail\n");
//            //}
//
//            //TAGame::UNotificationManager_TA_PopUpOnlyNotification_Params params{};
//            //params.NotificationClass = UGenericNotification_TA::StaticClass();
//            //params.ReturnValue = {};
//            //UFunction* popUpFn = r::ufunction::find("Function TAGame.NotificationManager_TA.PopUpOnlyNotification");
//            //r::process_event::call(mgr, popUpFn, &params);
//            //if (params.ReturnValue != nullptr) {
//            //    printf("  ret: %p\n", params.ReturnValue);
//            //    printf("  ret val: %s\n", params.ReturnValue->GetFullName().c_str());
//            //} else {
//            //    printf(" return null\n");
//            //}
//        //toaster->SetTitle();
//        //toaster->SetBody(L"bar");
//        //toaster->PopUpDuration = static_cast<float>(5);
//    }
};