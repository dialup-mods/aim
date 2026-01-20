#pragma once
#include <Windows.h>

#include "ValueResolver.h"
#include "SDK.h"

struct FStringResolver {
    // fixme DRY, this is probably the better implementation
    static auto enshrinken(const std::wstring& input) -> std::string {
        return enshrinken(std::wstring_view(input));
    }

    static auto enshrinken(std::wstring_view input) -> std::string {
        if (input.empty()) return "";

        // WideCharToMultiByte expects int length
        const int wideLen = static_cast<int>(input.size());

        int sizeRequired = WideCharToMultiByte(
            CP_UTF8,
            0,
            input.data(),   // NOT c_str()
            wideLen,        // explicit length
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (sizeRequired <= 0) {
            return "[enshrinken error]";
        }

        std::string result(sizeRequired, '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            input.data(),
            wideLen,
            result.data(),
            sizeRequired,
            nullptr,
            nullptr
        );

        return result;
    }

    static void resolve(MyResolvedValue& out, void* valuePtr) {
        FString* str = *reinterpret_cast<FString**>(valuePtr);

        if (!str) {
            out.mKind = MyResolvedValue::Kind::Null;
            return;
        }

        out.mStorage = MyResolvedValue::StorageType::InlineStruct;
        out.mKind = MyResolvedValue::Kind::String;
        if (!str || !str->ArrayData || str->ArrayCount <= 0) {
            out.mKind = MyResolvedValue::Kind::String;
            out.primitiveStr = "";
            return;
        }

        // UE3 FString is UTF-16
        std::wstring_view wsv(str->ArrayData, str->ArrayCount);
        out.primitiveStr = enshrinken(wsv);
    }
};