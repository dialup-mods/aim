#include <memory>
#include <optional>

#include "fmt/format.h"

#include "EClassTypes.h"
#include "Logger.h"
#include "Runtime.h"
#include "Schema.h"
#include "TypeRules.h"
#include "WaveWorker.h"

#include "Object.h"
#include "UClass.h"
#include "UConst.h"
#include "UEnum.h"
#include "UObject.h"
#include "UScriptStruct.h"

#include "UArrayProperty.h"
#include "UBoolProperty.h"
#include "UByteProperty.h"
#include "UClassProperty.h"
#include "UDelegateProperty.h"
#include "UFloatProperty.h"
#include "UFunction.h"
#include "UIntProperty.h"
#include "UInterfaceProperty.h"
#include "UMapProperty.h"
#include "UNameProperty.h"
#include "UObjectProperty.h"
#include "UQWordProperty.h"
#include "UStrProperty.h"
#include "UStructProperty.h"

using r = Runtime;

auto
ObjectStore::isProbablyValidPtr(const uintptr_t ptr) -> bool {
    if (!ptr) {
        storeInvalidObject(ptr, InvalidUObjectReason::NullObject);
    }

    if (!(ptr > 0x10000 && ptr < 0x7FFFFFFFFFFF)) { // NOLINT(*-avoid-magic-numbers)
        storeInvalidObject(ptr, InvalidUObjectReason::InvalidAddress);
        return false;
    }
    return true;
}

void
ObjectStore::storeInvalidObject(UObject* rawObj, const InvalidUObjectReason reason) {
    invalidObjects_[reinterpret_cast<uintptr_t>(rawObj)] = reason;
}

void
ObjectStore::storeInvalidObject(uintptr_t ptr, const InvalidUObjectReason reason) {
    invalidObjects_[ptr] = reason;
}

auto
ObjectStore::isProbablyValidUObject(UObject* rawObj) -> bool {
    auto addr = reinterpret_cast<uintptr_t>(rawObj);
    if (!isProbablyValidPtr(addr)) {
        return false;
    }

    if (rawObj->Outer && !isProbablyValidPtr(reinterpret_cast<uintptr_t>(rawObj->Outer))) {
        storeInvalidObject(rawObj, InvalidUObjectReason::InvalidOuterAddress);
        return false;
    }

    if (!rawObj->Class) {
        storeInvalidObject(rawObj, InvalidUObjectReason::NullClass);
        return false;
    }

    if (static_cast<std::string_view>(rawObj->Name.ToString()).find("Default__") != std::string::npos) {
        storeInvalidObject(rawObj, InvalidUObjectReason::DefaultObject);
        return false;
    }

    return true;
}

auto
ObjectStore::getTotalGObjObjectsCount() const -> size_t {
    return Runtime::uobject::cache::rawObjects().size();
}

auto
ObjectStore::getTotalSeenCount() const -> size_t {
    return seen_.size();
}

auto
ObjectStore::getInvalidCount() const -> size_t {
    return invalidPtrs_.size();
}

void ObjectStore::iterateObjects(const std::function<bool(UObject*)>& fn) {
    for (UObject* obj : Runtime::uobject::game_pool::ref()) {
        if (!obj) continue;
        if (!fn(obj)) break;
    }
}

void
ObjectStore::initialize() {
    int i = 0;
    WaveWorker wave(std::chrono::milliseconds(30));
    wave.start();

    iterateObjects([&](UObject* raw) {

        // if it's invalid, go ahead and add the pointer to the `seen` list
        // so we never try to turn it into a cache entry, and instead
        // short-circuit the `add()` logic
        if (!isProbablyValidPtr(reinterpret_cast<uintptr_t>(raw))) {
            seen_.insert(raw);
            i++;
            return true; // continue to next iteration
        }

        try {
            add(raw, "GObjObjects");
        } catch (const std::exception& e) {
            Logger::instance().log("[ERROR] Exception in add() at index {}: {}", i, e.what());
        } catch (...) {
            Logger::instance().log("[ERROR] Unknown error in add() at index {}", i);
        }

        i++;
        return true;
    });
    wave.stop();
}

auto ObjectStore::existingEntryFor(const UObject* obj) const -> ObjectEntry* {
    if (!obj) return nullptr;
    for (const auto& entry : all_) {
        if (entry && entry->getObject() == obj) {
            return entry.get();
        }
    }
    return nullptr;
}

// add an object to the store and return the resulting ObjectEntry*
// returns ObjectEntry* from object store if it exists
// early returns nullptr if the ObjectEntry can't be created (invalid)
// early returns nullptr if the address has been seen before but no valid entry exists
auto ObjectStore::add(UObject* rawObj, const std::string& origin = "") -> ObjectEntry* {
    if (seen_.contains(rawObj)) {
        auto* seenEntry = objectToEntry_[rawObj];
        // seenEntry->addReferrer(origin);
        return seenEntry;
    }

    seen_.insert(rawObj);

    if (!isProbablyValidUObject(rawObj)) {
        seen_.insert(rawObj);
        return nullptr;
    }

    //Logger::instance().log("class: " + rawObj->Class->GetFullName() + "\n  origin: " + origin);
    //Logger::instance().log("UClass::StaticClass(): " + UClass::StaticClass()->GetName());
    //Logger::instance().log("UScriptStruct::StaticClass(): " + UScriptStruct::StaticClass()->GetName());

    std::unique_ptr<ObjectEntry> entry;
    if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ArrayProperty"))) {
        entry = std::make_unique<UArrayPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.StrProperty"))) {
        entry = std::make_unique<UStrPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.IntProperty"))) {
        entry = std::make_unique<UIntPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.FloatProperty"))) {
        entry = std::make_unique<UFloatPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.DelegateProperty"))) {
        entry = std::make_unique<UDelegatePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.NameProperty"))) {
        entry = std::make_unique<UNamePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.StructProperty"))) {
        entry = std::make_unique<UStructPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ClassProperty"))) {
        entry = std::make_unique<UClassPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ObjectProperty"))) {
        entry = std::make_unique<UObjectPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.MapProperty"))) {
        entry = std::make_unique<UMapPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.InterfaceProperty"))) {
        entry = std::make_unique<UInterfacePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.QWordProperty"))) {
        entry = std::make_unique<UQWordPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.BoolProperty"))) {
        entry = std::make_unique<UBoolPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ByteProperty"))) {
        entry = std::make_unique<UBytePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Enum"))) {
        entry = std::make_unique<EnumEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Class"))) {
        entry = std::make_unique<ClassEntry>(rawObj);
        nameToEntry_[rawObj->Name.ToString()] = entry.get();
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Function"))) {
        entry = std::make_unique<UFunctionEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ScriptStruct"))) {
        entry = std::make_unique<UScriptStructEntry>(rawObj);
        // fixme getFullName is empty
        nameToStruct_[entry->getFullName()] = entry.get();
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.State"))) {
        //Logger::instance().log("[WARN] UState. Not yet implemented. {}", entry->getFullName());
        return nullptr;
        // fixme unimplemented
        //entry = std::make_unique<UState>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Struct"))) {
        Logger::instance().log("[WARN] Raw UStruct.");
        entry = std::make_unique<UStructEntry>(rawObj);
        nameToStruct_[entry->getFullName()] = entry.get();
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Const"))) {
        entry = std::make_unique<ConstEntry>(rawObj);
        nameToStruct_[entry->getFullName()] = entry.get();
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Object"))) {
        entry = std::make_unique<UObjectEntry>(rawObj);
    } else {
        entry = std::make_unique<ObjectEntry>(rawObj);
        // fixme getFullName will crash on invalid elements
        Logger::instance().log("[WARN] Using fallback type for: {}", entry->getFullName());
    }

//    Logger::instance().log("entry: {}", entry->asString());

    entry->setOrigin(origin);

    if (!entry->isValid()) {
        // fixme getFullName will crash on invalid elements
        Logger::instance().log("[WARN] Invalid object detected: {}", entry->getFullName());
        return nullptr;
    }

    all_.emplace_back(std::move(entry));

    ObjectEntry* ptr = all_.back().get();
    objectToEntry_[rawObj] = ptr;

    return ptr;
}

auto ObjectStore::getStructByName(const std::string& given) -> UScriptStructEntry* {
    for (auto& [storedName, entry] : nameToStruct_) {
        if (storedName == given) {
            return static_cast<UScriptStructEntry*>(entry);
        }
    }
    return nullptr;
}

auto ObjectStore::getAllStructEntries() -> std::vector<UScriptStructEntry*> {
    std::vector<UScriptStructEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UScriptStruct) {
            result.emplace_back(static_cast<UScriptStructEntry*>(entry.get()));
        }
        //else if (entry->getType() == EClassTypes::UStruct) {
        //    Logger::instance().log("[ERROR] skipping UStruct, {}", entry->getFullName());
        //}
    }
    Logger::instance().log("returning {} struct entries", result.size());
    return result;
}

auto ObjectStore::getAllEnumEntries() -> std::vector<EnumEntry*> {
    std::vector<EnumEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UEnum) {
            result.emplace_back(static_cast<EnumEntry*>(entry.get()));
        }
    }
    Logger::instance().log("returning {} enum entries", result.size());
    return result;
}

auto ObjectStore::getAllClassEntries() -> std::vector<ClassEntry*> {
    std::vector<ClassEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UClass) {
            result.emplace_back(static_cast<ClassEntry*>(entry.get()));
        }
    }
    Logger::instance().log("returning {} class entries", result.size());
    return result;
}

auto ObjectStore::getAllConstEntries() -> std::vector<ConstEntry*> {
    std::vector<ConstEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UConst) {
            result.emplace_back(static_cast<ConstEntry*>(entry.get()));
        }
    }
    Logger::instance().log("returning {} const entries", result.size());
    return result;
}

auto ObjectStore::getAllFunctionEntries() -> std::vector<UFunctionEntry*> {
    std::vector<UFunctionEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UFunction) {
            result.emplace_back(static_cast<UFunctionEntry*>(entry.get()));
        }
    }
    Logger::instance().log("returning {} function entries", result.size());
    return result;
}

auto
ObjectStore::countStructsWithName(const std::string_view sanitizedName) -> int {
    // fixme performance -- maybe use the structs map
    return std::ranges::count_if(all_, [&](const std::unique_ptr<ObjectEntry>& entry) {
        return entry->getSanitizedName() == sanitizedName && entry->getType() == EClassTypes::UStruct;
    });
}

auto ObjectStore::get(UObject* obj) const -> ObjectEntry* {
    if (!obj) return nullptr;
    auto it = objectToEntry_.find(obj);
    return it != objectToEntry_.end() ? it->second : nullptr;
}

auto ObjectStore::getClassEntryByName(std::string& name) -> ClassEntry* {
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UClass) {
            return static_cast<ClassEntry*>(entry.get());
        }
    }
    return nullptr;
}