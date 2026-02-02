#pragma once
#include "IModule.h"

struct HINSTANCE__;
typedef HINSTANCE__* HMODULE;

class ILogger;
class UObject;
class FNameEntry;

class EngineLocator final : public IModule {
    AIM_INJECTABLE(EngineValidator)
    AIM_INJECT(ILogger, log)

    auto findPattern(HMODULE module, const unsigned char* pattern, const char* mask) -> uintptr_t;
    auto int_to_hex(uintptr_t value, size_t width = sizeof(uintptr_t) * 2) -> std::string;

    auto getFNameEntriesAddress() -> uintptr_t;
    auto getUObjectsAddress() -> uintptr_t;
};