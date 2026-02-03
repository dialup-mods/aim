#pragma once
#include <Windows.h>
#include "EventContext.h"
#include "CallbackWrapper.h"

namespace callbackbuilder {

// For void callbacks
template<typename F>
auto wrapCallback(F&& f) -> CallbackWrapper {
    using Fn = std::decay_t<F>;
    auto* ctx = new Fn(std::forward<F>(f));

    if constexpr (std::is_invocable_v<Fn, InvocationContext&>) {
        // Takes InvocationContext
        return CallbackWrapper{
            ctx,
            [](void* p, InvocationContext& ic) {
                printf("Invoke callback wrapper\n");
                __try {
                    (*static_cast<Fn*>(p))(ic);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    printf("SEH: Task callback crashed\n");
                }
            },
            [](void* p) { delete static_cast<Fn*>(p); } // destroy
        };
    } else {
        // No args
        return CallbackWrapper{
            ctx,
            [](void* p, InvocationContext&) {
                printf("Invoke callback wrapper\n");
                __try {
                    (*static_cast<Fn*>(p))();
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    printf("SEH: Task callback crashed\n");
                }
            },
            [](void* p) { delete static_cast<Fn*>(p); } // destroy
        };
    }
}

// For bool-returning callbacks (conditions, blocking, etc)
template<typename F>
auto wrapWithReturn(F&& f) -> CallbackWithReturnWrapper {
    using Fn = std::decay_t<F>;
    auto* ctx = new Fn(std::forward<F>(f));

    if constexpr (std::is_invocable_v<Fn, InvocationContext&>) {
        return CallbackWithReturnWrapper{
            ctx,
            [](void* p, InvocationContext& ic) -> bool {
                printf("Invoke callback wrapper\n");
                __try {
                    printf("executing callback wrapper\n");
                    return (*static_cast<Fn*>(p))(ic);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    printf("SEH: Condition crashed\n");
                    return false;
                }
            },
            [](void* p) { delete static_cast<Fn*>(p); } // destroy
        };
    } else {
        return CallbackWithReturnWrapper{
            ctx,
            [](void* p, InvocationContext&) -> bool {
                printf("Invoke callback wrapper\n");
                __try {
                    printf("executing callback wrapper\n");
                    return (*static_cast<Fn*>(p))();
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    printf("SEH: Condition crashed\n");
                    return false;
                }
            },
            [](void* p) { delete static_cast<Fn*>(p); } // destroy
        };
    }
}
}