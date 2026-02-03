#pragma once
#include "EventContext.h"

/**
 * Cross-DLL safe callback wrapper.
 *
 * Problem: std::function allocates on the heap of the DLL that constructs it.
 * When destroyed in a different DLL (e.g., created in plugin, destroyed in AIM),
 * it attempts to free memory using the wrong heap allocator → crash.
 *
 * Solution: Wrap the lambda with explicit function pointers and a custom deleter.
 * The 'destroy' function pointer is compiled in the same DLL as 'new', ensuring
 * deletion uses the correct heap. The 'invoke' wrapper includes SEH exception
 * handling to catch crashes without taking down the game.
 */

struct CallbackWrapper {
    void* context{ nullptr };
    void (*invoke)(void*, InvocationContext&){ nullptr };
    void (*destroy)(void*){ nullptr };

    CallbackWrapper() = default;
    CallbackWrapper(void* ctx, void(*inv)(void*, InvocationContext&), void(*del)(void*))
    : context(ctx), invoke(inv), destroy(del) {}

    // No constructor, just these methods:
    void operator()(InvocationContext& ctx) {
        if (invoke) invoke(context, ctx);
    }

    explicit operator bool() const { return invoke != nullptr; }

    ~CallbackWrapper() { cleanup(); }

    // Move ops...
    CallbackWrapper(CallbackWrapper&& other) noexcept
        : context(other.context), invoke(other.invoke), destroy(other.destroy) {
        other.context = nullptr;
        other.invoke = nullptr;
        other.destroy = nullptr;
    }

    CallbackWrapper& operator=(CallbackWrapper&& other) noexcept {
        if (this != &other) {
            cleanup();
            context = other.context;
            invoke = other.invoke;
            destroy = other.destroy;
            other.context = nullptr;
            other.invoke = nullptr;
            other.destroy = nullptr;
        }
        return *this;
    }

    CallbackWrapper(const CallbackWrapper&) = delete;
    CallbackWrapper& operator=(const CallbackWrapper&) = delete;

private:
    void cleanup() {
        if (destroy && context) destroy(context);
        context = nullptr;
        invoke = nullptr;
        destroy = nullptr;
    }
};

// For bool-returning callbacks
struct CallbackWithReturnWrapper {
    void* context{ nullptr };
    bool (*invoke)(void*, InvocationContext&) { nullptr };
    void (*destroy)(void*) { nullptr };

    bool operator()(InvocationContext& ctx) {
        if (invoke) return invoke(context, ctx);
        return true;
    }

    CallbackWithReturnWrapper() = default;
    CallbackWithReturnWrapper(void* ctx, bool(*inv)(void*, InvocationContext&), void(*del)(void*))
    : context(ctx), invoke(inv), destroy(del) {}

    explicit operator bool() const { return invoke != nullptr; }

    ~CallbackWithReturnWrapper() { cleanup(); }

    CallbackWithReturnWrapper(CallbackWithReturnWrapper&& other) noexcept
        : context(other.context), invoke(other.invoke)
        , destroy(other.destroy) {
        other.context = nullptr;
        other.invoke = nullptr;
        other.destroy = nullptr;
    }

    CallbackWithReturnWrapper& operator=(CallbackWithReturnWrapper&& other) noexcept {
        if (this != &other) {
            cleanup();
            context = other.context;
            invoke = other.invoke;
            destroy = other.destroy;
            other.context = nullptr;
            other.invoke = nullptr;
            other.destroy = nullptr;
        }
        return *this;
    }

    CallbackWithReturnWrapper(const CallbackWithReturnWrapper&) = delete;
    CallbackWithReturnWrapper& operator=(const CallbackWithReturnWrapper&) = delete;

private:
    void cleanup() {
        if (destroy && context) destroy(context);
        context = nullptr;
        invoke = nullptr;
        destroy = nullptr;
    }
};