#pragma once
#include "IModule.h"
#include "SDK.h"

class IRuntime : public IModule {
    AIM_INJECTABLE(IRuntime)

    template<typename T>
    auto classOf() -> UClass* {
        static UClass* cls = resolveClass(T::className);
        return cls;
    }
    virtual auto resolveClass(std::string_view className) -> UClass* = 0;

    template<typename T>
    auto getFirst() -> T* {
        return static_cast<T*>(getFirst(T::className));
    }
    virtual auto getFirst(std::string_view className) -> UObject* = 0;

    template<typename T>
    auto getAll() -> std::vector<UObject*> {
        std::vector<T*> out;
        auto found = getAll(T::className);
        if (!found.size()) { return out; }
        for (auto* obj : found) {
            out.emplace_back(static_cast<T*>(obj));
        }
        return out;
    }
    virtual auto getAll(std::string_view className) -> std::vector<UObject*> = 0;

    virtual auto getUObjects() -> TArray<UObject*> = 0;
};