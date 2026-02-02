#pragma once
#include <Windows.h>

#include <functional>

class InvocationContext;

namespace callbackbuilder {
template<typename T>
constexpr bool always_false = false;

template<typename F>
auto
wrapCallback(F&& f) {
    using Fn = std::decay_t<F>;
    if constexpr (std::is_invocable_v<Fn, InvocationContext&>) {
        using R = std::invoke_result_t<Fn, InvocationContext&>;
        return std::function<R(InvocationContext&)>{ [f = std::forward<F>(f)](InvocationContext& ctx) -> R {
            __try {
                if constexpr (std::is_void_v<R>) {
                    f(ctx);
                } else {
                    return f(ctx);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                OutputDebugStringA("SEH: Task callback crashed\n");
                printf("p SEH: Task callback crashed\n");
                if constexpr (!std::is_void_v<R>) {
                    return R{}; // Return default value for non-void
                }
            }
        } };
    } else if constexpr (std::is_invocable_v<Fn>) {
        // no-args version
        using R = std::invoke_result_t<Fn>;
        return std::function<R(InvocationContext&)>{ [f = std::forward<F>(f)](InvocationContext&) -> R {
            __try {
                if constexpr (std::is_void_v<R>) {
                    f();
                } else {
                    return f();
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                OutputDebugStringA("SEH: Task callback crashed\n");
                if constexpr (!std::is_void_v<R>) {
                    return R{};
                }
            }
        } };
    } else {
        static_assert(always_false<F>, "wrapCallback: lambda must accept InvocationContext& or nothing");
    }
}
}