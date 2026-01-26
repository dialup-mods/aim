#pragma once
#include <cstdint>
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UClassPropertyEntry final : public PropertyEntry, LayoutTraits<UClassProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    auto getType() const -> EClassTypes override { return EClassTypes::UClassProperty; }
    auto getCanonicalType() const -> std::string override { return "UClass*"; }
    auto getCacheType() const -> std::string override { return "UClassPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UClassProperty"; }
    bool isTriviallyCopyable() const override { return false; }
    bool canConst() const override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(uintptr_t); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
