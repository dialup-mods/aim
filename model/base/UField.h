#pragma once
#include <cstddef>
#include <string>

#include "Object.h"
#include "LayoutTraits.h"

class UFieldEntry : public ObjectEntry, LayoutTraits<UField, UObject> {
public:
    using ObjectEntry::ObjectEntry;

    static auto getBaseType() {
        return EClassTypes::UField;
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UField; }
    auto getCanonicalType() const -> std::string override { return "UField"; }
    auto getCacheType() const -> std::string override { return "UFieldEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UField"; }
};