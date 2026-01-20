#pragma once
#include "ValueResolver.h"
#include "SDK.h"

struct ByteResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr, UEnum* enumMaybe) {
        const int32_t raw = *reinterpret_cast<int32_t*>(valuePtr);

        out.mStorage = MyResolvedValue::StorageType::UInt32;

        if (enumMaybe) {
            out.mKind = MyResolvedValue::Kind::Enum;
            out.uEnum = enumMaybe;
            if (raw < enumMaybe->Names.size()) {
                const FName& name = enumMaybe->Names[raw];
                out.primitiveStr = name.ToString();
            } else {
                out.primitiveStr =
                    "<invalid:" + std::to_string(raw) + ">";
            }
        } else {
            out.mKind = MyResolvedValue::Kind::Int32;
            out.primitiveStr = std::to_string(raw);
        }
    }
};