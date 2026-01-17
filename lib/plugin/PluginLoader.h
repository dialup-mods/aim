#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

#include "Annotations.h"
#include "Exception.h"
#include "IModule.h"
#include "IPlugin.h"
#include "PluginContext.h"
#include "SafeLogger.h"
#include "main.h"

class PluginLoader : IModule {
    AIM_INJECTABLE(PluginLoader)

  public:
    PluginLoader() = default;

    auto callCreate(HMODULE lib, IPluginContext* context) -> void*;
    HMODULE loadLib(const char* path);
    void callInit(HMODULE lib, PluginContext* context);
    auto load(const std::string& path) -> bool;
    void safeOnLoad(IPlugin* plugin);
    bool unloadImpl(HMODULE lib);
    auto unload(const std::string& path) -> bool;
    void unloadAll();

    void*
    callCreate(HMODULE lib, IPluginContext* context) {
        void* pluginPtr = nullptr;
        __try {
            auto createFunc = (void* (*)(uintptr_t, void*))GetProcAddress(lib, "create"); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
            pluginPtr = createFunc(reinterpret_cast<uintptr_t>(lib), context);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            MessageBoxA(nullptr, "plugin failed create", "Test Plugin", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        }

        return pluginPtr;
    }

    HMODULE
    loadLib(const char* path) {
        HMODULE lib = nullptr;
        __try {
            lib = LoadLibraryA(path);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            MessageBoxA(nullptr, "failed to load plugin", "Test Plugin", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        }
        return lib;
    }

    auto
    load(const std::string& path) -> bool {
        auto context = std::make_unique<PluginContext>();
        auto log = context->frameworkContainer_->resolve<SafeLogger>();

        const char* pathCStr = path.c_str();
        HMODULE lib = loadLib(pathCStr);
        if (lib == nullptr) {
            MessageBoxA(nullptr, "lib is null", "Test Plugin", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return false;
        }

        // add it early so that if it fails loading you can still unload the .dll
        plugins_[path] = PluginRecord{ .handle = lib, .context = std::move(context) };

        auto plugin = static_cast<IPlugin*>(callCreate(lib, plugins_[path].context.get()));

        plugin->setContext(plugins_[path].context.get());
        plugin->onLoad();
        //    safeOnLoad(plugin.get());

        // plugins_[path].instance = std::move(plugin);

        return true;
    }

    void
    safeOnLoad(IPlugin* plugin) {
        std::function fn = [plugin] { plugin->onLoad(); };
        safe::callWithSEH(&fn);
    }

    bool
    unloadImpl(HMODULE handle) {
        __try {
            auto destroyFunc = (void (*)())GetProcAddress(handle, "destroy"); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
            if (destroyFunc != nullptr) {
                destroyFunc();
            }
            FreeLibrary(handle);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            MessageBoxA(nullptr, "plugin failed destroy", "Test Plugin", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            return false;
        }
    }

    bool
    unload(const std::string& path) {
        auto it = plugins_.find(path);
        if (it == plugins_.end()) {
            return false;
        }

        if (unloadImpl(it->second.handle)) {
            plugins_.erase(it);
            return true;
        }
        return false;
    }

    void
    unloadAll() {
        for (auto& [path, record] : plugins_) {
            __try {
                auto destroyFunc = (void (*)())GetProcAddress(record.handle, "destroy"); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
                if (destroyFunc != nullptr) {
                    destroyFunc();
                }
                FreeLibrary(record.handle);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                MessageBoxA(nullptr, "plugin failed destroy", "Test Plugin", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            }
        }
        plugins_.clear();
    }

  private:
    struct PluginRecord {
        HMODULE handle;
        std::unique_ptr<IPlugin> instance;
        std::unique_ptr<PluginContext> context;
    };

    std::unordered_map<std::string, PluginRecord> plugins_;
};