#pragma once

#include "ValueResolver.h"
#include "SDK.h"
#include "StringTool.h"

struct FStringResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        const FString* str = *reinterpret_cast<FString**>(valuePtr);

        if (!str) {
            out.kind = ResolvedValue::Kind::Null;
            return;
        }

        out.storage = ResolvedValue::StorageType::InlineStruct;
        out.kind = ResolvedValue::Kind::String;
        if (!str || !str->ArrayData || str->ArrayCount <= 0) {
            out.kind = ResolvedValue::Kind::String;
            out.primitiveStr = "";
            return;
        }

        // UE3 FString is UTF-16
        const std::wstring_view wsv(str->ArrayData, str->ArrayCount);
        out.primitiveStr = string_tool::enshrinken(wsv);
    }
};