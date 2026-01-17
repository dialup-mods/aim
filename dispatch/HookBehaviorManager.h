#pragma once
#include "SdkHeaders.hpp"
#include "CoreStructs.h"
#include "SafeLogger.h"

#include <regex>

enum class HookBehavior {
    Allow,   // forward to trampoline
    Block,   // suppress original call
    LogOnly, // log, then forward
    BlockAndLog
};

class HookBehaviorManager {
  public:
    HookBehaviorManager()
        : ctx_(ctx)
        , log_(ctx.safeLogger) {
        log_->info("HookBehaviorManager loaded");
    }
    
    ~HookBehaviorManager() {
        log_->info("HookBehaviorManager unloaded");
    }

    struct Rule {
        std::regex functionPattern;
        HookBehavior behavior;
    };

    void addRule(const std::string& regexPattern, HookBehavior behavior) {
        rules_.emplace_back(Rule{ std::regex(regexPattern), behavior });
    }

    HookBehavior getBehavior(UFunction* fn) const {
        if (!fn)
            return HookBehavior::Allow;
        std::string name = fn->GetFullName();
        for (const auto& rule : rules_) {
            if (std::regex_match(name, rule.functionPattern)) {
                return rule.behavior;
            }
        }
        return HookBehavior::Allow;
    }

  private:
    const PluginContext& ctx_;
    SafeLogger log_;
    std::vector<Rule> rules_;
};

extern HookBehaviorManager behaviorManager;
