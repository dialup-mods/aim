#pragma once

#include <string_view>
#include "IModule.h"

class UObject; class UClass;

class IObjectQuery : public IModule {
public:
    AIM_INJECTABLE(IObjectProvider)

    IObjectQuery() = default;
    ~IObjectQuery() override = default;

    // UE's "StaticClass"
    template<typename T>
    UClass* classOf() {
        static_assert(std::is_base_of_v<UObject, T>);
        static UClass* cls = resolveClass(T::className);
        return cls;
    }
    virtual auto resolveClass(std::string_view className) -> UClass* = 0;

    template<typename T>
    auto getFirst() -> T* {
        static_assert(std::is_base_of_v<UObject, T>);
        return static_cast<T*>(getFirst(T::className));
    }
    virtual auto getFirst(std::string_view className) -> UObject* = 0;

    template<typename T>
    auto getAll() -> std::vector<T*> {
        static_assert(std::is_base_of_v<UObject, T>);
        auto raw = getAll(T::className);

        std::vector<T*> out;
        out.reserve(raw.size());

        for (UObject* obj : raw) {
            out.emplace_back(static_cast<T*>(obj));
        }

        return out;
    }
    virtual auto getAll(std::string_view className) -> std::vector<UObject*> = 0;
};