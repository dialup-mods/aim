//#pragma once
//#include "AIM.h"
//#include "DependencyContainer.h"
//#include "DependencyInjector.h"
//#include "IModule.h"
//#include "IModules.h"
//
//std::atomic destroyHasBegun = false;
//
//class Unload {
//
//  private:
//    std::shared_ptr<DialUp> pluginSingleton_;
//
//  public:
//    Unload(const std::shared_ptr<DialUp>& pluginSingleton)
//      : pluginSingleton_(pluginSingleton) {}
//
//    template<typename T>
//    static void removeDetour(std::shared_ptr<T> fn, std::shared_ptr<AsyncGate> gate, std::shared_ptr<SafeLogger> log) {
//
//        fn->removeDetour();
//
//        std::thread([log, fn, gate] {
//            if (fn->waitForUnlock(500)) {
//                log->debug(fn->getName() + " gate ready");
//            } else {
//                log->warn(fn->getName() + " detour watchdog timed out");
//            }
//            gate->setReady();
//        }).detach();
//    }
//
//    static void triggerUnloadCascade(std::shared_ptr<DialUp>* pluginPtr) {
//        // if (g_pluginState != Running && g_pluginState != Failed) {
//        //     printf("destroy() called when not running or failed -- skipping.\n");
//        // }
//        //
//        // if (destroyHasBegun) {
//        //     printf("destroy() already in progress -- skipping.\n");
//        //     return;
//        // }
//        //
//        // g_pluginState = Terminating;
//        // destroyHasBegun = true;
//
//        auto log = AIM::getContainer()->resolve<SafeLogger>();
//        auto pe = AIM::getContainer()->resolve<ProcessEvent>();
//        auto pi = AIM::getContainer()->resolve<ProcessInternal>();
//        auto cf = AIM::getContainer()->resolve<CallFunction>();
//
//        auto peGate = std::make_shared<AsyncGate>();
//        auto piGate = std::make_shared<AsyncGate>();
//        auto cfGate = std::make_shared<AsyncGate>();
//
//        // clang-format off
//        removeDetour(cf, cfGate, log);
//
//        cfGate->onReady([pi, piGate, log] {
//            removeDetour(pi, piGate, log);
//        });
//
//        piGate->onReady([pe, peGate, log] {
//            removeDetour(pe, peGate, log);
//        });
//
//        peGate->onReady([pluginPtr] {
//            phase2(pluginPtr);
//        });
//        // clang-format on
//    }
//
//    static void safeYeet(const char* name, IModule* raw) {
//        __try {
//            raw->yeet();
//        } __except (EXCEPTION_EXECUTE_HANDLER) { printf("%s refused to yeet (SEH caught it)\n", name); }
//    }
//
//    static void yeetDependenciesAndContainer() {
//        //AIM::getInjector()->clear();
//
//        for (const auto& [name, module] : AIM::getContainer()->getDependencies()) {
//            if (module) {
//                printf("force yeeting: %s\n", name.c_str());
//                std::shared_ptr<IModule> mod = std::reinterpret_pointer_cast<IModule>(module);
//                IModule* raw = mod.get();
//                safeYeet(name.c_str(), raw);
//            }
//        }
//
//        AIM::getContainer()->clear();
//    }
//
//    static void phase2(std::shared_ptr<DialUp>* pluginPtr) {
//        Unload::yeetDependenciesAndContainer();
//
//        // g_pluginState = Uninitialized;
//
//        g_pluginMutex->release();
//
//        printf("Goodbye.\n");
//
//        pluginPtr->reset();
//    }
//};
