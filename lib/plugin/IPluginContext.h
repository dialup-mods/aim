#pragma once

#include "DependencyContainer.h"

struct IPluginContext {
    virtual ~IPluginContext() = default;

    [[nodiscard]] virtual auto getContainer() const -> DependencyContainer& = 0;
    [[nodiscard]] virtual auto getFrameworkContainer() const -> DependencyContainer& = 0;
    [[nodiscard]] virtual auto getPluginsContainer() const -> DependencyContainer& = 0;

    template<typename T>
    auto resolve() const -> std::shared_ptr<T> {
        return getContainer().resolve<T>();
    }

    template<typename T>
    auto resolveFramework() const -> std::shared_ptr<T> {
        return getFrameworkContainer().resolve<T>();
    }
};