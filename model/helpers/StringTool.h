#pragma once
#include <Windows.h>
#include <string>
#include <string_view>

// fixme DRY, this is probably the better implementation
namespace string_tool {

static auto enshrinken(const std::wstring& input) -> std::string {
    return enshrinken(std::wstring_view(input).data());
}

static auto enshrinken(const std::wstring_view input) -> std::string {
    if (input.empty()) return "";

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

}
