#pragma once
#include "Runtime.h"
#include "EngineLocator.h"
#include "ILogger.h"
#include "IModule.h"
#include "SDK.h"
using r = Runtime;

class IRuntime : public IModule {
    AIM_INJECTABLE(AIMRuntime)
    AIM_INJECT(ILogger, log)
    AIM_INJECT(EngineLocator, engineLocator)

    IRuntime() = default;
    virtual ~IRuntime() = default;

    bool init() {
        Runtime::create();

        log_->info("AIM Runtime initializing...");

        const uintptr_t fNameEntriesAddr = engineLocator_->getFNameEntriesAddress();
        const uintptr_t uObjectsAddr = engineLocator_->getUObjectsAddress();

        if (!uObjectsAddr || !fNameEntriesAddr) {
            log_->error("Cannot initialize.");
            return false;
        }

        r::fname::game_pool::set(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddr));
        r::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(uObjectsAddr));

        log_->debug("engine valid: {}", std::to_string(r::uobject::game_pool::hasUObjects() && r::fname::game_pool::isValid()));

        //log_->info("FNameEntry::Flags     = 0x" + int_to_hex(offsetof(FNameEntry, Flags)));
        //log_->info("FNameEntry::Index     = 0x" + int_to_hex(offsetof(FNameEntry, Index)));
        //log_->info("FNameEntry::Name      = 0x" + int_to_hex(offsetof(FNameEntry, Name)));
        //log_->info("FNameEntry sizeof     = 0x" + int_to_hex(sizeof(FNameEntry)));

        return (r::uobject::game_pool::isPopulated() && r::fname::game_pool::isValid());
    }

    static auto hasUObjects() -> bool { return r::uobject::game_pool::hasUObjects(); }
    static auto hasFNames() -> bool { return r::fname::game_pool::isValid(); }

    auto getUObjectsPtr() -> TArray<UObject*>* {
        return r::uobject::game_pool::ptr();
    }

    auto getUObjects() -> TArray<UObject*> {
        return r::uobject::game_pool::ref();
    }

    auto findClass(const std::string& classFullName) -> UClass* {
        return r::uclass::find(classFullName);
    }

    auto findFunction(const std::string& functionFullName) -> UFunction* {
        return r::ufunction::find(functionFullName);
    }

    auto areFNameEntriesValid() -> bool {
        return r::fname::game_pool::isValid();
    }

    auto areUObjectsPopulated() -> bool {
        return r::uobject::game_pool::isPopulated();
    }

    auto getFNameEntries() -> TArray<FNameEntry*>& {
        return r::fname::game_pool::ref();
    }

    auto getFNameEntriesPtr() -> TArray<FNameEntry*>* {
        return r::fname::game_pool::ptr();
    }

    auto getFNameEntry(const int32_t index) ->  std::optional<std::reference_wrapper<const FName>> {
        return r::fname::game_pool::find(index);
    }

    //auto getFNameEntryName(int32_t index) -> std::string {
    //    return Runtime::getFNameEntryName(index);
    //}

    auto findPackages() -> std::vector<UObject*> {
        return r::packages::findAll();
    }

    auto getRawObjects() -> const std::vector<UObject*>& {
        return r::uobject::cache::rawObjects();
    }

    auto getUObjectsCache() -> std::vector<UObject*>& {
        return r::uobject::cache::ref();
    }

    //void shutdown() {
    //    Runtimme::shutdown();
    //}
};
