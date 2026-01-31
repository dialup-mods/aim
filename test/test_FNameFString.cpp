#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "SDK.h"

TEST_CASE("FName and FString") {
const wchar_t* test = L"Hello UE";
CHECK(test[wcslen(test)] == 0);  

    // 1. Create a string, ensure deep copy
    FString a(L"Hello UE");
    std::wstring wide = a.ToWideString();
    CHECK(wide == L"Hello UE");

    // 2. Copy string and mutate original
    FString b = a;
    a = L"Changed";
    CHECK(b.ToWideString() == L"Hello UE");

    // 3. Assign from const char*
    FString c = "UTF8 ∑ ∞ 🎧";
    CHECK(c.ToWideString().find(L"🎧") != std::wstring::npos);

    // 4. Assign nullptr / empty
    FString d = nullptr;
    CHECK(d.ToWideString().empty());

    // 5. Reassign to self
    a = a;
    CHECK(a.ToWideString() == L"Changed");

    // 6. Inspect internal layout (simulate UE read)
    const wchar_t* raw = a.c_str();
    CHECK(raw != nullptr);
    CHECK(wcslen(raw) + 1 == a.length());

    // 7. Size consistency
    CHECK(a.size() >= a.length());
}
