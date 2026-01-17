#pragma once
#include "SafeLogger.h"
class DependencyContainer;

#include "IPluginContext.h"

struct IPlugin {
    virtual ~IPlugin() = default;
    // virtual void onLoad(IPluginContext* context) = 0;
    virtual void onLoad() = 0;
    virtual void onUnload() = 0;
    [[nodiscard]] virtual auto getName() const -> const char* = 0;
    //    virtual void registerServices(DependencyContainer& container) = 0;
    //    virtual void onPostRegister(DependencyContainer& container) = 0; // For resolving others

    virtual void setContext(IPluginContext* ctx) = 0;
    virtual auto getContext() const -> IPluginContext* = 0;
};
