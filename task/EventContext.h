#pragma once
struct FFrame;
class UObject;
class UFunction;

class InvocationContext {
  public:
    enum class Source { CallFunction, ProcessInternal, ProcessEvent };

    static InvocationContext makeProcessEventContext(UObject* self, UFunction* fn, void* params, void* result = nullptr) {
        InvocationContext ctx;
        ctx.self_ = self;
        ctx.function_ = fn;
        ctx.params_ = params;
        ctx.result_ = result;
        return ctx;
    }

    static InvocationContext makeProcessInternalContext(UObject* self, FFrame& stack, void* result = nullptr) {
        InvocationContext ctx;
        ctx.self_ = self;
        ctx.stack_ = &stack;
        ctx.result_ = result;
        return ctx;
    }

    static InvocationContext makeCallFunctionContext(UObject* self, FFrame& stack, void* result, UFunction* fn) {
        InvocationContext ctx;
        ctx.self_ = self;
        ctx.stack_ = &stack;
        ctx.result_ = result;
        ctx.function_ = fn;
        return ctx;
    }

    // accessors
    UObject* self() const { return self_; }

    template<typename T>
    T* getSelf() const {
        return static_cast<T*>(self_);
    }

    void* rawParams() const { return params_; }

    template<typename T>
    T* getParams() const {
        return reinterpret_cast<T*>(params_);
    }

    UFunction* function() const { return function_; }
    FFrame& stack() const { return *stack_; }
    void* result() const { return result_; }

  private:
    InvocationContext() {}

    UObject* self_;
    UFunction* function_;
    void* params_;
    void* result_{ nullptr };
    FFrame* stack_{};
};
