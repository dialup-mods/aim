#pragma once
#include "MockLogger.h"
#include "PatchManager.h"

struct PatchManagerFixture {
    std::shared_ptr<MockLogger> logger = std::make_shared<MockLogger>();

    PatchManager patchManager;

    PatchManagerFixture() {
        patchManager.__inject_log(logger);
    }

    ~PatchManagerFixture() = default;
};
