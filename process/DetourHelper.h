#pragma once
#include <future>


/*
std::future<bool> waitForBakkesAndPatch(
    std::function<void()> detourFunc,
    std::function<bool()> checkFunc,
    std::shared_ptr<SafeLogger> log
) {
    log->debug("waitForBakkesAndPatch()");

    auto checkFunc = [this]() { return getIsBakkesModPatched(); };
    
    auto detourFunc = [derived = derived()] {
        return derived->applyDetour();
    };
    
    auto promise = std::make_shared<std::promise<bool>>();

    std::thread([log, checkFunc, detourFunc, promise]() {
        for (int i = 0; i < 100; ++i) {
            if (checkFunc()) {
                log()->debug("BakkesMod patch detected, applying detour.");
                promise->set_value(detourFunc());
                return;
            }
            Sleep(6000);
        }

        log()->warn("Timeout waiting for BakkesMod patch.");
        promise->set_value(false);
        
    }).detach();

    return promise->get_future();
}
*/