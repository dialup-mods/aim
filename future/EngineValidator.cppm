module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <memory>

export module EngineValidator;

import "SdkHeaders.hpp";
import "GameDefines.hpp";
import "SafeLogger.h";
import "DependencyContainer.h";

// defined in gamedefines
// TArray<UObject*>* GObjects;
// TArray<FNameEntry*>* GNames;

export class EngineValidator : public IModule {
    AIM_INJECTABLE(EngineValidator)
    AIM_INJECT(SafeLogger, log)

  public:
    EngineValidator() = default;
    
    ~EngineValidator() {
        log_->info("EngineValidator unloading...");
        GObjects = nullptr;
        GNames = nullptr;
        log_->info("EngineValidator unloaded");
    }

    bool init() {
        log_->debug("EngineValidator initializing...");

        uintptr_t gObjAddr = GetGObjectsAddress();
        uintptr_t gNameAddr = GetGNamesAddress();

        if (!gObjAddr || !gNameAddr) {
            log_->error("Pattern scan failed. Cannot initialize.");
            return false;
        }

        GObjects = reinterpret_cast<TArray<UObject*>*>(gObjAddr);
        GNames = reinterpret_cast<TArray<FNameEntry*>*>(gNameAddr);

        return (this->AreGObjectsValid() && this->AreGNamesValid());
    }

    static TArray<UObject*>* getObjects() { return GObjects; }
    static TArray<FNameEntry*>* getNames() { return GNames; }

    auto AreGObjectsValid() -> bool {
        if (GObjects && !GObjects->IsEmpty() && GObjects->Max() > GObjects->Num()) {
            if (GObjects->At(0)->GetFullName() == "Class Core.Config_ORS") {
                log_->info("AreGObjectsValid returning true");
                return true;
            }
        }
        log_->info("AreGObjectsValid returning false");
        return false;
    }

    auto AreGNamesValid() -> bool {
        if (GNames && !GNames->IsEmpty() && GNames->Max() > GNames->Num()) {
            if (FName(0).ToString() == "None") {
                log_->info("AreGNamesValid returning true");
                return true;
            }
        }
        log_->info("AreGObjectsValid returning false");
        return false;
    }

  private:
    auto FindPattern(HMODULE module, const unsigned char* pattern, const char* mask) -> uintptr_t {
        MODULEINFO info = {};
        GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

        uintptr_t start = reinterpret_cast<uintptr_t>(module);
        size_t length = info.SizeOfImage;

        size_t pos = 0;
        size_t maskLength = std::strlen(mask) - 1;

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

    auto GetGNamesAddress() -> uintptr_t {
        // fixme
        unsigned char GNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
        char GNamesMask[] = "??????xx??xxxxxx";

        uintptr_t GNamesAddress = FindPattern(GetModuleHandleW(L"RocketLeague.exe"), GNamesPattern, GNamesMask);

        return GNamesAddress;
    }

    auto GetGObjectsAddress() -> uintptr_t {
        // fixme
        return GetGNamesAddress() + 0x48;
    }

    TArray<class UObject*>* gObjects_{};
    TArray<struct FNameEntry*>* gNames_{};
};
