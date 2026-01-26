#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "ValueResolver.h"
#include "resolve/QWordResolver.h"

// Semantics: numeric value, not pointer, not flags, not name
// exists mostly for:
//   large counters
//   timestamps
//   bit-packed values
//   legacy script exposure
class UQWordPropertyEntry final : public PropertyEntry, LayoutTraits<UInterfaceProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    void resolveInto(ResolvedValue& out, void* valuePtr) const override {
        IntegerResolver::resolve(out, valuePtr);
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UQWordProperty; }
    auto getCanonicalType() const -> std::string override { return "uint64_t"; }
    auto getCanonicalTypeStr() const -> std::string override { return "int64_t"; }
    auto getCacheType() const -> std::string override { return "UQWordPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UQWordProperty"; }
    bool isTriviallyCopyable() const override { return true; }
    bool canConst() const override { return false; }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
