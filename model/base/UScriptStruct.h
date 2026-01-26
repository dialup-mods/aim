#pragma once
#include <string>

#include "LayoutTraits.h"
#include "StructLike.h"
#include "Schema.h"

class UScriptStructEntry : public StructLikeEntry, LayoutTraits<UStruct, UField> {
public:
    explicit UScriptStructEntry(UObject* obj) : StructLikeEntry(obj) {}

    auto getType() const -> EClassTypes override { return EClassTypes::UScriptStruct; }
    auto getCacheType() const -> std::string override { return "ScriptStructEntry"; }
    auto getCanonicalType() const -> std::string override { return "UScriptStruct"; }
    auto getDefaultClassName() const -> std::string override { return "UScriptStruct"; }

private:
    UStructEntry* superFieldEntry_{nullptr};
    std::unordered_set<std::string> deps_;
};