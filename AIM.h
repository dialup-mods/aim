#pragma once

#include "AsyncGate.h"
#include "IModule.h"
#include "IPlugin.h"
#include "PluginBase.h"
#include "SDK.h"

class AIM : public PluginBase<AIM> {
    AIM_INJECTABLE(AIM)

    ~AIM() override = default;
    auto getName() const -> const char* override;
    void startup() override;
    auto registerPublicInterfaces() const -> std::vector<PublicInterface> override;
    void shutdown() override;

    template<class T>
    static auto classOf(UObject*) -> UClass *;

    template<class T>
    static auto getInstanceOf() -> T *;

    void testToast();

    auto modal() -> const char*;

    bool createPopulateRuntime();

    bool loadObjectProvider();

    template<class T>
    bool registerFunctionModuleDependencies(const std::string& prefix);
    bool loadDetourModules();

    template<class T>
    static void removeDetour(std::shared_ptr<T> fn, std::shared_ptr<AsyncGate> gate, std::shared_ptr<ILogger> log);

    // fixme probably not needed anymore, handled through PluginContext
    static Resolver* getStaticResolver() { return staticResolver_; }
    static void setStaticResolver(Resolver* resolver) { staticResolver_ = resolver; }
    static Resolver* staticResolver_;

private:
    std::string modal_;
};