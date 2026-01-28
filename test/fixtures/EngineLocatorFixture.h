#pragma once
#include <memory>

#include "MockLogger.h"
#include "EngineLocator.h"

struct EngineLocatorFixture {
    std::shared_ptr<MockLogger> logger = std::make_shared<MockLogger>();

    EngineLocator engineLocator;

    EngineLocatorFixture() {
        engineLocator.__inject_log(logger);
    }
};