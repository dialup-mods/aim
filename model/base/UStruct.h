#pragma once
#include <string>

#include "fmt/format.h"

#include "FlagStrings.h"
#include "LayoutTraits.h"
#include "Logger.h"
#include "Object.h"
#include "Property.h"
#include "StructLike.h"
#include "StructWalker.h"

class UStructEntry final
  : public StructLikeEntry, LayoutTraits<UStruct, UField> {
public:
    explicit UStructEntry(UObject* obj) : StructLikeEntry(obj) {
        //UStructEntry::iterateDependencies();
        //UStructEntry::createSuperFieldEntryMaybe();
        //walkChildren(asStruct(), UStructEntry::getFullName());
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UStruct; }
    auto getCacheType() const -> std::string override { return "StructEntry"; }
    auto getCanonicalType() const -> std::string override { return "UStruct"; }
    auto getDefaultClassName() const -> std::string override { return "UStruct"; }

//    auto getDependencyTypes() -> std::vector<std::string> override {
//        const auto baseStructName = getResolvedBaseStructName();
//        if (!baseStructName.empty()) {
//            deps_.insert(baseStructName);
//        }
//
//        for (auto* p : properties_) {
//            auto fieldDeps = p->getDependencyTypes();  // delegate to field entries
//            deps_.insert(deps_.end(), fieldDeps.begin(), fieldDeps.end());
//        }
//
//        return deps_;
//    }

    auto deps() -> const std::unordered_set<std::string>& override {
        return EMPTY_STR_SET;
//        if (depsBuilt_) { return deps_; }
//
//        const auto fullName = getFullName();
//
//        const UStruct* uStruct = asStruct();
//        if (!uStruct) {
//            Logger::instance().log("[WARNING] uStruct null");
//            return {};
//        };
//
//        if (const auto* superField = getSuperFieldEntry()) {
//            deps_.insert(superField->getFullName());
//        }
//
//        for (UField* field = uStruct->Children; field; field = field->Next) {
//            const auto cached = ObjectStore::instance().add(field, getFullName());
//
//            if (field->IsA<UProperty>()) {
//                if (const auto* prop = cached->as<PropertyEntry>()) {
//                    if (prop->isValidProperty()) {
//                        auto structDeps = prop->getStructDependencyTypes();
//                        // properties for dependency sorting
//                        deps_.insert(structDeps.begin(), structDeps.end());
//
//                        // properties for emission
//                        properties_.insert(cached->as<PropertyEntry>());
//                    }
//                }
//            } else {
//                Logger::instance().log("[WARNING] skipping b/c not PropertyEntry:\n{}", cached->asString());
//            }
//        }
//
//        depsBuilt_ = true;
//
//        return deps_;
    }

    //auto deps() -> const std::vector<UStructEntry*>& {
    //    if (depsBuilt_) { return deps_; }

    //    if (auto* sup = getSuperFieldEntry()) {
    //        deps_.emplace_back(static_cast<UStructEntry*>(sup));
    //    }

    //    for (auto* p : properties_) {
    //        if (auto* sp = p->as<UStructPropertyEntry>()) {
    //            if (auto* tgt = sp->getStructEntry()) deps_.emplace_back(tgt);
    //        } else if (auto* pe = p->as<UStructEntry>()) {
    //            deps_.emplace_back(pe);
    //        }
    //    }
    //    // Dedupe

    //    std::unordered_set<std::string> seen;
    //    std::erase_if(deps_, [&seen](const auto* dep) { return !seen.insert(dep->getFullName()).second; });
    //    depsBuilt_ = true;

    //    return deps_;
    //}

    void walkChildren(UStruct* uStruct, const std::string& origin) {
//        for (UField* field = asStruct()->Children; field; field = field->Next) {
//            auto cached = ObjectStore::instance().add(field, origin);
//            if (!cached || cached->wasIterated()) { continue; }
//
//            if (cached->getObject()->IsA<UProperty>() && cached->as<PropertyEntry>()->isValidProperty() ) {
//                if (auto* structProperty = cached->as<UStructPropertyEntry>()) {
//                    // getStructEntryAsObjectEntry calls `ObjectStore::add()` so we don't need to do it here
//                    if (auto* targetStruct = structProperty->getStructEntryAsObjectEntry()) {
//                        //deps_.insert(targetStruct->getFullName());
//                        walkChildren(static_cast<UStruct*>(targetStruct->getObject()), origin);
//                    }
//                }
//                auto structDeps = cached->getStructDependencyTypes();
//                //deps_.insert(structDeps.begin(), structDeps.end());
//
//            } else {
//                Logger::instance().log("[WARNING] Child is ??. Origin: {}", origin);
//            }
//            cached->markIterated();
//        }
    }

    void iterateDependencies() override {
        Logger::instance().log("[WARNING] not implemented on base UStruct");
    //    if (wasIterated()) return;
    //    const auto fullName = getFullName();
    //    const UStruct* uStruct = asStruct();
    //    if (!uStruct) return;

    //    if (auto* super = uStruct->SuperField; super && super != uStruct) {
    //        if (super->IsA<UStruct>()) {
    //            ObjectStore::instance().add(super, fullName);
    //        } else {
    //            Logger::instance().log("[WARNING] SuperField is not UStruct for {}: {}",
    //                                 fullName, super->GetClass()->GetName());
    //        }
    //    }
    //    if (auto* super = uStruct->SuperField; super && super != uStruct) {
    //        ObjectStore::instance().add(super, fullName);
    //    }
    //    markIterated();
    }

//    void iterateProperties() const {
//        for (auto* uProperty = static_cast<UProperty*>(asStruct()->Children); uProperty;
//             uProperty = static_cast<UProperty*>(uProperty->Next)) {
//            if (uProperty->ElementSize > 0 && !uProperty->IsA<UScriptStruct>()) {
//                if (auto* cached = ObjectStore::instance().add(uProperty, getFullName())->as<PropertyEntry>()) {
//                    if (cached->isValidProperty()) { properties_.insert(cached); }
//                }
//            }
//        }
//    }

private:
    UStructEntry* superFieldEntry_{nullptr};
    std::unordered_set<std::string> deps_;
};