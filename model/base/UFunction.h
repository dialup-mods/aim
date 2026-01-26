#pragma once
#include <optional>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "fmt/ranges.h"

#include "FlagStrings.h"
#include "LayoutTraits.h"
#include "Object.h"
#include "Property.h"
#include "Schema.h"

class UFunctionEntry : public ObjectEntry, LayoutTraits<UFunction, UStruct> {
public:
    explicit UFunctionEntry(UObject* obj) : ObjectEntry(obj) {
        UFunctionEntry::iterateDependencies();
    }

    auto asFunction() const -> UFunction* {
        return static_cast<UFunction*>(getObject());
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UFunction; }
    auto getCanonicalType() const -> std::string override { return "UFunction"; }
    auto getCacheType() const -> std::string override { return "UFunctionEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UFunction"; }

    auto getFunctionFlags() -> uint64_t {
        if (!flags_) { flags_ = asFunction()->FunctionFlags; }
        return flags_;
    }

    auto isEvent() -> bool { return (getFunctionFlags() & FUNC_Event) != 0; }
    auto isStaticFunction() -> bool { return (getFunctionFlags() & FUNC_Static); }
    auto getMethodName() -> std::string { return isEvent() ? "event" + getSanitizedName() : getSanitizedName(); }
    auto getReturnName() const -> std::string { return returnParam_ ? returnParam_.value()->getSanitizedName() : ""; }
    auto getReturnType() const -> std::string {
        auto typ = returnParam_ ? returnParam_.value()->getCanonicalType() : "void";
        return typ == "SearchStatusOwner" ? "ESearchStatusOwner" : typ;
    }
    auto getEmitType() const -> std::string {
        if (returnParam_) {
            if (returnParam_.value()->getEmitType() != "<unknown>") {
                // fixme stupid hack, thanks psyonix
                if (returnParam_.value()->getEmitType() == "SearchStatusOwner") {
                    return "ESearchStatusOwner";
                }
                return returnParam_.value()->getEmitType();
            } else {
                return returnParam_.value()->getCanonicalType();
            }
        }
        return "void";
    }
    auto getFunctionName() const -> std::string { return getSanitizedName(); }

    auto getArguments() const -> std::vector<PropertyEntry*> { return arguments_; }
    auto getReturnParam() const -> std::optional<PropertyEntry*> { return returnParam_; }
    auto getOutParams() const -> std::vector<PropertyEntry*> { return outParams_; }
    auto getAllParams() const -> std::vector<PropertyEntry*> {
        // fixme sort by offset, if not already
        // std::sort(parms.begin(), parms.end(),
        // [](FProperty* a, FProperty* b)
        // {
        //     return a->Offset_Internal < b->Offset_Internal;
        // });
        std::vector<PropertyEntry*> params;

        auto args = getArguments();
        auto outs = getOutParams();

        params.insert(params.end(), args.begin(), args.end());
        params.insert(params.end(), outs.begin(), outs.end());

        if (auto retParam = getReturnParam()) {
            params.emplace_back(*retParam);
        }

        return params;
    }


    auto getParamKey() const -> std::string override {
        auto key = fmt::format("{}_{}_Params", getClassNameCPP(), getFunctionName());
        return key;
    }

    void emitClassMethods(FILE* file) {
        fmt::print(file, "    {}{} {}({});\n",
            isStaticFunction() ? "static " : "",
            getEmitType() == "SearchStatusOwner" ? "ESearchStatusOwner" : getEmitType(),
            getMethodName(),
            fmt::join(getFormattedArgs(), ", ")
        );
    }

    void emitFunctionParamsStruct(FILE* file) const {
        if (getFunctionName().starts_with("_")) {
            return;
        }
        //Logger::log("Generating params struct for {} with {} arguments", getFunctionName(), arguments_.size());
        //Logger::log("UFunctionEntry emit called at {}", (void*)this);
        //for (const auto& arg : arguments_) {
        //    Logger::log(" - {} {}", arg->getClassNameCPP(), arg->getNameCPP());
        //}
        // annoying mode
        //fmt::print(file, "// {}\n", getFullName());

        fmt::print(file, "struct {} {{\n", getParamKey());

        if (!arguments_.empty()) {
            std::map<std::string, uint32_t> nameMap;

            for (const PropertyEntry* prop : arguments_) {
                const auto flagStr = FlagStrings::getInterestingPropertyFlagsString(prop->getPropertyFlags());
                auto argName = prop->getSanitizedName();

                if (nameMap.contains(argName)) {
                    argName += std::to_string(nameMap[argName]++);
                } else {
                    nameMap[argName] = 1;
                }

                fmt::print(file,
                    "    {} {};{}\n",
                    prop->getCanonicalType() == "SearchStatusOwner" ? "ESearchStatusOwner" : prop->getCanonicalType(),
                    argName,
                    flagStr.empty() ? "" : fmt::format(" // {}", flagStr)
                );
            }
        }

        // fixme, not checking if name was emitted with arguments for renaming
        if (getReturnParam()) {
            const auto flagStr = FlagStrings::getInterestingPropertyFlagsString(getReturnParam().value()->getPropertyFlags());
            fmt::print(file,
                "    {} ReturnValue;{}\n",
                getReturnType(),
                flagStr.empty() ? "" : fmt::format(" // {}", flagStr)
            );
        }

        fmt::print(file, "}};\n\n");
    }

    auto getFormattedArgs() -> std::vector<std::string> {
        std::vector<std::string> formattedArgs;
        std::map<std::string, uint32_t> nameMap;

        for (const PropertyEntry* arg : arguments_) {
            auto argName = arg->getSanitizedName();
            if (nameMap.contains(argName)) {
                argName += std::to_string(nameMap[argName]++);
            } else {
                nameMap[argName] = 1;
            }
            formattedArgs.push_back(fmt::format("{} {}", arg->getFormattedArgType(), argName));
        }

        for (const PropertyEntry* out : outParams_) {
            auto argName = out->getSanitizedName();
            if (nameMap.contains(argName)) {
                argName += std::to_string(nameMap[argName]++);
            } else {
                nameMap[argName] = 1;
            }
            formattedArgs.push_back(fmt::format("{}& {}", out->getFormattedArgType(), argName));
        }

        return formattedArgs;
    }

    void emitImplementation(FILE* file) {
        //fmt::print(file, "// {}\n", getFullName());
        if (getClassNameCPP().empty()) {
            fmt::print(file, "// skipping: {}\n", getFullName());
            return;
        }

        const std::string paramVar = "params";

        UObject* outer = getObject()->Outer;

        if (outer && r::types::conformsTo(outer, r::uclass::find("Class Core.State"))) {
            UObject* stateOuter = outer->Outer;
            if (stateOuter && r::types::conformsTo(stateOuter, r::uclass::find("Class Core.Class"))) {
                if (stateOuter->GetName() != "Default") {
                    fmt::print(file, "// State-scoped UnrealScript function (no callable wrapper)\n\n");
                    return;
                }
            }
        }

        // signature
        fmt::print(file, "SDK_API {} {}::{}({}) {{\n",
            getReturnType(),
            getClassNameCPP(),
            getMethodName(),
            fmt::join(getFormattedArgs(), ", ")
        );

        //fmt::print(file, "    static UFunction* fn = Runtime::findFunction(\"{}\");\n", getFullName());

        fmt::print(file, "    {} {}{{}};\n", getParamKey(), paramVar);

        // NOTE:
        // Input params may be written via memcpy at reflected offsets.
        // Return values MUST be written by the engine and read from
        // fn->ReturnValueOffset. Do NOT memcpy or construct ReturnValue.
        // I.E.:
        // You must allocate the params struct with the correct size and layout.
        // You must NOT construct or assign the return value slot.
        for (const PropertyEntry* arg : arguments_) {
            const auto& name = arg->getSanitizedName();
            if (arg->isTriviallyCopyable()) {
                fmt::print(file, "    memcpy_s(&{}.{}, sizeof({}.{}), &{}, sizeof({}));\n",
                    paramVar, name, paramVar, name, name, name
                );
            } else {
                fmt::print(file, "    {}.{} = {};\n", paramVar, name, name);
            }
        }

        // Repeat, you must NOT construct or assign the return value slot.
        // do not be tempted to add it here

        fmt::print(file
            , "    r::process_event::call(\n        {}\n        , r::ufunction::find(\"{}\")\n        , &params\n    );\n"
            , isStaticFunction() ? "r::uclass::find(className)" : "this"
            , getFullName()
        );

        if (returnParam_.has_value()) { fmt::print(file, "    return {}.{};\n", paramVar, getReturnName()); }

        fputs("}\n\n", file);
    }

private:
    uint64_t flags_{0};
    std::vector<PropertyEntry*> arguments_;
    std::vector<PropertyEntry*> outParams_;
    std::optional<PropertyEntry*> returnParam_;
    std::vector<ObjectEntry*> weirdOrphans_;
    std::set<UFunctionEntry*> methods_;
    std::set<ObjectEntry*> orphans_;
};