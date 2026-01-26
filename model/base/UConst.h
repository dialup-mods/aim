#pragma once
#include <string>

#include "fmt/format.h"

#include "Object.h"
#include "LayoutTraits.h"

class ConstEntry final : public ObjectEntry, LayoutTraits<UConst, UField> {
public:
    using ObjectEntry::ObjectEntry;

    static auto getBaseType() {
        return EClassTypes::UConst;
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UConst; }
    auto getCanonicalType() const -> std::string override { return "UConst"; }
    auto getCacheType() const -> std::string override { return "ConstEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UConst"; }

    auto asConst() const -> UConst* {
        return static_cast<UConst*>(getObject());
    }

    void emit(FILE* file, const std::string& package) override {
        fmt::print(file
            , "#define {} {}\n"
            , getNameCPP()
            , asConst()->Value.ToString()
        );
    };
};