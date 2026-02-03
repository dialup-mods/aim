#pragma once
#ifdef false
// TODO: refactor to use new Runtime module
// TODO: determine what's still useful
// TODO: rewrite anything using string comparisons for searching
// TODO: add tests
// notes on what

#include <functional>
#include <string>
#include <typeindex>

#include "ILogger.h"
#include "IModule.h"
#include "IObjectQuery.h"
#include "RuntimeWrapper.h"
#include "SDK.h"

class PluginState;
class AsyncGate;
class RuntimeWrapper;

using r = Runtime;

class AIM_API ObjectQuery : public IObjectQuery {
    AIM_INJECTABLE(ObjectProvider)

    AIM_INJECT(ILogger, log)
    AIM_INJECT(AsyncGate, asyncGate)
    AIM_INJECT(RuntimeWrapper, runtime)

public:

    ObjectQuery() {}


    // fix me, use flags, see getInstanceOf

    //bool isUStruct(const UObject* obj) {
    //    if (!obj) return false;

    //    // Reinterpret and sanity-check layout
    //    const auto* s = reinterpret_cast<const UStruct*>(obj);

    //    // UStruct always has PropertySize and Children
    //    return s->PropertySize >= 0
    //        && s->Children != nullptr;
    //}
    //bool isUClass(const UObject* obj) {
    //    if (!isUStruct(obj)) return false;

    //    const auto* cls = reinterpret_cast<const UClass*>(obj);

    //    // UClass-only invariant
    //    return cls->ClassDefaultConstructor != nullptr;
    //}

    // usage:
    // forEachObject<UFunction>([](UFunction* fn) {
    //     printf("Function: %s", *fn->GetFullName());
    // });
//    template<typename T, typename Predicate>
//    void forEachObject(Predicate&& pred) {
//        auto& objects = r::uobject::game_pool::ref();
//
//        for (int i = objects.size(); i > 0; i--) {
//            UObject* obj = objects.at(i);
//
//            if (!obj || !obj->Class || !obj->IsA(T::StaticClass())) {
//                continue;
//            }
//
//            pred(static_cast<T*>(obj));
//        }
//    }

//    template<typename T, typename Predicate>
//    void forEachValidLiveObject(Predicate&& pred) {
//        static_assert(std::is_base_of_v<UObject, T>, "T must be a UObject-derived type");
//
//        auto& objects = r::uobject::game_pool::ref();
//
//        for (int i = objects.size(); i > 0; i--) {
//            UObject* obj = objects.at(i);
//            if (!obj) { continue; }
//
//            uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
//            if (addr < 0x10000 || addr > 0xFFFFFFFFFF) { continue; } // skip obviously bad memory
//            if (!obj->Class) { continue; }
//            if (!obj->IsA(T::StaticClass())) { continue; }
//            if (r::uobject_utils::hasAnyFlags(obj, static_cast<EObjectFlags>(RF_BeginDestroyed || RF_FinishDestroyed || RF_DefaultOrArchetypeFlags))) { continue; }
//
//            // fixme special checks for UGFxData_Chat_TA
//            if constexpr (std::is_same_v<T, UGFxData_Chat_TA>) {
//                T* chat = static_cast<T*>(obj);
//                if (!chat->Shell || !chat->Shell->DataStore)
//                    continue;
//            }
//
//            if (!pred(static_cast<T*>(obj))) {
//                break;
//            }
//        }
//    }

//    template<typename T>
//    T* findLiveInstanceOf() {
//        return findFirstWhere<T>([](T*) { return true; });
//    }
//
//    template<typename T>
//    T* findObjectByName(const std::string& name, bool strict = false) {
//        return findFirstWhere<T>([&](T* candidate) {
//            const std::string& fullName = candidate->GetFullName();
//            return strict ? (fullName == name) : (fullName.find(name) != std::string::npos);
//        });
//    }

//    template<typename T, typename Predicate>
//    T* findFirstWhere(Predicate&& predicate) {
//        T* result = nullptr;
//
//        forEachValidLiveObject<T>([&](T* candidate) {
//            if (predicate(candidate)) {
//                result = candidate;
//                return false; // stop iteration
//            }
//        });
//
//        return result;
//    }

//    template<typename T, typename Predicate>
//    std::vector<T*> findAllWhere(Predicate&& predicate) {
//        std::vector<T*> results;
//
//        forEachValidLiveObject<T>([&](T* candidate) {
//            if (predicate(candidate)) {
//                results.push_back(candidate);
//            }
//        });
//
//        return results;
//    }

    // Fast path - cache only
    //template<typename T, typename Predicate>
    //auto findFirstCached(Predicate&& pred) -> T* {
    //    for (auto* obj : r::uobject::cache::get()) {
    //        if (auto* typed = dynamic_cast<T*>(obj)) {
    //            if (pred(typed)) return typed;
    //        }
    //    }
    //    return nullptr;
    //}

    // Slow path - game pool only
    //template<typename T, typename Predicate>
    //auto findFirstInGame(Predicate&& pred) -> T* {
    //    for (auto* obj : r::uobject::game_pool::ref()) {
    //        if (!obj || !obj->Class) continue;
    //        if (!r::uobject::isA(obj, T::className)) continue;
    //        if (pred(static_cast<T*>(obj))) return obj;
    //    }
    //    return nullptr;
    //}

    // Smart path - cache with fallback
    //template<typename T, typename Predicate>
    //auto findFirst(Predicate&& pred) -> T* {
    //    // Try cache first (fast)
    //    if (auto* result = findFirstCached<T>(pred)) {
    //        return result;
    //    }

    //    // Fall back to game pool (slow)
    //    if (auto* result = findFirstInGame<T>(pred)) {
    //        r::uobject::cache::add(result);  // Cache for next time
    //        return result;
    //    }

    //    return nullptr;
    //}

//    template<typename T>
//    auto get() -> T* {
//        log_->debug("get() called");
//        log_->debug("get() Requested type: {}", typeid(T).name());
//
//        // Use type_index as the key (more efficient than type_info)
//        std::type_index typeIdx(typeid(T));
//
//        // Check if we already have an instance in the cache
//        auto it = instanceCache_.find(typeIdx);
//        if (it != instanceCache_.end()) {
//            return static_cast<T*>(it->second);
//        }
//
//        // Not in cache, get the instance
//        log_->debug("not in cache");
//        T* instance = getInstanceOf<T>();
//
//        // Store in cache if found
//        if (instance) {
//            instanceCache_[typeIdx] = instance;
//        }
//
//        return instance;
//    }

    //    template<typename T>
    //    auto getInstanceOf() -> T* {
    //        T* result = nullptr;
    //
    //        forEachObject<T>([&result, this](T* candidate) {
    //            // if (this->isValidLiveInstance(candidate)) {
    //            result = candidate;
    //            return; // early exit (can break if you extend walker)
    //            //}
    //        });
    //
    //        return result;
    //    }

    //template<typename T>
    //std::vector<T*> getCandidatesOf() {
    //    static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");

    //    std::vector<T*> results;

    //    auto& objects = r::uobject::game_pool::ref();

    //    for (int i = objects.size(); i > 0; i--) {
    //        UObject* obj = objects.at(i);
    //        if (!obj) { continue; }

    //        if (!obj->Class || !obj->IsA<T>()) { continue; }

    //        results.push_back(static_cast<T*>(obj));
    //    }

    //    return results;
    //}

    // This guarantees you’re grabbing the instance actually wired into the frontend / HUD.
    //bool hasValidUIOuter(const UObject* obj) {
    //    for (UObject* outer = obj->Outer; outer; outer = outer->Outer) {
    //        if (outer->IsA(UInteraction::StaticClass()))
    //            return true;
    //    }
    //    return false;
    //}

//    template<typename T>
//    auto getAllInstancesOf() -> std::vector<T*> {
//        std::vector<T*> results;
//
//        forEachObject<T>([&results, this](T* candidate) {
//            if (this->isValidLiveInstance(candidate)) {
//                results.push_back(candidate);
//            }
//        });
//
//        return results;
//    }
//
//    template<typename T>
//    std::vector<T*> getAllLiveInstancesOf() {
//        std::vector<T*> results;
//        forEachValidLiveObject<T>([&](T* candidate) { results.push_back(candidate); });
//        return results;
//    }

    // get all default instances of a class type.
    //template<typename T>
    //auto getAllDefaultInstancesOf() -> std::vector<T*> {
    //    if (!std::is_base_of_v<UObject, T>) { return nullptr; }

    //    std::vector<T*> objectInstances;

    //    auto& objects = r::uobject::game_pool::ref();

    //    for (int i = objects.size(); i > 0; i--) {
    //        UObject* uObject = r::uobject::game_pool::ref().at(i);

    //        if (uObject && uObject->IsA<T>()) {
    //            if (uObject->GetFullName().find("Default__") != std::string::npos) {
    //                objectInstances.push_back(static_cast<T*>(uObject));
    //            }
    //        }
    //    }
    //    return objectInstances;
    //}

//    template<typename T>
//    std::vector<T*> findAllObjects(const std::string& name) {
//        return findAllWhere<T>([&](T* candidate) { return candidate->GetFullName().find(name) != std::string::npos; });
//    }

    // get the default constructor of a class type. Example: UGameData_TA* gameData = GetDefaultInstanceOf<UGameData_TA>();
    //template<typename T>
    //auto getDefaultInstanceOf() -> T* {
    //    if (!std::is_base_of_v<UObject, T>) { return nullptr; }

    //    auto& objects = r::uobject::game_pool::ref();

    //    for (int i = objects.size(); i > 0; i--) {
    //        UObject* uObject = r::uobject::game_pool::ref().at(i);

    //        if (uObject && uObject->IsA<T>()) {
    //            if (uObject->GetFullName().find("Default__") != std::string::npos) {
    //                return static_cast<T*>(uObject);
    //            }
    //        }
    //    }

    //    return nullptr;
    //}

    // get an object instance by its name and class type. Example: UTexture2D* texture = FindObject<UTexture2D>("WhiteSquare");
    //template<typename T>
    //auto findObject(const std::string& objectName, bool bStrictFind) -> T* {
    //    if (!std::is_base_of_v<UObject, T>) { return nullptr; }
    //    auto& objects = r::uobject::game_pool::ref();

    //    for (int i = objects.size(); i > 0; i--) {
    //        UObject* uObject = r::uobject::game_pool::ref().at(i);

    //        if (uObject && uObject->IsA<T>()) {
    //            std::string objectFullName = uObject->GetFullName();

    //            if (bStrictFind) {
    //                if (objectFullName == objectName) {
    //                    return static_cast<T*>(uObject);
    //                }
    //            } else if (objectFullName.find(objectName) != std::string::npos) {
    //                return static_cast<T*>(uObject);
    //            }
    //        }
    //    }

    //    return nullptr;
    //}

    // Creates a new transient instance of a class which then adds it to globals.
    // YOU are required to make sure these objects eventually get eaten up by the garbage collector in some shape or form.
    // Example: UObject* newObject = CreateInstance<UObject>();
    //template<typename T>
    //auto createInvincibleInstance() -> T* {
    //    T* object = nullptr;

    //    if (std::is_base_of_v<UObject, T>) {
    //        T* defaultObject = this->getDefaultInstanceOf<T>();
    //        UClass* staticClass = T::StaticClass();

    //        if (defaultObject && staticClass) {
    //            object = static_cast<T*>(defaultObject->DuplicateObject(defaultObject, defaultObject->Outer, staticClass));
    //        }

    //        // Making sure newly created object doesn't get randomly destroyed by the garbage collector when we don't want it do.
    //        //            if (object) {
    //        //                // use this instead
    //        //                object->ObjectFlags &= ~EObjectFlags::RF_Transient;
    //        //                object->ObjectFlags &= ~EObjectFlags::RF_TagGarbage;
    //        //                object->ObjectFlags &= ~EObjectFlags::RF_PendingKill;
    //        //                object->ObjectFlags |= EObjectFlags::RF_DisregardForGC;
    //        //                object->ObjectFlags |= EObjectFlags::RF_RootSet;
    //        //                MemoryManager::MarkInvincible(object);
    //        createdObjects_.push_back(object);
    //    }

    //    return object;
    //}

    // Get the most current/active instance of a class, if one isn't found it creates a new instance. Example: UEngine* engine =
    // GetInstanceOf<UEngine>();
    //template<typename T>
    //auto getOrCreateInvincibleInstance() -> T* {
    //    if (!std::is_base_of_v<UObject, T>) { return nullptr; }
    //    auto& objects = r::uobject::game_pool::ref();

    //    for (int i = objects.size(); i > 0; i--) {
    //        UObject* uObject = objects.at(i);

    //        if (uObject && uObject->IsA<T>()) {
    //            // if (uObject->GetFullName().find("Default__") == std::string::npos)
    //            if (this->checkNotInName(uObject, "Default") && this->checkNotInName(uObject, "Archetype") &&
    //                this->checkNotInName(uObject, "PostGameLobby") && this->checkNotInName(uObject, "Test")) {
    //                return static_cast<T*>(uObject);
    //            }
    //        }
    //    }
    //    return this->createInvincibleInstance<T>();
    //}

    //auto getFullName(UObject* obj) -> std::string;
    //auto findStaticClass(const std::string& className) -> class UClass*;

    //auto findStaticFunction(const std::string& functionName) -> class UFunction*;

    //auto checkNotInName(UObject* obj, const std::string& str) -> bool;

    //template<typename T>
    //bool isValidAndA(UObject* obj) {
    //    uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
    //    return obj && addr > 0x10000 && addr < 0xFFFFFFFFFF && obj->Class && obj->IsA(T::StaticClass());
    //}

    //template<typename T>
    //auto getLiveInstanceOf() -> T* {
    //    static_assert(std::is_base_of_v<UObject, T>, "T must be a UObject-derived type");

    //    log_->debug("getLiveInstanceOf<>() called");

    //    auto& objects = r::getUObjects();
    //    const size_t limit = std::min(iterateLimit_, objects.size());

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

    //        log_->debug("Found live instance: {}", obj->GetFullName());
    //        return candidate;
    //    }

    //    log_->warn("No valid live instance found");
    //    return nullptr;
    //}

    // untested
    // void FindTextField(UGFxObject* obj, const std::wstring& targetName, int depth = 0) {
    //    if (!obj) return;
    //
    //    // Get all member names of this object
    //    TArray<FString> memberNames;
    //    obj->GetMemberNames(memberNames);
    //
    //    for (const FString& member : memberNames) {
    //        FString value;
    //        if (member.ToWideString() == targetName) {
    //            auto* child = obj->GetObject(member);
    //            if (child) {
    //                log_->info(L"🎯 Found text field: {} (depth {})", member.ToWideString(), depth);
    //                child->SetText(FString::SafeFString(L"SENATOR AOL")); // Try override!
    //            }
    //        }
    //
    //        // Dive deeper if it's another object
    //        auto* nested = obj->GetObject(member);
    //        if (nested && nested != obj) {
    //            FindTextField(nested, targetName, depth + 1);
    //        }
    //    }
    //
    //    // Optionally iterate elements too (if it's an array-like object)
    //    for (int i = 0; i < 64; ++i) {
    //        auto* el = obj->GetElement(i);
    //        if (!el) break;
    //        FindTextField(el, targetName, depth + 1);
    //    }
    //}

  private:

    //mutable std::unordered_map<std::type_index, std::function<void*()>> lookupCache_;
    // Game-specific cached instances
    //    AGFxHUD_TA* hud;
    //    UGFxDataStore_X* dataStore;
    //    USaveData_TA* saveData;
    //    UOnlinePlayer_X* onlinePlayer;
};
#endif