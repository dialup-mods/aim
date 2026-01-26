#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "ValueResolver.h"
#include "resolve/BooleanResolver.h"

class UBoolPropertyEntry final : public PropertyEntry, LayoutTraits<UBoolProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    void resolveInto(ResolvedValue& out, void* valuePtr) const override {
        BooleanResolver::resolve(out, valuePtr, getBitMask());
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UBoolProperty; }
    auto getCanonicalType() const -> std::string override { return "uint32_t"; }
    auto getCanonicalTypeStr() const -> std::string override { return "uint32_t"; }
    auto getCacheType() const -> std::string override { return "UBoolPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UBoolProperty"; }
    bool isTriviallyCopyable() const override { return false; }
    bool canConst() const override { return false; }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
