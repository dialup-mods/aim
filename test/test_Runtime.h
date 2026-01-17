#pragma once
#include <cassert>

#ifndef SDK_DLL
#error "SDK_DLL must be defined at compile time"
#endif

#include <Windows.h>
#include <filesystem>

#include "ILogger.h"
#include "Runtime.h"
#include "SDK.h"

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
    void run() {
        printf("Initializing Runtime...\n");
        HMODULE sdk = GetModuleHandleW(L"DialUp-SDK.dll");
        if (!sdk) {
            MessageBoxA(nullptr, "SDK not loaded", "Test error", MB_OK);
            return;
        }
        Runtime::create();

        populate();
        testClassLookup();

        //checkGetUObjectsPtr();
        //checkGetFNameEntriesPtr();
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
        printf(" - Core.Object: ");
        auto* uObjCls = Runtime::findClass("Class Core.Object");
        if (uObjCls && uObjCls->GetFullName() == "Class Core.Object") {
            printf("pass\n");
        } else {
            printf("fail\n");
        }

        printf(" - ProjectX.GFxDataStore: ");
        auto* cls = Runtime::findClass("Class ProjectX.GFxDataStore_X");
        if (cls && cls->GetFullName() == "Class ProjectX.GFxDataStore_X") {
            printf("pass\n");
        } else {
            printf("fail\n");
        }
    }

};

//
//    void testObjectIteration() {
//        log->info("[TEST] Object iteration");
//        int count = 0;
//        auto* objs = Runtime::getUObjectsPtr();
//
//        for (int i = objs->size() - 1; i >= 50; --i) {
//            UObject* obj = objs->at(i);
//            if (obj && obj->Class) {
//                count++;
//                if (count == 1) {
//                    log->info("  First valid object: " + obj->GetFullName());
//                }
//            }
//        }
//        log->info("  Valid objects found: " + std::to_string(count));
//        assert(count > 0);
//        log->info("  ✓ Iteration works");
//    }
//
//    void testDatastoreDiscovery() {
//        log->info("[TEST] Datastore discovery");
//        auto* targetClass = Runtime::findClass("Class TAGame.GFxDataStore_X");
//
//        int found = 0;
//        auto* objs = Runtime::getUObjectsPtr();
//        for (int i = objs->size() - 1; i >= 50; --i) {
//            UObject* obj = objs->at(i);
//            if (obj && obj->Class && obj->IsA(targetClass)) {
//                log->info("  Found datastore: " + obj->GetFullName());
//                found++;
//            }
//        }
//
//        log->info("  Total datastores: " + std::to_string(found));
//        assert(found > 0);
//        log->info("  ✓ Datastores discovered");
//    }
//};

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
