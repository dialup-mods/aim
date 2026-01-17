//#pragma once
//#include <filesystem>
//#include "AIM.h"
//#include "SafeLogger.h"
//#include "AsyncGate.h"
//#include "DependencyContainer.h"
//#include "Dispatch.h"
//#include "ModuleLoader.h"
//#include "PatchManager.h"
//#include "TaskQueue.h"
//#include "ConsoleInterpreter.h"
//#include "ObjectProvider.h"
//#include "EngineValidator.h"
//
//#include "ProcessEvent.h"
//#include "ProcessInternal.h"
//#include "CallFunction.h"
//#include "ChatboxConsole.h"
//#include "MutexGuard.h"
//
//// clang-format off
//
//namespace bootstrap::aim {
//
//inline std::string modal =
//"\n"
//"<==[--==[--==[  A I M  ]==--]==--]==>\n"
//"\n"
//"     ADVANCED INJECTION MODULE v1.0\n"
//"\n"
//"       Developed by: FREE_AOL\n"
//"       Part of the Dial-Up Framework™\n"
//"\n"
//"     ░█▀▄░█▀▀░█▀▄░█▄█░█▀▀░░█▀▀░█░█\n"
//"     ░█▀▄░█▀▀░█░█░█░█░▀▀█░░█▀▀░░█░\n"
//"     ░▀▀░░▀▀▀░▀▀░░▀░▀░▀▀▀░░▀▀▀░░▀░\n"
//"\n"
//"  <==[--------------------------------]==>\n"
//"\n"
//"  🔥 looks like malware, runs like middleware 🔥\n"
//"\n";
//
//// EngineValidator and ObjectProvider
//inline bool
//unreal(DependencyContainer& frameworkContainer, DependencyInjector& frameworkInjector, std::shared_ptr<SafeLogger> log) {
//    log->debug("Loading EngineValidator...");
//
//    auto builder = std::make_unique<ModuleBuilder>();
//
//    builder->registerModule(
//        ModuleDefinition<EngineValidator>()
//        .withDependency(&EngineValidator::__inject_log, "[default]")
//        .asSingleton()
//    );
//
//    auto engineValidator = AIM::getContainer()->resolve<EngineValidator>("[default]");
//    if (!engineValidator) {
//        log->error("Could not resolve EngineValidator.");
//        return false;
//    }
//
//    if (!engineValidator->init()) {
//        log->warn("EngineValidator init() failed.");
//        return false;
//    }
//    log->debug("EngineValidator initialized.");
//
//    log->debug("Loading ObjectProvider...");
//
//    builder->registerModule(ModuleDefinition<AsyncGate>().named("ObjectProviderReady").asSingleton());
//
//    builder->registerModule(ModuleDefinition<ObjectProvider>()
//            .withDependency(&ObjectProvider::__inject_log, "[default]")
//            .withDependency(&ObjectProvider::__inject_engineValidator, "[default]")
//            .withDependency(&ObjectProvider::__inject_asyncGate, "ObjectProviderReady")
//            .asSingleton());
//
//    auto objectProvider = AIM::getContainer()->resolve<ObjectProvider>("[default]");
//
//    if (!objectProvider) {
//        log->error("Could not resolve ObjectProvider");
//        return false;
//    }
//
//    objectProvider->init();
//
//    return true;
//}
//
//template <typename T>
//static bool registerFunctionModuleDependencies(DependencyContainer& frameworkContainer, DependencyInjector& frameworkInjector, const std::string& prefix) {
//
//    auto log = AIM::getContainer()->resolve<SafeLogger>("[default]");
//    if (!log) {
//        return false;
//    }
//    log->debug("Loading AIM function...");
//
////    auto stateObj = container.resolve<PluginState>("[default]");
////    if (!stateObj) {
////        log->debug("Could not resolve PluginState...");
////        return false;
////    }
//
////    stateObj->setStatus(PluginRunStatus::Initializing);
//
//    std::string taskQueueName     = prefix + "-TaskQueue";
//    std::string dispatchName      = prefix + "-Dispatch";
//    std::string patchManagerName  = prefix + "-PatchManager";
//
//    log->debug("Loading " + taskQueueName + "...");
//    auto builder = std::make_unique<ModuleBuilder>();
//
//    builder->registerModule(
//        ModuleDefinition<AsyncGate>()
//            .named(prefix + "-AsyncGate")
//            .asSingleton()
//    );
//
//    builder->registerModule(
//        ModuleDefinition<AsyncGate>()
//            .named(prefix + "-TaskQueueReady")
//            .asSingleton()
//    );
//
//    builder->registerModule(
//        ModuleDefinition<AsyncGate>()
//            .named(prefix + "-TaskQueueShutdown")
//            .asSingleton()
//    );
//
//    builder->registerModule(
//        ModuleDefinition<AsyncGate>()
//            .named(prefix + "-DispatchReady")
//            .asSingleton()
//    );
//
//    builder->registerModule(
//        ModuleDefinition<AsyncGate>()
//            .named(prefix + "-DispatchShutdown")
//            .asSingleton()
//    );
//
//    builder->registerModule(
//        ModuleDefinition<MutexGuard>()
//            .named(prefix + "-Mutex")
//            .asSingleton()
//    );
//
//    builder->registerModule(
//        ModuleDefinition<TaskQueue>()
//            .named(taskQueueName)
//            .withDependency(&TaskQueue::__inject_log, "[default]")
//            .asSingleton()
//    );
//
//    if (!AIM::getContainer()->resolve<TaskQueue>(taskQueueName)) {
//        log->error("Could not resolve " + taskQueueName);
////        stateObj->setStatus(PluginRunStatus::Failed);
//        return false;
//    }
//
//    log->debug("Loading " + dispatchName + "...");
//    builder->registerModule(
//        ModuleDefinition<Dispatch>()
//            .named(dispatchName)
//            .withDependency(&Dispatch::__inject_log, "[default]")
//            .withDependency(&Dispatch::__inject_objectProvider, "[default]")
//            .withDependency(&Dispatch::__inject_taskQueue, taskQueueName)
//            .asSingleton()
//    );
//
//    if (!AIM::getContainer()->resolve<Dispatch>(dispatchName)) {
//        log->error("Could not resolve " + dispatchName);
////        stateObj->setStatus(PluginRunStatus::Failed);
//        return false;
//    }
//
//    log->debug("Loading " + patchManagerName + "...");
//
//    builder->registerModule(
//        ModuleDefinition<PatchManager>()
//            .named(patchManagerName)
//            .asSingleton()
//    );
//
//    if (!AIM::getContainer()->resolve<PatchManager>(patchManagerName)) {
//        log->error("Could not resolve " + patchManagerName);
////        stateObj->setStatus(PluginRunStatus::Failed);
//        return false;
//    }
//
//    return true;
//}
//
//static bool loadDetourModules(DependencyContainer& container, DependencyInjector& injector, std::shared_ptr<SafeLogger> log) {
//    log->debug(modal);
//
//    bool ok = true;
//    ok &= bootstrap::aim::registerFunctionModuleDependencies<ProcessEvent>(container, injector, "PE");
//    ok &= bootstrap::aim::registerFunctionModuleDependencies<ProcessInternal>(container, injector, "PI");
//    ok &= bootstrap::aim::registerFunctionModuleDependencies<CallFunction>(container, injector, "CF");
//
//    if (!ok) {
//        log->error("Failed to load AIM module dependencies.");
//        return false;
//    }
//
//    auto builder = std::make_unique<ModuleBuilder>();
//    log->debug("Loading AIM-PE");
//    builder->registerModule(
//        ModuleDefinition<ProcessEvent>()
//            .withDependency(&ProcessEvent::__inject_log, "[default]")
//            .withDependency(&ProcessEvent::__inject_state, "[default]")
//            .withDependency(&ProcessEvent::__inject_taskQueue, "PE-TaskQueue")
//            .withDependency(&ProcessEvent::__inject_dispatch, "PE-Dispatch")
//            .withDependency(&ProcessEvent::__inject_patchManager, "PE-PatchManager")
//            .withDependency(&ProcessEvent::__inject_mutex, "PE-Mutex")
//            .withDependency(&ProcessEvent::__inject_asyncGate, "PE-AsyncGate")
//            .withDependency(&ProcessEvent::__inject_gameWrapperProvider, "[default]")
//            .asSingleton()
//    );
//
//    log->debug("Loading AIM-PI");
//    builder->registerModule(
//        ModuleDefinition<ProcessInternal>()
//            .withDependency(&ProcessInternal::__inject_log, "[default]")
//            .withDependency(&ProcessInternal::__inject_state, "[default]")
//            .withDependency(&ProcessInternal::__inject_taskQueue, "PI-TaskQueue")
//            .withDependency(&ProcessInternal::__inject_dispatch, "PI-Dispatch")
//            .withDependency(&ProcessInternal::__inject_patchManager, "PI-PatchManager")
//            .withDependency(&ProcessInternal::__inject_mutex, "PI-Mutex")
//            .withDependency(&ProcessInternal::__inject_asyncGate, "PI-AsyncGate")
//            .withDependency(&ProcessInternal::__inject_processEvent, "[default]")
//            .asSingleton()
//    );
//
//    log->debug("Loading AIM-CF");
//    builder->registerModule(
//        ModuleDefinition<CallFunction>()
//            .withDependency(&CallFunction::__inject_log, "[default]")
//            .withDependency(&CallFunction::__inject_state, "[default]")
//            .withDependency(&CallFunction::__inject_dispatch, "CF-Dispatch")
//            .withDependency(&CallFunction::__inject_taskQueue, "CF-TaskQueue")
//            .withDependency(&CallFunction::__inject_patchManager, "CF-PatchManager")
//            .withDependency(&CallFunction::__inject_mutex, "CF-Mutex")
//            .withDependency(&CallFunction::__inject_asyncGate, "CF-AsyncGate")
//            .withDependency(&CallFunction::__inject_processEvent, "[default]")
//            .asSingleton()
//    );
//
//    auto pe = container.resolve<ProcessEvent>();
//    auto pi = container.resolve<ProcessInternal>();
//    auto cf = container.resolve<CallFunction>();
//    if (!pe) { return false; }
//    if (!pi) { return false; }
//    if (!cf) { return false; }
//
//    pe->init();
//
//    pe->getAsyncGate()->onReady([log, pi] {
//        log->debug("ProcessEvent is detoured, loading PI...");
//        pi->init();
//    });
//
//    pe->getAsyncGate()->onReady([log,cf] {
//        log->debug("ProcessEvent is detoured, loading CF...");
//        cf->init();
//    });
//
//    if (auto stateObj = container.resolve<PluginState>("[default]")) {
////        stateObj->setStatus(Running);
//        return true;
//    }
//
//    return false;
//}
//
//inline void
//init(DependencyContainer& frameworkContainer, DependencyInjector& frameworkInjector, std::shared_ptr<SafeLogger> log) {
//    std::shared_ptr<IAIM> aim = std::make_shared<AIM>();
//    AIM::getContainer()->registerInstance<IAIM>(aim);
//
//    bootstrap::aim::unreal(AIMContainer_, injector, log);
//
//    AIM::getContainer()->resolve<AsyncGate>("ObjectProviderReady")->onReady([&container, &injector, log] {
//        log->debug("Object Provider ready, continuing...");
//        bootstrap::aim::loadDetourModules(container, injector, log);
//    });
//
//    auto builder = std::make_unique<ModuleBuilder>();
//    builder->registerModule(
//        ModuleDefinition<AsyncGate>()
//            .named("AIM-ready-gate")
//            .asSingleton()
//    );
//    auto aimReadyGate = AIM::getContainer()->resolve<AsyncGate>("AIM-ready-gate");
//
//    auto gates = { AIM::getContainer()->resolve<ProcessEvent>()->getAsyncGate(),
//        // broken? fixme
//        // container_->resolve<ProcessInternal>()->getAsyncGate(),
//        AIM::getContainer()->resolve<CallFunction>()->getAsyncGate()
//    };
//
//    AsyncGate::onAllReady(gates, [aimReadyGate, log] {
//        log->debug("Processes are detoured, continuing...");
//        aimReadyGate->setReady();
//    });
//}
//
//
//}
//
//// clang-format on