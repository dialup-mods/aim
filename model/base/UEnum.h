#pragma once
#include <mutex>
#include <string>

#include "ConfigManager.h"
#include "Object.h"

class EnumEntry final : public ObjectEntry, LayoutTraits<UEnum, UField> {
public:
    using ObjectEntry::ObjectEntry;

    static auto getBaseType() {
        return EClassTypes::UEnum;
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UEnum; }
    auto getCacheType() const -> std::string override { return "EnumEntry"; }
    auto getCanonicalType() const -> std::string override { return "UEnum"; }
    auto getDefaultClassName() const -> std::string override { return "UEnum"; }
    static auto prefixWithClass() -> bool {
        // thread-safe cache. not using threads currently (at time of writing anyway) but would be nice
        static bool cached = false;
        return cached;
    }

    void emit(FILE* file, const std::string& package) override {}
};