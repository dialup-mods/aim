#pragma once
#ifdef AIM_BUILD
    #define AIM_API __declspec(dllexport)
#else
    #define AIM_API __declspec(dllimport)
#endif

#include <string_view>

#include "IModule.h"

class UObject;
class UClass;

class AIM_API IObjectProvider : public IModule {
public:
    AIM_INJECTABLE(IObjectProvider)

    IObjectProvider() = default;
    virtual ~IObjectProvider() = default;

    template<typename T>
    UClass* classOf() {
        static UClass* cls = resolveClass(T::className);
        return cls;
    }

    virtual auto resolveClass(std::string_view className)  -> UClass* = 0;
};