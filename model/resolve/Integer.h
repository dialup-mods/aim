#pragma once
#include "ValueResolver.h"
#include "SDK.h"

struct IntegerResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr) {
        const uint32_t raw = *reinterpret_cast<uint32_t*>(valuePtr);

        out.mStorage = MyResolvedValue::StorageType::UInt32;
        out.mKind = MyResolvedValue::Kind::Int32;
        out.primitiveStr = std::to_string(raw);
    }
};