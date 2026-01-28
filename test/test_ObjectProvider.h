#pragma once
#include <cassert>

#include <Windows.h>
#include <filesystem>
#include <Psapi.h>

#include "ILogger.h"
#include "Runtime.h"
#include "SDK.h"

#include "fixtures/ObjectProviderFixture.h"
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
//auto MakeEngineFString(wchar_t* s) -> FString {
//    // Borrowed input FString (stack-only, never escapes)
//    FString borrowed;
//    borrowed.ArrayData  = s;
//    borrowed.ArrayCount = s ? static_cast<int32_t>(wcslen(s) + 1) : 0;
//    borrowed.ArrayMax   = borrowed.ArrayCount;

//    // Engine allocates + returns a real FString
//    return UObject::RepeatString(borrowed, 1);
//}

class ObjectProviderTest {
public:
    std::shared_ptr<ObjectProviderFixture> objectProvider = std::make_shared<ObjectProviderFixture>();

    void run() {
        printf("\n\nRUNNING OBJECT PROVIDER TESTS\n\n");

        __try {
            testGetInstanceOf();
            testGetAllInstancesOf();
            //testUObjectUtil();
            //testReturnValue();
//
//            //testGetInstanceOf();
//            //testFindFunction();
//            //testDataStores();
//            //testDatastoreDiscovery();
//
//            //testProcessEventDirect();
//
//            // object provider tests
            //testNotification();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("big test exception\n");
        }

        printf("\n\nTESTING COMPLETE\n\n");
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
    void testGetInstanceOf() {
        printf("\n[TEST] getInstanceOf()\n\n");

        printf("  get instance of UNotificationManager_TA:");
        auto* mgr = objectProvider->provider.getInstanceOf<UNotificationManager_TA>();
        if (!mgr) {
            printf("FAIL\n");
            return;
        }
        printf("  ret: %s\n", r::uobject_utils::getFullName(mgr).c_str());
    }

    void testGetAllInstancesOf() {
        printf("\n[TEST] getAllInstancesOf()\n\n");

        printf("  get all instances of :");
        auto objs = objectProvider->provider.getAllInstancesOf<UGFxData_ShopCatalogue_TA>();
        if (!objs.size()) {
            printf("FAIL\n");
            return;
        }
        for (auto obj : objs) {
            printf("  ret: %s\n", r::uobject_utils::getFullName(obj).c_str());
        }
    }

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


    void printName(const FNameEntry* entry) {
        printf("entry: %s\n", entry->ToString().c_str());
    }

    bool isMatch(const FNameEntry* entry) {
        if (entry->ToString() == "NotificationManager_TA") {
            printf("entry id: %i\n", entry->GetIndex());
            return true;
        }
        return false;
    }

    void testUObjectUtil() {
        //printf("[TEST] ObjectUtil\n");

        //auto* mgr = r::uobject::getInstanceOf<UNotificationManager_TA>();
        //if (!mgr) {
        //    printf("  fail - no object\n");
        //    return;
        //}
        //auto name = r::uobject_utils::getName(mgr);
        //printf("  returned name: %s\n", name.c_str());

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