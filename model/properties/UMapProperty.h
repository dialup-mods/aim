#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UMapPropertyEntry final : public PropertyEntry, LayoutTraits<UMapProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    auto getType() const -> EClassTypes override { return EClassTypes::UMapProperty; }

    auto getCacheType() const -> std::string override { return "UMapPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UMapProperty"; }
    auto isTriviallyCopyable() const -> bool override { return false; }
    auto canConst() const -> bool override { return false; }
};