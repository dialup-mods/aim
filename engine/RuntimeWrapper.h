#pragma once
#include "Runtime.h"
#include "EngineLocator.h"
#include "ILogger.h"
#include "IRuntime.h"
#include "SDK.h"
using r = Runtime;

class RuntimeWrapper : public IRuntime {
    AIM_INJECTABLE(RuntimeWrapper)
    AIM_INJECT(ILogger, log)
    AIM_INJECT(EngineLocator, engineLocator)

    RuntimeWrapper() = default;
    virtual ~RuntimeWrapper() = default;

    bool init() {
        log_->info("AIM Runtime initializing...");

        // fixme security pull out of public headers
        const uintptr_t uObjectsAddr = engineLocator_->getUObjectsAddress();
        const uintptr_t fNameEntriesAddr = engineLocator_->getFNameEntriesAddress();

        if (!uObjectsAddr || !fNameEntriesAddr) {
            log_->error("Cannot initialize.");
            return false;
        }

        Runtime::create();
        r::fname::game_pool::set(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddr));
        r::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(uObjectsAddr));

        if (!(r::uobject::game_pool::isPopulated() && r::uobject::game_pool::hasUObjects())) {
            log_->error("Failed to start Runtime.");
            return false;
        }

        log_->debug("Engine valid: {}", std::to_string(r::uobject::game_pool::hasUObjects() && r::fname::game_pool::isValid()));

        //log_->info("FNameEntry::Flags     = 0x" + int_to_hex(offsetof(FNameEntry, Flags)));
        //log_->info("FNameEntry::Index     = 0x" + int_to_hex(offsetof(FNameEntry, Index)));
        //log_->info("FNameEntry::Name      = 0x" + int_to_hex(offsetof(FNameEntry, Name)));
        //log_->info("FNameEntry sizeof     = 0x" + int_to_hex(sizeof(FNameEntry)));

        if (!r::uobject::game_pool::isPopulated()) {
            log_->error("Object pool failed to populate.");
            return false;
        }
        if (!r::fname::game_pool::isValid()) {
            log_->error("FName pool is not valid.");
            return false;
        }

        r::uobject::cache::buildClassNameCacheFromCDOs();
        r::uobject::cache::populateClassToCDO();

        return true;
    }

    auto hasUObjects() -> bool { return r::uobject::game_pool::hasUObjects(); }
    auto hasFNames() -> bool { return r::fname::game_pool::isValid(); }

    auto getUObjectsPtr() -> TArray<UObject*>* {
        return r::uobject::game_pool::ptr();
    }

    auto getUObjects() -> TArray<UObject*> override {
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

    template<typename T>
    auto classOf() -> UClass* {
        static UClass* cls = resolveClass(T::className);
        return cls;
    }

    auto resolveClass(std::string_view className) -> UClass* override {
        return r::uobject::resolveClass(className);
    }

    auto getFirst(std::string_view className) -> UObject* override {
        return r::uobject::getFirst(className);
    }

    auto getAll(std::string_view className) -> std::vector<UObject*> override {
        return r::uobject::getAll(className);
    }
};