#pragma once
#include <string>

#include "Object.h"
#include "LayoutTraits.h"

class UObjectEntry final : public ObjectEntry, LayoutTraits<UObject, UObject> {
public:
    using ObjectEntry::ObjectEntry;

    static auto getBaseType() {
        return EClassTypes::UObject;
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UObject; }
    std::string getDefaultClassName() const override { return "UObject"; }
    auto getCacheType() const -> std::string override { return "UObjectEntry"; }
};