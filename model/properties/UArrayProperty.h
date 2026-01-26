#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "UScriptStruct.h"
#include "UStruct.h"
#include "UStructProperty.h"

class UArrayPropertyEntry final : public PropertyEntry, LayoutTraits<UArrayProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    auto getType() const -> EClassTypes override { return EClassTypes::UArrayProperty; }

    auto getCacheType() const -> std::string override { return "UArrayPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UArrayProperty"; }

    auto canConst() const -> bool override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(TArray<uintptr_t>); }
};