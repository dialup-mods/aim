#pragma once
#include "DependencyContainer.h"
#include "IPluginContext.h"
#include "main.h"

#include <SafeLogger.h>
#include <memory>

struct PluginContext : IPluginContext {
    DependencyContainer* frameworkContainer_;
    DependencyContainer* pluginsContainer_;
    std::unique_ptr<DependencyContainer> pluginContainer_;

    PluginContext()
      : frameworkContainer_(DialUp::getContainer().get())
      , pluginsContainer_(DialUp::getPluginContainer().get())
      , pluginContainer_(std::make_unique<DependencyContainer>()) {}

    [[nodiscard]] auto getFrameworkContainer() const -> DependencyContainer& override { return *frameworkContainer_; }
    [[nodiscard]] auto getPluginsContainer() const -> DependencyContainer& override { return *pluginsContainer_; }
    [[nodiscard]] auto getContainer() const -> DependencyContainer& override { return *pluginContainer_; }
};