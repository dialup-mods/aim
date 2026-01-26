#pragma once
#include <string>

#include "Object.h"
#include "LayoutTraits.h"

class FNameEntryEntry: public ObjectEntry, LayoutTraits<FNameEntry, FName> {
public:
    using ObjectEntry::ObjectEntry;

    auto getType() const -> EClassTypes override { return EClassTypes::FNameEntry; }
    auto getDefaultClassName() const -> std::string override { return "FNameEntry"; }
    auto getCanonicalType() const -> std::string override { return "FNameEntry"; }
    auto getCacheType() const -> std::string override { return "FNameEntry"; }
};