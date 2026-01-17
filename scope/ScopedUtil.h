#pragma once

// 🧠 Scoped utilities for RAII, guarded logic, and defer-style cleanup

#include "EventContext.h"

#include <utility>

namespace scopedutil {

struct ScopedFlag {
    bool& flag;
    ScopedFlag(bool& f)
      : flag(f) {
        flag = true;
    }
    ~ScopedFlag() { flag = false; }
};

template<typename F>
void
runGuarded(bool& flag, F&& fn) {
    if (flag)
        return;
    ScopedFlag guard(flag);
    std::forward<F>(fn)();
}

template<typename F>
void
runGuardedBlocking(bool& flag, InvocationContext& ctx, F&& fn) {
    if (flag) {
        ctx.setBlocking(true);
        return;
    }
    ScopedFlag guard(flag);
    std::forward<F>(fn)();
    ctx.setBlocking(true);
}
}

template<typename F>
struct DeferHelper {
    F func;
    DeferHelper(F f)
      : func(f) {}
    ~DeferHelper() { func(); }
};

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define DEFER(code) auto CONCAT(_defer_, __LINE__) = ScopedUtil::DeferHelper([&]() { code; })