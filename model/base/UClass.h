#pragma once
#include <string>
#include <vector>
#include <map>

#include "fmt/format.h"

#include "Object.h"
#include "Property.h"
#include "StructLike.h"
#include "UFunction.h"
#include "UStruct.h"
#include "UStructProperty.h"
#include "Runtime.h"

using r = Runtime;

class ClassEntry final : public StructLikeEntry {
public:
    explicit ClassEntry(UObject* obj) : StructLikeEntry(obj) {
        iterateDependencies();
        iterateProperties();
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UClass; }
    auto getCanonicalType() const -> std::string override { return "class"; }
    auto getCacheType() const -> std::string override { return "ClassEntry"; }

    auto getSuperFieldAsClass() const -> UClass* {
        return static_cast<UClass*>(asStruct()->SuperField);
    }

    bool isUObjectDerived() const {
        for (UStruct* cur = asStruct(); cur; cur = static_cast<UStruct*>(cur->SuperField)) {
            if (cur == UObjectClass()) {
                return true;
            }
        }
        return false;
    }

    auto resolveCppBase() const -> UClass* {
        UClass* super = getSuperFieldAsClass();

        if (super && super != asClass()) {
            return super;
        }

        if (isUObjectDerived()) {
            return UObjectClass();
        }

        return nullptr;
    }

    auto getMinAlignment() const -> ptrdiff_t override { return asStruct() ? asStruct()->MinAlignment : 0; }
    auto getPropertySize() const -> ptrdiff_t override {
        return asStruct() ? asStruct()->PropertySize : 0;
    }

    auto getSortedMethods() const -> std::vector<UFunctionEntry*> {
        std::vector out(methods_.begin(), methods_.end());
        std::ranges::sort(out, [](auto* a, auto* b) {
            return a->getName() < b->getName();
        });
        return out;
    }

    auto getMethodNames() const -> std::unordered_set<std::string> {
        std::unordered_set<std::string> out;
        for (const auto* fn : methods_) {
            out.insert(fn->getNameCPP());
        }
        return out;
    }

    void emit(FILE* file, const std::string& package) override {};

    void emitClassName(FILE* file) const {
        fmt::print(file, "    static constexpr auto className = \"{}\";\n", getFullName());
    }

    void emitMethods(FILE* file) const {
        const auto sortedMethods = getSortedMethods();
        if (!sortedMethods.empty()) { fputs("\n", file); }

        for (UFunctionEntry* fn : sortedMethods) {
            fn->emitClassMethods(file);
        }
    }

    // fixme not needed methinks
    //void emitFindFunctionDef(FILE* file) const {
    //    if (asClass() == UObject::StaticClass()) {
    //        if (ConfigManager::instance().getProcessEventMethod() == "vtable") {
    //            fprintf(file, "    void ProcessEvent(UFunction* uFunction, void* uParams, void* uResult = nullptr);\n");
    //        // fixme?
    //        //} else if (GConfig::GetProcessEventIndex() != -1) {
    //        //    //GenerateVirtualFunctions(file);
    //        }
    //        //fprintf(file
    //        //    , "    static UFunction* SDK::findFunction(const std::string& %s);\n"
    //        //    , getFullName().c_str()
    //        //);
    //    }
    //}

    void emitStaticClasses(FILE* file) const {
        fputs("    static UClass* StaticClass() {\n", file);
        // fixme
        //if (GConfig::UsingConstants()) {
        //    fprintf(file, "return reinterpret_cast<UClass*>(UObject::GObjObjects()->at(%s));",
        //        GCache::GetConstant(unrealObj).first.c_str());
        //} else {
            fmt::print(file, "return r::uclass::find(\"{}\");",
                getFullName()
            );
        //}
        fputs("}\n", file);
    }

    void emitClose(FILE* file) const {
        fputs("};\n\n", file);
    }


private:
    UStruct* uStruct_{nullptr};
    std::unordered_set<UFunctionEntry*> methods_;
    std::unordered_set<ObjectEntry*> orphans_;
};