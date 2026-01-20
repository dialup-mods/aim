#pragma once
#include "ValueResolver.h"
#include "SDK.h"

struct IntegerResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        const uint32_t raw = *reinterpret_cast<uint32_t*>(valuePtr);

        out.storage = ResolvedValue::StorageType::UInt32;
        out.kind = ResolvedValue::Kind::Int32;
        out.primitiveStr = std::to_string(raw);
    }
};