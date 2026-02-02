#pragma once
#include <stdexcept>

#include "CallbackBuilder.h"
#include "TaskStructs.h"
#include "IModule.h"

namespace c = callbackbuilder;

class AIM_API ITaskBuilder : public IModule {
public:
    AIM_INJECTABLE(ITaskBuilder)

    using CallbackType = std::function<void(InvocationContext&)>;

    ITaskBuilder() = default;
    ~ITaskBuilder() override = default;

    virtual ITaskBuilder& name(std::string n) = 0;
    virtual ITaskBuilder& functionName(std::string fn) = 0;
    virtual ITaskBuilder& phase(HookPhase p) = 0;
    virtual ITaskBuilder& once(bool once = true) = 0;
    virtual ITaskBuilder& nextTick(bool deferred = false) = 0;
    virtual ITaskBuilder& retry(int attempts, float seconds) = 0;
    virtual ITaskBuilder& maxAttempts(int maxAttempts) = 0;
    virtual ITaskBuilder& timeoutSeconds(float timeoutSeconds) = 0;

    template<typename F>
    ITaskBuilder& callback(F&& f) {
        setCallback(c::wrapCallback(std::forward<F>(f)));
        return *this;
    }

    template<typename F>
    ITaskBuilder& callbackBlocking(F&& f) {
        setCallbackBlocking(c::wrapCallback(std::forward<F>(f)));
        return *this;
    }

    template<typename F>
    ITaskBuilder& preStep(F&& f) {
        setPreStep(c::wrapCallback(std::forward<F>(f)));
        return *this;
    }

    template<typename F>
    ITaskBuilder& successCondition(F&& f) {
        setPreStep(c::wrapCallback(std::forward<F>(f)));
        return *this;
    }

    template<typename F>
    ITaskBuilder& successCallback(F&& f) {
        setPreStep(c::wrapCallback(std::forward<F>(f)));
        return *this;
    }

    template<typename F>
    ITaskBuilder& failureCallback(F&& f) {
        setPreStep(c::wrapCallback(std::forward<F>(f)));
        return *this;
    }

    virtual std::shared_ptr<TaskDefinition> build() = 0;

protected:
    virtual void setCallback(CallbackType cb) = 0;
    virtual void setPreStep(CallbackType cb) = 0;
    virtual void setSuccessCondition(CallbackType cb) = 0;
    virtual void setSuccessCallback(CallbackType cb) = 0;
    virtual void setFailureCallback(CallbackType cb) = 0;
};