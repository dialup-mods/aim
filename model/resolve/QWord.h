#pragma once
#include <string>
#include "ValueResolver.h"

struct QWordResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr) {
        const uint64_t raw = *reinterpret_cast<uint64_t*>(valuePtr);

        out.mStorage = MyResolvedValue::StorageType::Int64;
        out.mKind = MyResolvedValue::Kind::Int64;
        out.primitiveStr = std::to_string(raw);
    }
};