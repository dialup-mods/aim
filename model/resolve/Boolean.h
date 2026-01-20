#pragma once
#include "ValueResolver.h"
#include "SDK.h"

struct BooleanResolver {
    static void resolve(ResolvedValue& out, void* valuePtr, const uint32_t bitMask) {
        const uint32_t raw = *reinterpret_cast<uint32_t*>(valuePtr);
        const bool value = (raw & bitMask) != 0;

        out.storage = ResolvedValue::StorageType::UInt32;
        out.kind = ResolvedValue::Kind::Bool;
        out.primitiveStr = value ? "true" : "false";
    }
};