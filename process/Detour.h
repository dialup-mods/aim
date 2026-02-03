#pragma once
#include "IModule.h"
#include "ILogger.h"
#include "polyhook2/Detour/x64Detour.hpp"

class Detour : public IModule {
    AIM_INJECTABLE(ProcessEvent)

    AIM_INJECT(ILogger, log)

    Detour()
        : dis_(std::make_unique<PLH::ZydisDisassembler>(PLH::Mode::x64))
    {}

    ~Detour() {
        if (detour_) {
            detour_->unHook();
            log_->debug("Detour removed");
        }
    }

    bool attach(void* target, void* hook) {
        detour_ = std::make_unique<PLH::x64Detour>(
            reinterpret_cast<uint64_t>(target), // address
            reinterpret_cast<uint64_t>(hook),   // target
            &trampoline_                        // user trampoline
        );

        if (!detour_->hook()) {
            log_->error("Failed to hook {}", target);
            return false;
        }

        log_->info("Hooked {} -> {}, trampoline at {}",
                  target, hook, (void*)trampoline_);
        return true;
    }

    bool detach() {
        return detour_->unHook();
    }

    void* getTrampoline() const { return (void*)trampoline_; }

private:
    std::unique_ptr<PLH::x64Detour> detour_;
    std::unique_ptr<PLH::ZydisDisassembler> dis_;
    void* target_ = nullptr;
    uint64_t trampoline_ = 0;
};