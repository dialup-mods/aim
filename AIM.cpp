#include "AIM.h"
#include "IModule.h"
#include "IRuntime.h"

#include "PluginBase.h"
//#include "GameWrapperProvider.h"
#include "AsyncGate.h"
#include "Dispatch.h"
#include "ILogger.h"
#include "Detour.h"

#include "TaskBuilder.h"
#include "v1/ITaskBuilder.h"

#include "ModuleLoader.h"
#include "TaskQueue.h"
// #include "ConsoleInterpreter.h"
#include "EngineLocator.h"
#include "ObjectProvider.h"
#include "PluginFence.h"

#include "CallFunction.h"
//#include "ChatboxConsole.h"
#include "MutexGuard.h"
#include "ProcessEvent.h"
#include "ProcessInternal.h"

#include "Resolver.h"

#include "ICallFunction.h"
#include "IProcessEvent.h"
#include "IProcessInternal.h"

Resolver* AIM::staticResolver_ = nullptr;

auto
AIM::modal() -> const char* {
    return
    "\n"
    "<==[--==[--==[  A I M  ]==--]==--]==>\n"
    "\n"
    "     ADVANCED INJECTION MODULE v3.0\n"
    "\n"
    "       Developed by: FREE AOL\n"
    "       Part of the Dial-Up Framework™\n"
    "\n"
    "     ░█▀▄░█▀▀░█▀▄░█▄█░█▀▀░░█▀▀░█░█\n"
    "     ░█▀▄░█▀▀░█░█░█░█░▀▀█░░█▀▀░░█░\n"
    "     ░▀▀░░▀▀▀░▀▀░░▀░▀░▀▀▀░░▀▀▀░░▀░\n"
    "\n"
    "  <==[--------------------------------]==>\n"
    "\n"
    "  🔥 looks like malware, runs like middleware 🔥\n"
    "\n";
}

bool AIM::createPopulateRuntime() {
    auto log = getLogger();
    log->debug("Loading Runtime...");

    registerModule(
    ModuleDefinition<EngineLocator>()
        .withDependency(&EngineLocator::__inject_log, "[default]")
        .asSingleton()
    );

    auto engineLocator = resolve<EngineLocator>();
    if (!engineLocator) {
        log->error("Unable to resolve EngineLocator.");
        return false;
    }

    registerModule(
    ModuleDefinition<IRuntime>()
        .withDependency(&IRuntime::__inject_log, "[default]")
        .withDependency(&IRuntime::__inject_engineLocator, "[default]")
        .asSingleton()
    );

    auto runtime = resolve<IRuntime>();
    if (!runtime) {
        log->error("Unable to resolve Runtime.");
        return false;
    }

    if (!runtime->init()) {
        log->error("Runtime failed during initialization.");
        return false;
    }

    getLogger()->debug("Loading ObjectProvider...");

    registerModule(ModuleDefinition<AsyncGate>().named("ObjectProviderReady").asSingleton());

    registerModule(ModuleDefinition<ObjectProvider>()
            .withDependency(&ObjectProvider::__inject_log, "[default]")
            .withDependency(&ObjectProvider::__inject_asyncGate, "ObjectProviderReady")
            .withDependency(&ObjectProvider::__inject_runtime, "ObjectProviderReady")
            .asSingleton());

    auto objectProvider = resolve<ObjectProvider>("[default]");
    if (!objectProvider) {
        getLogger()->error("Could not resolve ObjectProvider");
        return false;
    }

    getLogger()->debug("Unreal dependencies initialized.");
    resolve<AsyncGate>("unreal-ready")->setReady();

    return true;
}

template <typename T>
bool AIM::registerFunctionModuleDependencies(const std::string& prefix) {
    getLogger()->debug("Loading AIM function...");

//    auto stateObj = container.resolve<PluginState>("[default]");
//    if (!stateObj) {
//        getLogger()->debug("Could not resolve PluginState...");
//        return false;
//    }

//    stateObj->setStatus(PluginRunStatus::Initializing);

    registerModule(
        ModuleDefinition<AsyncGate>()
            .named(prefix + "-AsyncGate")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<AsyncGate>()
            .named(prefix + "-TaskQueueReady")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<AsyncGate>()
            .named(prefix + "-TaskQueueShutdown")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<AsyncGate>()
            .named(prefix + "-DispatchReady")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<AsyncGate>()
            .named(prefix + "-DispatchShutdown")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<MutexGuard>()
            .named(prefix + "-Mutex")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<TaskQueue>()
            .named(prefix + "-TaskQueue")
            .withDependency(&TaskQueue::__inject_log, "[default]")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<Dispatch>()
            .named(prefix + "-Dispatch")
            .withDependency(&Dispatch::__inject_log, "[default]")
            .withDependency(&Dispatch::__inject_objectProvider, "[default]")
            .withDependency(&Dispatch::__inject_taskQueue, prefix + "-TaskQueue")
            .asSingleton()
    );

    registerModule(
        ModuleDefinition<Detour>()
            .named(prefix + "-Detour")
            .withDependency(&Detour::__inject_log, "[default]")
            .asSingleton()
    );

    return true;
}

bool
AIM::loadDetourModules() {
    getLogger()->log("{}", modal());

    bool ok = true;
    ok &= registerFunctionModuleDependencies<ProcessEvent>("PE");
    ok &= registerFunctionModuleDependencies<ProcessInternal>("PI");
    ok &= registerFunctionModuleDependencies<CallFunction>("CF");

    if (!ok) {
        getLogger()->error("Failed to load AIM module dependencies.");
        return false;
    }

    getLogger()->debug("Loading AIM-PE");
    registerModule(
        ModuleDefinition<ProcessEvent>()
            .withDependency(&ProcessEvent::__inject_log, "[default]")
            .withDependency(&ProcessEvent::__inject_taskQueue, "PE-TaskQueue")
            .withDependency(&ProcessEvent::__inject_dispatch, "PE-Dispatch")
            .withDependency(&ProcessEvent::__inject_mutex, "PE-Mutex")
            .withDependency(&ProcessEvent::__inject_appliedGate, "PE-AsyncGate")
            .withDependency(&ProcessEvent::__inject_removedGate, "PE-AsyncGate-Remove")
            .withDependency(&ProcessEvent::__inject_readyFence, "ReadyFence") // shared fence for all processes
            .withDependency(&ProcessEvent::__inject_teardownFence, "ReadyDestroyFence") // shared fence for all processes
            .withDependency(&ProcessEvent::__inject_detour, "PE-Detour")
            .asSingleton()
    );

    getLogger()->debug("Loading AIM-PI");
    registerModule(
        ModuleDefinition<ProcessInternal>()
            .withDependency(&ProcessInternal::__inject_log, "[default]")
            .withDependency(&ProcessInternal::__inject_taskQueue, "PI-TaskQueue")
            .withDependency(&ProcessInternal::__inject_dispatch, "PI-Dispatch")
            .withDependency(&ProcessInternal::__inject_mutex, "PI-Mutex")
            .withDependency(&ProcessInternal::__inject_appliedGate, "PI-AsyncGate")
            .withDependency(&ProcessInternal::__inject_removedGate, "PI-AsyncGate-Remove")
            .withDependency(&ProcessInternal::__inject_processEvent, "[default]")
            .withDependency(&ProcessInternal::__inject_readyFence, "ReadyFence") // shared fence for all processes
            .withDependency(&ProcessInternal::__inject_teardownFence, "ReadyDestroyFence") // shared fence for all processes
            .withDependency(&ProcessInternal::__inject_detour, "PI-Detour")
            .asSingleton()
    );

    getLogger()->debug("Loading AIM-CF");
    registerModule(
        ModuleDefinition<CallFunction>()
            .withDependency(&CallFunction::__inject_log, "[default]")
            .withDependency(&CallFunction::__inject_dispatch, "CF-Dispatch")
            .withDependency(&CallFunction::__inject_taskQueue, "CF-TaskQueue")
            .withDependency(&CallFunction::__inject_mutex, "CF-Mutex")
            .withDependency(&CallFunction::__inject_appliedGate, "CF-AsyncGate")
            .withDependency(&CallFunction::__inject_removedGate, "CF-AsyncGate-Remove")
            .withDependency(&CallFunction::__inject_processEvent, "[default]")
            .withDependency(&CallFunction::__inject_readyFence, "ReadyFence") // shared fence for all processes
            .withDependency(&CallFunction::__inject_teardownFence, "ReadyDestroyFence") // shared fence for all processes
            .asSingleton()
    );

    auto pe = resolve<ProcessEvent>();
    auto pi = resolve<ProcessInternal>();
    auto cf = resolve<CallFunction>();
    if (!pe || !pi || !cf) return false;

    pe->init();

    pe->onDetoured([pi, log = getLogger()] {
        log->debug("ProcessEvent is detoured, loading PI...");
        pi->init();
    });

    //registerModule(
    //ModuleDefinition<TaskBuilder>()
    //    .withFactory([](Resolver& r) {
    //        return std::make_shared<TaskBuilder>();
    //    })
    //    .asTransient()
    //);

    //registerModule(
    //ModuleDefinition<ITaskBuilder>()
    //    .withFactory([](Resolver& r) {
    //        return std::make_shared<ITaskBuilder>();
    //    })
    //    .asTransient()
    //);

    //auto taskBuilder = resolve<TaskBuilder>();
    //pe->onDetoured([this, log = getLogger(), cf, pe] {
    //    //cf->init();
    //});

//    auto processEventInterface = std::make_shared<IProcessEvent>();
//    context_->getDialUp()->registerInstance<IProcessEvent>(processEventInterface);

//    if (auto stateObj = container.resolve<PluginState>("[default]")) {
////        stateObj->setStatus(Running);
//        return true;
//    }

    return true;
}

template<typename T>
void AIM::removeDetour(std::shared_ptr<T> fn, std::shared_ptr<AsyncGate> gate, std::shared_ptr<ILogger> log) {
    fn->removeDetour();

//    // mutex
//    std::thread([log, fn, gate] {
//        if (fn->waitForUnlock(500)) {
//            log->debug(fn->getName() + " gate ready");
//        } else {
//            log->warn(fn->getName() + " detour watchdog timed out");
//        }
//        fn->onDetoured([gate]() {
//            gate->setReady();
//        });
//    }).detach();
}

auto
AIM::getName() const -> const char* {
    return "AIM";
}

void
AIM::startup() {
    auto log = getLogger();
    if (!log) { return; }
    log->debug("AIM startup\n");

    //static_assert(sizeof(FName) == 8, "bad FName");
    //static_assert(sizeof(FString) == 0x10, "bad FString");
    //static_assert(sizeof(TArray<void*>) == 0x10, "bad TArray");
    setStaticResolver(getResolver());
    registerModule(
        ModuleDefinition<PluginFence>()
            .named("ReadyFence")
            .asSingleton()
    );
    registerModule(
        ModuleDefinition<PluginFence>()
            .named("ReadyDestroyFence")
            .asSingleton()
    );
    auto fence = resolve<PluginFence>("ReadyFence");

    //fence->block("CF");
    //fence->block("PI");

    auto unrealReadyGate = std::make_shared<AsyncGate>();
    registerInstance<AsyncGate>(unrealReadyGate, "unreal-ready");

    auto detourGate = std::make_shared<AsyncGate>();
    registerInstance<AsyncGate>(detourGate, "detour-ready");

    if (!createPopulateRuntime()) {
        return;
    }

    unrealReadyGate->onReady([this, fence, log] {
        log->debug("unreal ready, calling detours\n");
        loadDetourModules();
        fence->onReady([this] {
            printf("detours ready");
        });
        //
        std::vector<AsyncGate*> processGates;
        processGates.emplace_back(getResolver()->resolve<AsyncGate>("PE-AsyncGate").get());
        processGates.emplace_back(getResolver()->resolve<AsyncGate>("PI-AsyncGate").get());
        //processGates.emplace_back(getResolver()->resolve<AsyncGate>("CF-AsyncGate").get());

        AsyncGate::onAllReady(processGates, [this] {
            getLogger()->debug("AIM::init() processGates ready");
            getLogger()->debug("AIM gate setReady()");

            setPluginReady();
        });
    });

//    auto processEvent = resolve<ProcessEvent>("[default]");
//    auto objectProvider = resolve<ObjectProvider>("[default]");

//    std::shared_ptr<IObjectProvider> iface = resolve<ObjectProvider>();
//    registerInstance<IObjectProvider>(iface);
}

//template <typename T>
//auto AIM::classOf(UObject*) -> UClass* {
//    static UClass* cached = [] {
//        UClass* cls = r::uclass::find(T::className);
//        if (!cls) {
//            printf("can't find\n");
//        }
//        return cls;
//    }();
//    return cached;
//};
//
//template<typename T>
//auto AIM::getInstanceOf() -> T* {
//    static_assert(std::is_base_of_v<UObject, T>);
//
//    UClass* wanted = r::uobject::classOf<T>();
//    if (!wanted) return nullptr;
//
//    for (UObject* obj : r::uobject::game_pool::ref()) {
//        if (!obj) continue;
//        if (obj->ObjectFlags & RF_DefaultOrArchetypeFlags) continue;
//
//        if (obj->Class == wanted) {
//            return static_cast<T*>(obj);
//        }
//    }
//    return nullptr;
//}

void AIM::testToast() {
    auto processEvent = resolve<ProcessEvent>();
    auto objectProvider = resolve<ObjectProvider>();

    // spawn one, getinstance returns this new one
    // and it shows a notification in the friends, etc panel
    //UNotification_TA* notification;

    processEvent->registerTask(TaskBuilder()
            .name("Toast")
            .functionName("Function Engine.HUD.PostRender")
            .phase(HookPhase::Post)
            .callback([objectProvider](InvocationContext& ctx) {
                auto* mgr = objectProvider->getInstanceOf<UNotificationManager_TA>();
                auto* notificationClass = objectProvider->classOf<UGenericNotification_TA>();
                UNotification_TA* ret = mgr->PopUpOnlyNotification(notificationClass);
                ret->SetTitle(FString(L"Welcome."));
                ret->SetBody(FString(L"You've got mail!"));
            })
            .once()
            .build());
}

auto AIM::registerPublicInterfaces() const -> std::vector<PublicInterface> {
    // upcast in plugin where both types are known
    return {
        expose<IProcessEvent>(resolve<ProcessEvent>())
        , expose<ICallFunction>(resolve<CallFunction>())
        , expose<IProcessInternal>(resolve<ProcessInternal>())
        , expose<IObjectProvider>(resolve<IObjectProvider>())
        , expose<ITaskBuilder>(resolve<ITaskBuilder>())
    };
}

void
AIM::shutdown() {
    auto log = resolve<ILogger>();
    log->debug("AIM::shutdown()");

    auto teardownFence = resolve<PluginFence>("ReadyDestroyFence");

    auto pe = resolve<ProcessEvent>();
    auto pi = resolve<ProcessInternal>();
    auto cf = resolve<CallFunction>();

    auto peGate = std::make_shared<AsyncGate>();
    auto piGate = std::make_shared<AsyncGate>();
    auto cfGate = std::make_shared<AsyncGate>();

    log->trace("gates");
    pi->removeDetour();
    pe->removeDetour();

    //removeDetour(cf, cfGate, log);
    //removeDetour(pi, piGate, log);
    //piGate->onReady([pe, peGate, log] {
    //    removeDetour(pe, peGate, log);
    //});

    log->trace("remove");
    teardownFence->onReady([this] {
        printf("teardown ready\n");
        staticResolver_ = nullptr;
        Runtime::yeet();
        setPluginYeetable();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
//

extern "C" __declspec(dllexport) void*
create() {
    return new AIM();
}

extern "C" __declspec(dllexport) void destroy(const IPlugin* instance) {
    delete instance;
}