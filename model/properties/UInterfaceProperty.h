#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UInterfacePropertyEntry final : public PropertyEntry, LayoutTraits<UInterfaceProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    static auto getBaseType() {
        return EClassTypes::UInterfaceProperty;
    }

    //[[nodiscard]] auto getCanonicalType() const -> std::string override {
    //    if (const auto* interfaceProperty = static_cast<UInterfaceProperty*>(getObject())) {
    //        if (interfaceProperty->InterfaceClass) {
    //            return interfaceProperty->InterfaceClass->GetNameCPP();
    //        }
    //    }
    //    return "void*"; // fallback if not resolvable
    //}

    auto getType() const -> EClassTypes override { return EClassTypes::UInterfaceProperty; }
    auto getCanonicalType() const -> std::string override {
        if (const auto* interfaceProperty = static_cast<UInterfaceProperty*>(getObject())) {
            if (interfaceProperty->InterfaceClass) {
                // Return as FScriptInterface (UE3's interface wrapper type)
                return "FScriptInterface";
                // OR if you want the specific type:
                // return "class " + interfaceProperty->InterfaceClass->GetNameCPP() + "*";
            }
        }
        return "FScriptInterface"; // fallback
    }
    auto getCacheType() const -> std::string override { return "UInterfacePropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UInterfaceProperty"; }
    bool isTriviallyCopyable() const override { return false; }
    bool canConst() const override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(uintptr_t); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
