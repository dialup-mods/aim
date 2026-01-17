#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "DependencyContainer.h"
#include "DependencyInjector.h"
#include "ModuleLoader.h"

#include "LogHandler.h"
#include "SafeLogger.h"

#include "PatchDefinition.h"
#include "PatchManager.h"
#include "PatchBuilder.h"

TEST_CASE("PatchManager can be resolved and used") {
    DependencyContainer& container = DependencyContainer::getInstance();
    DependencyInjector& injector = DependencyInjector::getInstance();

    auto logHandler = std::make_shared<LogHandler>();
    logHandler->setLogLevel(LogLevel::LOG_DEBUG);
    logHandler->addSink(std::make_shared<ConsoleLogSink>());

    auto fileSink = std::make_shared<FileLogSink>("PatchManagerTests.log");
    if (fileSink->isAvailable()) {
        logHandler->addSink(fileSink);
    }

    container.registerInstance<LogHandler>(logHandler);
    container.registerInstance<SafeLogger>(std::make_shared<SafeLogger>(std::weak_ptr(logHandler)));

    auto buildPatchManager = std::make_unique<ModuleBuilder>();
    buildPatchManager->add<PatchManager>()
           .withDependency(&PatchManager::__inject_log, "[default]")
           .named("PE-PatchManager")
           .asSingleton();
    buildPatchManager->run(container, injector);

    auto pm = container.resolve<PatchManager>("PE-PatchManager");
    REQUIRE(pm != nullptr);

    PatchDefinition patch = PatchBuilder()
        .name("Test")
        .setPosition(0x12341234)
        .absoluteJump(0x01010101)
        .finalize();

    // Try to call a method that would crash if bad
    CHECK_NOTHROW(pm->add(patch));
    CHECK_NOTHROW(pm->dryRun(patch));
}
