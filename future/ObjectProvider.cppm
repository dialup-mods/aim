module;

// fixme remove from public repo

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module ObjectProvider;

import "SdkHeaders.hpp";
import "IModule.h";
import "SafeLogger.h";
import EngineValidator;
import "AsyncGate.h";

static constexpr int32_t UOBJECTS_ITERATE_OFFSET = 100;

export class ObjectProvider : public IModule {
    AIM_INJECTABLE(ObjectProvider)

    AIM_INJECT(SafeLogger, log)
    AIM_INJECT(EngineValidator, engineValidator)
    AIM_INJECT(AsyncGate, asyncGate)

  public:
    ObjectProvider() = default;
    ~ObjectProvider() {
        log_->debug("ObjectProvider unloading...");
        m_staticClasses.clear();
        log_->debug("ObjectProvider unloaded");
    }

    void init() {
        asyncGate_->setReady();
        log_->debug("ObjectProvider initialized.");
    }

    static void cleanup() {}

    //
    // ProfileQuickChatSave_TAs
    // if there are multiple instances,
    // you can scan and pick the one with the biggest Bindings array
    // (sometimes Rocket League uses Bindings length to track valid saves).
    //
    // fix me, use flags, see getInstanceOf
    template<typename T>
    bool isValidLiveInstance(T* obj) {
        if (!obj)
            return false;
        auto name = obj->GetFullName();
        return name.find("Default__") == std::string::npos && name.find("Archetype") == std::string::npos &&
            name.find("PostGameLobby") == std::string::npos && name.find("Test") == std::string::npos;
    }

    template<typename T, typename Func>
    void forEachValidLiveObject(Func&& func) {
        static_assert(std::is_base_of_v<UObject, T>, "T must be a UObject-derived type");

        auto* objects = UObject::GObjObjects();
        if (!objects) {
            log_->warn("⚠️ GObjObjects() returned nullptr", LogCategory::CORE);
            return;
        }

        int32_t numObjects = objects->Num();
        if (numObjects <= UOBJECTS_ITERATE_OFFSET) {
            log_->warn("⚠️ Not enough objects to iterate", LogCategory::CORE);
            return;
        }

        log_->debug("about to iter", LogCategory::CORE);
        for (int32_t i = numObjects - 1; i >= UOBJECTS_ITERATE_OFFSET; --i) {
            log_->debug("about to cast", LogCategory::CORE);
            UObject* obj = objects->At(i);

            if (!obj)
                continue;

            log_->debug("was obj", LogCategory::CORE);
            uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
            if (addr < 0x10000 || addr > 0xFFFFFFFFFF)
                continue; // skip obviously bad memory

            log_->debug("was addr good", LogCategory::CORE);
            if (!obj->Class)
                continue;

            log_->debug("has class", LogCategory::CORE);
            if (!obj->IsA(T::StaticClass()))
                continue;

            log_->debug("is static class", LogCategory::CORE);
            if (obj->HasAnyFlags(EObjectFlags::RF_BeginDestroyed || EObjectFlags::RF_FinishDestroyed))
                continue;

            log_->debug("has flags", LogCategory::CORE);
            const std::string& fullName = obj->GetFullName();
            if (fullName.find("Default__") != std::string::npos)
                continue; // skip class default objects

            log_->debug("not default", LogCategory::CORE);
            // If it's UGFxData_Chat_TA, special checks
            if constexpr (std::is_same_v<T, UGFxData_Chat_TA>) {
                T* chat = static_cast<T*>(obj);
                if (!chat->Shell || !chat->Shell->DataStore)
                    continue;
            }

            func(static_cast<T*>(obj));
        }
    }

    template<typename T>
    T* findLiveInstanceOf() {
        return findFirstWhere<T>([](T*) { return true; });
    }

    template<typename T, typename Predicate>
    void forEachObject(Predicate&& pred) {
        auto* objects = UObject::GObjObjects();
        if (!objects)
            return;

        int32_t numObjects = objects->Num();
        if (numObjects <= UOBJECTS_ITERATE_OFFSET)
            return;

        for (int32_t i = numObjects - 1; i >= UOBJECTS_ITERATE_OFFSET; --i) {
            UObject* obj = objects->At(i);
            if (!obj || !obj->Class || !obj->IsA(T::StaticClass()))
                continue;

            pred(static_cast<T*>(obj));
        }
    }

    template<typename T>
    T* findObjectByName(const std::string& name, bool strict = false) {
        return findFirstWhere<T>([&](T* candidate) {
            const std::string& fullName = candidate->GetFullName();
            return strict ? (fullName == name) : (fullName.find(name) != std::string::npos);
        });
    }

    template<typename T, typename Predicate>
    T* findFirstWhere(Predicate&& predicate) {
        T* result = nullptr;

        forEachValidLiveObject<T>([&](T* candidate) {
            if (predicate(candidate)) {
                result = candidate;
                return;
            }
        });

        return result;
    }

    template<typename T, typename Predicate>
    std::vector<T*> findAllWhere(Predicate&& predicate) {
        std::vector<T*> results;

        forEachValidLiveObject<T>([&](T* candidate) {
            if (predicate(candidate)) {
                results.push_back(candidate);
            }
        });

        return results;
    }

    template<typename T>
    auto get() -> T* {
        log_->debug("get() called", LogCategory::CORE);
        log_->debug(std::string("get() Requested type: ") + typeid(T).name(), LogCategory::CORE);

        // Use type_index as the key (more efficient than type_info)
        std::type_index typeIdx(typeid(T));

        // Check if we already have an instance in the cache
        auto it = instanceCache_.find(typeIdx);
        if (it != instanceCache_.end()) {
            return static_cast<T*>(it->second);
        }

        // Not in cache, get the instance
        log_->debug("not in cache", LogCategory::CORE);
        T* instance = getInstanceOf<T>();

        // Store in cache if found
        if (instance) {
            instanceCache_[typeIdx] = instance;
        }

        return instance;
    }

    template<typename T>
    std::vector<T*> getCandidatesOf() {
        static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");

        std::vector<T*> results;

        auto* objects = UObject::GObjObjects();
        if (!objects)
            return results;

        int32_t count = objects->Num();
        if (count <= UOBJECTS_ITERATE_OFFSET)
            return results;

        for (int32_t i = count - 1; i >= UOBJECTS_ITERATE_OFFSET; --i) {
            UObject* obj = objects->At(i);
            if (!obj)
                continue;

            if (!obj->Class || !obj->IsA<T>())
                continue;

            log_->debug("name: " + obj->Name.ToString());
            log_->debug("outer: " + obj->Outer->Name.ToString());

            std::stringstream ss;
            ss << "flags: 0x" << std::uppercase << std::hex << obj->ObjectFlags;
            log_->debug(ss.str());

            results.push_back(static_cast<T*>(obj));
        }

        return results;
    }

    template<typename T>
    T* getInstanceOf2() {
        if (!std::is_base_of<UObject, T>::value)
            return nullptr;

        for (int32_t i = UObject::GObjObjects()->size() - UOBJECTS_ITERATE_OFFSET; i > 0; --i) {
            UObject* uObject = UObject::GObjObjects()->at(i);
            if (!uObject || !uObject->IsA<T>())
                continue;

            // Must have valid class
            if (!uObject->Class)
                continue;

            // Must not be pending kill / being destroyed
            if (uObject->ObjectFlags & (RF_PendingKill | RF_BeginDestroyed | RF_FinishDestroyed))
                continue;

            // If it's the class default object, only accept it if it's under TAGame
            if (uObject->ObjectFlags & RF_ClassDefaultObject) {
                if (uObject->Outer && uObject->Outer->Name.ToString() == "TAGame")
                    return static_cast<T*>(uObject);

                continue; // skip other CDOs
            }

            // Prefer non-transient objects
            if (uObject->Outer && uObject->Outer->Name.ToString() == "Transient")
                continue;

            return static_cast<T*>(uObject);
        }

        return nullptr;
    }

    template<typename T>
    T* getInstanceOf() const {
        if (!std::is_base_of<UObject, T>::value)
            return nullptr;

        for (int32_t i = (UObject::GObjObjects()->size() - UOBJECTS_ITERATE_OFFSET); i > 0; --i) {
            UObject* uObject = UObject::GObjObjects()->at(i);
            if (!uObject)
                continue;

            if (!uObject->IsA<T>())
                continue;

            if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags)
                continue;

            return static_cast<T*>(uObject);
        }

        return nullptr;
    }

    template<typename T>
    auto getAllInstancesOf() -> std::vector<T*> {
        std::vector<T*> results;

        forEachObject<T>([&results, this](T* candidate) {
            if (this->isValidLiveInstance(candidate)) {
                results.push_back(candidate);
            }
        });

        return results;
    }

    template<typename T>
    std::vector<T*> getAllLiveInstancesOf() {
        std::vector<T*> results;

        forEachValidLiveObject<T>([&](T* candidate) { results.push_back(candidate); });

        return results;
    }

    // get all default instances of a class type.
    template<typename T>
    auto getAllDefaultInstancesOf() -> std::vector<T*> {
        std::vector<T*> objectInstances;

        if (std::is_base_of_v<UObject, T>) {
            for (int32_t i = (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i > 0; i--) {
                UObject* uObject = engineValidator_->getObjects()->at(i);

                if (uObject && uObject->IsA<T>()) {
                    if (uObject->GetFullName().find("Default__") != std::string::npos) {
                        objectInstances.push_back(static_cast<T*>(uObject));
                    }
                }
            }
        }
        return objectInstances;
    }

    template<typename T>
    std::vector<T*> findAllObjects(const std::string& name) {
        return findAllWhere<T>([&](T* candidate) { return candidate->GetFullName().find(name) != std::string::npos; });
    }

    template<typename T>
    auto getTypeName() -> std::string {
        std::string objTypeName = typeid(T).name();

        // Remove "class " or "struct " if present
        const std::string classPrefix = "class ";
        const std::string structPrefix = "struct ";

        if (objTypeName.starts_with(classPrefix)) {
            objTypeName.erase(0, classPrefix.length()); // Erase the "class " prefix
        } else if (objTypeName.starts_with(structPrefix)) {
            objTypeName.erase(0, structPrefix.length()); // Erase the "struct " prefix
        }

        return objTypeName;
    }

    // get the default constructor of a class type. Example: UGameData_TA* gameData = GetDefaultInstanceOf<UGameData_TA>();
    template<typename T>
    auto getDefaultInstanceOf() -> T* {
        if (std::is_base_of_v<UObject, T>) {
            for (int32_t i = 0; i < (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i++) {
                UObject* uObject = engineValidator_->getObjects()->At(i);

                if (uObject && uObject->IsA<T>()) {
                    if (uObject->GetFullName().find("Default__") != std::string::npos) {
                        return static_cast<T*>(uObject);
                    }
                }
            }
        }

        return nullptr;
    }

    // get an object instance by its name and class type. Example: UTexture2D* texture = FindObject<UTexture2D>("WhiteSquare");
    template<typename T>
    auto findObject(const std::string& objectName, bool bStrictFind) -> T* {
        if (std::is_base_of_v<UObject, T>) {
            for (int32_t i = (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i > 0; i--) {
                UObject* uObject = engineValidator_->getObjects()->At(i);

                if (uObject && uObject->IsA<T>()) {
                    std::string objectFullName = uObject->GetFullName();

                    if (bStrictFind) {
                        if (objectFullName == objectName) {
                            return static_cast<T*>(uObject);
                        }
                    } else if (objectFullName.find(objectName) != std::string::npos) {
                        return static_cast<T*>(uObject);
                    }
                }
            }
        }

        return nullptr;
    }

    // Creates a new transient instance of a class which then adds it to globals.
    // YOU are required to make sure these objects eventually get eaten up by the garbage collector in some shape or form.
    // Example: UObject* newObject = CreateInstance<UObject>();
    template<typename T>
    auto createInvincibleInstance() -> T* {
        T* object = nullptr;

        if (std::is_base_of_v<UObject, T>) {
            T* defaultObject = this->getDefaultInstanceOf<T>();
            UClass* staticClass = T::StaticClass();

            if (defaultObject && staticClass) {
                object = static_cast<T*>(defaultObject->DuplicateObject(defaultObject, defaultObject->Outer, staticClass));
            }

            // Making sure newly created object doesn't get randomly destroyed by the garbage collector when we don't want it do.
            //            if (object) {
            //                // use this instead
            //                object->ObjectFlags &= ~EObjectFlags::RF_Transient;
            //                object->ObjectFlags &= ~EObjectFlags::RF_TagGarbage;
            //                object->ObjectFlags &= ~EObjectFlags::RF_PendingKill;
            //                object->ObjectFlags |= EObjectFlags::RF_DisregardForGC;
            //                object->ObjectFlags |= EObjectFlags::RF_RootSet;
            //                MemoryManager::MarkInvincible(object);
            createdObjects_.push_back(object);
        }

        return object;
    }

    // Get the most current/active instance of a class, if one isn't found it creates a new instance. Example: UEngine* engine =
    // GetInstanceOf<UEngine>();
    template<typename T>
    auto getOrCreateInvincibleInstance() -> T* {
        if (std::is_base_of_v<UObject, T>) {
            for (int32_t i = (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i > 0; i--) {
                UObject* uObject = engineValidator_->getObjects()->at(i);

                if (uObject && uObject->IsA<T>()) {
                    // if (uObject->GetFullName().find("Default__") == std::string::npos)
                    if (this->checkNotInName(uObject, "Default") && this->checkNotInName(uObject, "Archetype") &&
                        this->checkNotInName(uObject, "PostGameLobby") && this->checkNotInName(uObject, "Test")) {
                        return static_cast<T*>(uObject);
                    }
                }
            }
            return this->createInvincibleInstance<T>();
        }

        return nullptr;
    }

    auto getFullName(UObject* obj) -> std::string {
        if (obj) {
            return obj->GetFullName();
        }
        return "None";
    }

    auto findStaticClass(const std::string& className) -> class UClass* {
        // Lock the weak_ptr at the beginning of the method
        if (m_staticClasses.empty()) {
            for (int32_t i = 0; i < (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i++) {
                if (UObject* uObject = engineValidator_->getObjects()->At(i)) {
                    if ((uObject->GetFullName().find("Class") == 0)) {
                        m_staticClasses[uObject->GetFullName()] = static_cast<UClass*>(uObject);
                    }
                }
            }
        }

        if (m_staticClasses.contains(className)) {
            return m_staticClasses[className];
        }

        return nullptr;
    }

    template<typename T>
    UClass* findStaticClass() {
        // Prime the map once if needed
        if (m_staticClasses.empty()) {
            for (int32_t i = 0; i < (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i++) {
                if (UObject* uObject = engineValidator_->getObjects()->At(i)) {
                    if (uObject->GetFullName().starts_with("Class")) {
                        m_staticClasses[uObject->GetFullName()] = static_cast<UClass*>(uObject);
                    }
                }
            }
        }

        const std::string className = T::StaticClass()->GetFullName();

        if (m_staticClasses.contains(className)) {
            return m_staticClasses[className];
        }

        // Optionally cache it if not found in the initial pass (e.g. module registered late)
        UClass* cls = T::StaticClass();
        if (cls) {
            m_staticClasses[className] = cls;
        }

        return cls;
    }

    auto findStaticFunction(const std::string& functionName) -> class UFunction* {
        // Try cached first
        auto it = this->staticFunctions_.find(functionName);
        if (it != this->staticFunctions_.end()) {
            return it->second;
        }

        // Otherwise search
        for (int32_t i = (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i > 0; --i) {
            if (UObject* uObject = engineValidator_->getObjects()->At(i)) {
                if (uObject->IsA(UFunction::StaticClass())) {
                    std::string objName = uObject->GetFullName();
                    this->staticFunctions_[objName] = static_cast<UFunction*>(uObject);

                    if (objName == functionName) {
                        return static_cast<UFunction*>(uObject);
                    }
                }
            }
        }

        return nullptr;
    }

    // Core game instance getters
    auto getEngine() -> class UEngine* {
        UClass* engineClass = findStaticClass("Class Engine.Engine");
        if (!engineClass) {
            log_->error("Could not find UEngine static class");
            return nullptr;
        }

        for (int32_t i = 0; i < (engineValidator_->getObjects()->Num() - UOBJECTS_ITERATE_OFFSET); i++) {
            UObject* obj = engineValidator_->getObjects()->At(i);
            if (!obj)
                continue;

            if (obj->IsA(engineClass)) {
                std::string name = obj->GetName();
                if (name.find("Default__") == 0)
                    continue; // skip the CDO
                log_->debug("Found UEngine instance via scan: " + std::string(obj->GetFullName()));
                return static_cast<UEngine*>(obj);
            }
        }

        log_->error("No live UEngine instance found in scan");
        return nullptr;
    }

    UGameViewportClient* getViewportClient() {
        UEngine* engine = this->getEngine();
        if (!engine || !engine->GameViewport) {
            log_->error("Could not get GameViewport");
            return nullptr;
        }
        return engine->GameViewport;
    }

    auto getAudioDevice() -> class UAudioDevice* {
        return UEngine::GetAudioDevice();
    }

    auto getWorldInfo() -> class AWorldInfo* {
        return UEngine::GetCurrentWorldInfo();
    }

    auto getFileSystem() -> class UFileSystem* {
        return (UFileSystem*)UFileSystem::StaticClass();
    }

    auto getLocalPlayer() -> class ULocalPlayer* {
        if (UEngine* engine = this->getEngine(); engine && engine->GamePlayers[0]) {
            return engine->GamePlayers[0];
        }
        return nullptr;
    }

    auto getLocalPlayerTA() -> class ULocalPlayer_TA* {
        if (UEngine* engine = this->getEngine(); engine && !engine->GamePlayers.IsEmpty() && engine->GamePlayers[0]) {
            ULocalPlayer* player = engine->GamePlayers[0];
            if (player && player->IsA(ULocalPlayer_TA::StaticClass())) {
                return reinterpret_cast<ULocalPlayer_TA*>(player);
            }
        }
        return nullptr;
    }

    auto getLocalPlayerX() -> class ULocalPlayer_X* {
        if (UEngine* engine = this->getEngine(); engine && !engine->GamePlayers.IsEmpty() && engine->GamePlayers[0]) {
            ULocalPlayer* player = engine->GamePlayers[0];
            if (player && player->IsA(ULocalPlayer_TA::StaticClass())) {
                return reinterpret_cast<ULocalPlayer_TA*>(player);
            }
        }
        return nullptr;
    }

    auto getPlayerController() -> class APlayerController* {
        APlayerController* controller = this->getLocalPlayer()->Actor;
        return controller;
    }

    auto getHUD() -> AGFxHUD_TA* {
        return this->get<AGFxHUD_TA>();
    }

    auto getDataStore() -> UGFxDataStore_X* {
        return this->get<UGFxDataStore_X>();
    }

    auto getOnlinePlayer() -> UOnlinePlayer_X* {
        return this->get<UOnlinePlayer_X>();
    }

    auto checkNotInName(UObject* obj, const std::string& str) -> bool {
        return true;
    }

    template<typename T>
    bool isValidAndA(UObject* obj) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
        return obj && addr > 0x10000 && addr < 0xFFFFFFFFFF && obj->Class && obj->IsA(T::StaticClass());
    }

    template<typename T>
    auto getLiveInstanceOf() -> T* {
        static_assert(std::is_base_of_v<UObject, T>, "T must be a UObject-derived type");

        log_->debug("🔎 getLiveInstanceOf<>() called", LogCategory::CORE);

        auto* objects = UObject::GObjObjects();
        if (!objects) {
            log_->error("❌ GObjObjects() returned nullptr", LogCategory::CORE);
            return nullptr;
        }

        int32_t numObjects = objects->Num();
        if (numObjects <= UOBJECTS_ITERATE_OFFSET) {
            log_->warn("⚠️ Not enough objects to iterate", LogCategory::CORE);
            return nullptr;
        }

        for (int32_t i = numObjects - 1; i >= UOBJECTS_ITERATE_OFFSET; --i) {
            UObject* obj = objects->At(i);
            if (!obj)
                continue;

            //            uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
            //            if (addr < 0x10000 || addr > 0xFFFFFFFFFF)
            //                continue; // skip obviously bad memory

            // fixme
            // skip if is going to be kill
            // rn flags don't work i think
            // if (obj->HasAnyFlags(RF_PendingKill))
            //    continue;

            if (!obj->Class)
                continue; // skip if no valid class metadata

            if (!obj->IsA(T::StaticClass()))
                continue; // not our type

            if (obj->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject | RF_Transient))
                continue;

            if (obj->IsPendingKill())
                continue;

            T* candidate = static_cast<T*>(obj);

            // 👇 Special case if you're looking for UGFxData_Chat_TA
            if constexpr (std::is_same_v<T, UGFxData_Chat_TA>) {
                if (!candidate->Shell || !candidate->Shell->DataStore)
                    continue; // not fully initialized, skip
            }

            log_->debug("🎯 Found live instance: " + obj->GetFullName(), LogCategory::CORE);
            return candidate;
        }

        log_->warn("❌ No valid live instance found", LogCategory::CORE);
        return nullptr;
    }

  private:
    std::unordered_map<std::type_index, void*> instanceCache_;
    std::map<std::string, class UClass*> m_staticClasses;
    std::unordered_map<std::string, UFunction*> staticFunctions_;
    std::vector<class UObject*> createdObjects_;

    template<typename T>
    auto GetTypeName() -> std::string;
};

#define FIND_LIVE_INSTANCE_SAFE(ClassName)                                 \
    ([]() {                                                                \
        auto* ptr = getLiveInstanceOf<ClassName>();                        \
        if (!ptr)                                                          \
            log_->error("FIND_LIVE_INSTANCE_SAFE failed for " #ClassName); \
        return ptr;                                                        \
    })()
