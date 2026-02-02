#pragma once
#include "IModule.h"

class UObject;
class UClass;

class IObjectProvider : public IModule {
public:
    AIM_INJECTABLE(IObjectProvider)

    IObjectProvider() = default;
    virtual ~IObjectProvider() = default;

    //virtual UObject* getByClass(UClass* cls) const = 0;

    // ⚠️ DO NOT USE ACROSS DLL BOUNDARIES. This is inline template hell.
    template<typename T>
    T* getInstanceOf() const {
        return static_cast<T*>(getByClass(T::StaticClass()));
    }
};
