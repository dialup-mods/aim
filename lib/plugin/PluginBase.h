#pragma once
#include "IModule.h"
#include "IPlugin.h"
#include "IPluginContext.h"

template<typename T>
class PluginBase
  : public IPlugin
  , public IModule {
  public:
    void setContext(IPluginContext* ctx) override { context_ = ctx; }

    auto getContext() const -> IPluginContext* override { return context_; }

    void yeet() override {}

    static void* defaultCreate(uintptr_t handle, void* context) {
        auto ctx = static_cast<IPluginContext*>(context);
        auto plugin = std::make_shared<T>();
        ctx->getPluginsContainer().registerInstance<T>(plugin);

        auto& meta = ctx->getPluginsContainer().getFactoryEntry<T>();
        meta.setMetadata("handle", std::to_string(handle));
        meta.setMetadata("name", plugin->getName());

        return ctx->resolveFramework<T>().get();
    }

    static void defaultDestroy() {}

    IPluginContext* context_ = nullptr;
};