#pragma once
#include <AsyncGate.h>
#include <memory>

#include "RuntimeWrapper.h"
#include "MockLogger.h"
#include "ObjectProvider.h"

struct ObjectProviderFixture {
    std::shared_ptr<MockLogger> logger = std::make_shared<MockLogger>();

    std::shared_ptr<AsyncGate> asyncGate =
        std::make_shared<AsyncGate>(); // inert is fine

    std::shared_ptr<RuntimeWrapper> runtime;

    ObjectProvider provider;

    ObjectProviderFixture() {
        provider.__inject_log(logger);
        provider.__inject_asyncGate(asyncGate);
        provider.__inject_runtime(runtime);
    }

    ~ObjectProviderFixture() = default;
};