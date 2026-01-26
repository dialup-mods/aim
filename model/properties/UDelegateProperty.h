#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "ValueResolver.h"
#include "resolve/DelegateResolver.h"

class UDelegatePropertyEntry final : public PropertyEntry, LayoutTraits<UDelegateProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    void resolveInto(ResolvedValue& out, void* valuePtr) const override {
        DelegateResolver::resolve(out, valuePtr);
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UDelegateProperty; }
    auto getCanonicalType() const -> std::string override { return "FScriptDelegate"; }
    auto getCacheType() const -> std::string override { return "UDelegatePropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UDelegateProperty"; }
    bool isTriviallyCopyable() const override { return false; }
    bool canConst() const override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(FScriptDelegate); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
