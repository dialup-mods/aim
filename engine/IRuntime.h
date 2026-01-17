#pragma once
#include "Runtime.h"
#include "EngineLocator.h"
#include "ILogger.h"
#include "IModule.h"
#include "SDK.h"

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

        Runtime::setFNameEntries(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddr));
        Runtime::setUObjects(reinterpret_cast<TArray<UObject*>*>(uObjectsAddr));

        log_->debug("engine valid: {}", std::to_string(Runtime::areUObjectsPopulated() && Runtime::areFNameEntriesValid()));

        //log_->info("FNameEntry::Flags     = 0x" + int_to_hex(offsetof(FNameEntry, Flags)));
        //log_->info("FNameEntry::Index     = 0x" + int_to_hex(offsetof(FNameEntry, Index)));
        //log_->info("FNameEntry::Name      = 0x" + int_to_hex(offsetof(FNameEntry, Name)));
        //log_->info("FNameEntry sizeof     = 0x" + int_to_hex(sizeof(FNameEntry)));

        return (Runtime::areUObjectsPopulated() && Runtime::areFNameEntriesValid());
    }

    static auto hasUObjects() -> bool { return Runtime::hasUObjects(); }
    static auto hasFNames() -> bool { return Runtime::hasFNames(); }

    auto getUObjectsPtr() -> TArray<UObject*>* {
        return Runtime::getUObjectsPtr();
    }

    auto getUObjects() -> TArray<UObject*> {
        return Runtime::getUObjects();
    }

    auto findClass(const std::string& classFullName) -> UClass* {
        return Runtime::findClass(classFullName);
    }

    auto findFunction(const std::string& functionFullName) -> UFunction* {
        return Runtime::findFunction(functionFullName);
    }

    auto areFNameEntriesValid() -> bool {
        return Runtime::areFNameEntriesValid();
    }

    auto areUObjectsPopulated() -> bool {
        return Runtime::areUObjectsPopulated();
    }

    auto getFNameEntries() -> TArray<FNameEntry*>& {
        return Runtime::getFNameEntries();
    }

    auto getFNameEntriesPtr() -> TArray<FNameEntry*>* {
        return Runtime::getFNameEntriesPtr();
    }

    auto getFNameEntry(int32_t index) -> FNameEntry* {
        return Runtime::getFNameEntry(index);
    }

    auto getFNameEntryName(int32_t index) -> std::string {
        return Runtime::getFNameEntryName(index);
    }

    auto findPackages() -> std::vector<UObject*> {
        return Runtime::findPackages();
    }

    auto getRawObjects() -> const std::vector<UObject*>& {
        return Runtime::getRawObjects();
    }

    auto getUObjectsCache() -> std::vector<UObject*>& {
        return Runtime::getObjectCache();
    }

    //void shutdown() {
    //    Runtimme::shutdown();
    //}
};
