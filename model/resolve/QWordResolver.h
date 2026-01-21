#pragma once
#include <string>
#include "ValueResolver.h"

struct QWordResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        const uint64_t raw = *reinterpret_cast<uint64_t*>(valuePtr);

        out.storage = ResolvedValue::StorageType::Int64;
        out.kind = ResolvedValue::Kind::Int64;
        out.primitiveStr = std::to_string(raw);
    }
};