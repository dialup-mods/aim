#pragma once
#include <string>
#include "ValueResolver.h"

struct FloatResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        const uint32_t raw = *reinterpret_cast<uint32_t*>(valuePtr);

        out.storage = ResolvedValue::StorageType::Float;
        out.kind = ResolvedValue::Kind::Float;
        out.primitiveStr = std::to_string(raw);
    }
};