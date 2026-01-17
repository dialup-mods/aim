#include <string>
#include "SDK.h"

#include "Runtime.h"
#include "AsyncGate.h"
#include "GameTypes.h"
#include "ILogger.h"
#include "ObjectProvider.h"

ObjectProvider::~ObjectProvider() {
    log_->debug("ObjectProvider unloading...");
    //    for (UObject* uObject : m_createdObjects) {
    //        if (uObject) {
    //            MarkForDestroy(uObject);
    //        }
    //    }
    //    m_createdObjects.clear();
    log_->debug("ObjectProvider unloaded");
}

void
ObjectProvider::init() {}

auto
ObjectProvider::getFullName(UObject* obj) -> std::string {
    if (obj) {
        return obj->GetFullName();
    }
    return "None";
}

auto
ObjectProvider::findStaticClass(const std::string& className) -> class UClass* {
    // fixme lock the weak_ptr at the beginning of the method
    auto classCache = Runtime::getClassCache();
    if (classCache.empty()) {
        std::map<std::string, UClass*> tempClassCache;

        auto& objects = Runtime::getUObjects();
        const size_t limit = std::min(iterateLimit_, objects.size());

        for (size_t i = 0; i < limit; --i) {
            if (UObject* uObject = objects.at(i)) {
                if (uObject->GetFullName().starts_with("Class")) {
                    tempClassCache[uObject->GetFullName()] = static_cast<UClass*>(uObject);
                }
            }
        }
        Runtime::setClassCache(tempClassCache);
    }

    if (classCache.contains(className)) {
        return classCache[className];
    }

    return nullptr;
}

UFunction*
ObjectProvider::findStaticFunction(const std::string& fullName) {
    auto functionCache = Runtime::getFunctionCache();
    auto it = functionCache.find(fullName);
    if (it != functionCache.end()) {
        return it->second;
    }

    auto& objects = Runtime::getUObjects();
    const size_t limit = std::min(iterateLimit_, objects.size());

    for (size_t i = 0; i < limit; --i) {
        if (UObject* uObject = objects.at(i)) {
            if (uObject->IsA(UFunction::StaticClass())) {
                std::string objName = uObject->GetFullName();

                if (objName == fullName) {
                    auto* uFunctionObj = static_cast<UFunction*>(uObject);
                    Runtime::addToFunctionCache(objName, uFunctionObj);
                    return uFunctionObj;
                }
            }
        }
    }

    return nullptr;
}

auto
ObjectProvider::getEngine() -> class UEngine* {
    UClass* engineClass = findStaticClass("Class Engine.Engine");
    if (!engineClass) {
        log_->error("Could not find UEngine static class");
        return nullptr;
    }

    auto& objects = Runtime::getUObjects();
    const size_t limit = std::min(iterateLimit_, objects.size());

    for (size_t i = 0; i < limit; --i) {
        UObject* obj = objects.at(i);
        if (!obj) { continue; }

        if (obj->IsA(engineClass)) {
            std::string name = obj->GetName();
            if (name.find("Default__") == 0) { continue; } // skip the CDO
            return static_cast<UEngine*>(obj);
        }
    }

    log_->error("No live UEngine instance found in scan");
    return nullptr;
}

UGameViewportClient*
ObjectProvider::getViewportClient() {
    UEngine* engine = this->getEngine();
    if (!engine || !engine->GameViewport) {
        log_->error("Could not get GameViewport");
        return nullptr;
    }
    return engine->GameViewport;
}

auto
ObjectProvider::getAudioDevice() -> class UAudioDevice* {
    return UEngine::GetAudioDevice();
}

auto
ObjectProvider::getWorldInfo() -> class AWorldInfo* {
    return UEngine::GetCurrentWorldInfo();
}

auto
ObjectProvider::getFileSystem() -> class UFileSystem* {
    return (UFileSystem*)UFileSystem::StaticClass();
}

auto
ObjectProvider::getLocalPlayer() -> class ULocalPlayer* {
    if (UEngine* engine = this->getEngine(); engine && engine->GamePlayers[0]) {
        return engine->GamePlayers[0];
    }
    return nullptr;
}

//auto
//ObjectProvider::getLocalPlayerTA() -> class ULocalPlayer_TA* {
//    if (UEngine* engine = this->getEngine(); engine && !engine->GamePlayers.empty() && engine->GamePlayers[0]) {
//        ULocalPlayer* player = engine->GamePlayers[0];
//        if (player && player->IsA(ULocalPlayer_TA::StaticClass())) {
//            return reinterpret_cast<ULocalPlayer_TA*>(player);
//        }
//    }
//    return nullptr;
//}
//
//auto
//ObjectProvider::getLocalPlayerX() -> class ULocalPlayer_X* {
//    if (UEngine* engine = this->getEngine(); engine && !engine->GamePlayers.empty() && engine->GamePlayers[0]) {
//        ULocalPlayer* player = engine->GamePlayers[0];
//        if (player && player->IsA(ULocalPlayer_TA::StaticClass())) {
//            return reinterpret_cast<ULocalPlayer_TA*>(player);
//        }
//    }
//    return nullptr;
//}
//
//auto
//ObjectProvider::getPlayerController() -> class APlayerController* {
//    APlayerController* controller = this->getLocalPlayer()->Actor;
//    return controller;
//}

// auto
// ObjectProvider::getUniqueID() -> struct FUniqueNetId {
//     if (ULocalPlayer* localPlayer = this->getLocalPlayer()) {
//         return localPlayer->eventGetUniqueNetId();
//     }
//     return FUniqueNetId{};
// }

auto
ObjectProvider::getHUD() -> AGFxHUD_TA* {
    return this->get<AGFxHUD_TA>();
}

auto
ObjectProvider::getDataStore() -> UGFxDataStore_X* {
    return this->get<UGFxDataStore_X>();
}

auto
ObjectProvider::getOnlinePlayer() -> UOnlinePlayer_X* {
    return this->get<UOnlinePlayer_X>();
}

// fixme
auto
ObjectProvider::checkNotInName(UObject* obj, const std::string& str) -> bool {
    return true;
}

// void
// ObjectProvider::scanForProcessEvent() {
//     for (int32_t i = 0; i < allObjects->size(); i++) {
//         UObject* uObject = allObjects->at(i);
//         if (!uObject || !uObject->GetClass()) continue;
//
//         const char* name = uObject->GetName();
//         if (name && strstr(name, "ProcessEvent")) {
//             log_->debug("Candidate: " + std::string(name));
//         }
//     }
// }

// void
// ObjectProvider::cmd_list_playlist_info(std::vector<std::string> args) {
//     UOnlineGamePlaylists_X* playlists = Instances.getInstanceOf<UOnlineGamePlaylists_X>();
//     if (!playlists)
//         return;
//
//     LOG("DownloadedPlaylists size: {}", playlists->DownloadedPlaylists.size());
//
//     LOG("--------------------------------------");
//     LOG("ID --> Internal name --> Display name");
//     LOG("--------------------------------------");
//     for (const auto& p : playlists->DownloadedPlaylists) {
//         if (!p)
//             continue;
//
//         LOG("{} --> {} --> {}", p->PlaylistId, playlists->IdToName(p->PlaylistId).ToString(), p->getLocalizedName().ToString());
//     }
// }
