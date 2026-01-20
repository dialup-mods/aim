#pragma once
#include <string>
#include "ValueResolver.h"

struct FloatResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr) {
        const uint32_t raw = *reinterpret_cast<uint32_t*>(valuePtr);

        out.mStorage = MyResolvedValue::StorageType::Float;
        out.mKind = MyResolvedValue::Kind::Float;
        out.primitiveStr = std::to_string(raw);
    }
};