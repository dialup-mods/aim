#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "ValueResolver.h"
#include "resolve/FNameResolver.h"

class UNamePropertyEntry final : public PropertyEntry, LayoutTraits<UNameProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    void resolveInto(ResolvedValue& out, void* valuePtr) const override {
        FNameResolver::resolve(out, valuePtr);
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UNameProperty; }
    auto getCanonicalType() const -> std::string override { return "FName"; }
    auto getCanonicalTypeStr() const -> std::string override { return "FName"; }
    auto getCacheType() const -> std::string override { return "UNamePropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UNameProperty"; }
    bool isTriviallyCopyable() const override { return false; }
    bool canConst() const override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(FName); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
