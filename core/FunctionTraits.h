#if false
#pragma once
#include <tuple>
#include <type_traits>

// This code serves as a reminder of why you question everything in an existing codebase.
// I made a beautifully nerdy, fully functional trait-based dispatch system when there was
// zero reason not to just combine PreEventContext and PostEventContext into a singular
// class that does the needful
//
// Yes, it worked. Yes, we used this. No, we're not deleting it.
//
// Some code exists to run. This code exists to *warn*
// 
// jk it's pure flex

/* usage:

        template <typename TContext>
        void registerTypedTask(std::string name, std::function<void(TContext&)> cb) {
            static_assert(std::is_base_of_v<Context, TContext>, "Context type must derive from Context");
            // store your fancy erased callback here, like a fine wine
        }


// primary template: fallback to empty
template<typename T, typename = void>
struct FunctionTraits {
    static constexpr size_t arity = 0;
};

// for lambdas and functors with non-templated operator()
template<typename T>
struct FunctionTraits<T, std::void_t<decltype(&T::operator())>>
    : FunctionTraits<decltype(&T::operator())> {};

// specialization for const lambdas
template<typename ClassType, typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType(ClassType::*)(Args...) const> {
    static constexpr size_t arity = sizeof...(Args);
    using arg0 = std::tuple_element_t<0, std::tuple<Args...>>;
};

// specialization for non-const lambdas (rare)
template<typename ClassType, typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType(ClassType::*)(Args...)> {
    static constexpr size_t arity = sizeof...(Args);
    using arg0 = std::tuple_element_t<0, std::tuple<Args...>>;
};

// for plain function pointers
template<typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType (*)(Args...)> {
    static constexpr size_t arity = sizeof...(Args);
    using arg0 = std::tuple_element_t<0, std::tuple<Args...>>;
};

// for std::function
template<typename ReturnType, typename... Args>
struct FunctionTraits<std::function<ReturnType(Args...)>> {
    static constexpr size_t arity = sizeof...(Args);
    using arg0 = std::tuple_element_t<0, std::tuple<Args...>>;
};


/*
struct TaskDefinition {
    using ContextKind = EventContext::ContextKind;
        
    std::function<void(EventContext&)> callback{};
    std::function<void(EventContext&)> preStep{};
    std::function<bool(EventContext&)> successCondition{};
    std::function<void(EventContext&)> onSuccessCallback{};
    std::function<void(EventContext&)> onFailureCallback{};
    std::function<void(EventContext&)> afterSuccessCallback{};
    std::function<void(EventContext&)> afterFailureCallback{};

    ContextKind callbackContext{ContextKind::None};
    ContextKind preStepContext{ContextKind::None};
    ContextKind successContext{ContextKind::None};
    ContextKind onSuccessContext{ContextKind::None};
    ContextKind onFailureContext{ContextKind::None};
    ContextKind afterSuccessContext{ContextKind::None};
    ContextKind afterFailureContext{ContextKind::None};

};
*/

/*
namespace callbackBuilder {
    template<typename T>
    constexpr bool always_false = false;

    template<typename F>
    auto makeContextAwareCallback(F&& lambda) {
        using CleanF = std::decay_t<F>;
        EventContext::ContextKind contextKind = EventContext::ContextKind::None;

        if constexpr (std::is_invocable_v<CleanF, PreEventContext&>) {
            using ReturnType = std::invoke_result_t<CleanF, PreEventContext&>;
            std::function<ReturnType(EventContext&)> wrapped = 
                [lambda = std::forward<F>(lambda)](EventContext& ctx) -> ReturnType {
                    return lambda(static_cast<PreEventContext&>(ctx));
                };
            contextKind = EventContext::ContextKind::Pre;
            return std::make_tuple(wrapped, contextKind);

        } else if constexpr (std::is_invocable_v<CleanF, PostEventContext&>) {
            using ReturnType = std::invoke_result_t<CleanF, PostEventContext&>;
            std::function<ReturnType(EventContext&)> wrapped = 
                [lambda = std::forward<F>(lambda)](EventContext& ctx) -> ReturnType {
                    return lambda(static_cast<PostEventContext&>(ctx));
                };
            contextKind = EventContext::ContextKind::Post;
            return std::make_tuple(wrapped, contextKind);

        } else if constexpr (std::is_invocable_v<CleanF>) {
            using ReturnType = std::invoke_result_t<CleanF>;
            std::function<ReturnType(EventContext&)> wrapped = 
                [lambda = std::forward<F>(lambda)](EventContext&) -> ReturnType {
                    return lambda();
                };
            contextKind = EventContext::ContextKind::None;
            return std::make_tuple(wrapped, contextKind);

        } else {
            static_assert(always_false<F>, "Lambda must be invocable with PreEventContext, PostEventContext, or no args.");
        }
    }
}
*/

/*
    TaskBuilder& callback(auto&& lambda) {
        auto [callback, contextKind] = callbackBuilder::makeContextAwareCallback(std::forward<decltype(lambda)>(lambda));
        task_->callback = callback;
        task_->callbackContext = contextKind;
        return *this;
    }

*/

/*
template <typename ReturnType>
ReturnType TaskQueue::invokeWithStoredContext(const std::function<ReturnType(EventContext&)>& fn,
                                   EventContext::ContextKind kind,
                                   PreEventContext& pre,
                                   std::optional<PostEventContext>& post)
{
    if (!fn) {
        if constexpr (!std::is_void_v<ReturnType>) return ReturnType{};
        else return;
    }

    EventContext& ctx = [&]() -> EventContext& {
        switch (kind) {
            case EventContext::ContextKind::Pre:  return pre;
            case EventContext::ContextKind::Post: return post.value();
            case EventContext::ContextKind::None: return pre; // Doesn't matter, will be ignored
        }
        return pre; // Fallback
    }();

    return fn(ctx);
}
*/
#endif