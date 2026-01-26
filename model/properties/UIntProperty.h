#pragma once
#include <cstdint>
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "ValueResolver.h"
#include "resolve/IntegerResolver.h"

class UIntPropertyEntry final : public PropertyEntry, LayoutTraits<UIntProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    void resolveInto(ResolvedValue& out, void* valuePtr) const override {
        IntegerResolver::resolve(out, valuePtr);
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UIntProperty; }
    auto getCanonicalType() const -> std::string override { return "int32_t"; }
    auto getCanonicalTypeStr() const -> std::string override { return "int32_t"; }
    auto getCacheType() const -> std::string override { return "UIntPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UIntProperty"; }
    auto isTriviallyCopyable() const -> bool override { return true; }
    auto canConst() const -> bool override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(int32_t); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
