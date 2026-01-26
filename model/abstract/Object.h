#pragma once

#if defined(SDK_BUILD)
#define SDK_API __declspec(dllexport)
#else
#define SDK_API __declspec(dllimport)
#endif

#include <mutex>
#include <optional>
#include <set>
#include <string>

#include "ConfigManager.h"
#include "EClassTypes.h"
#include "Runtime.h"
#include "Schema.h"
#include "TypeRules.h"
#include "ValueResolver.h"
#include "SDK.h"

using obj_pool = Runtime::uobject::game_pool;
using r = Runtime;

class ObjectEntry {
public:
    virtual ~ObjectEntry() = default;

    explicit ObjectEntry(UObject* unrealObj, const char* origin = "") {
        uObject_ = unrealObj;
        origin_ = origin;
    }

    static auto UObjectClass() -> UClass* {
        static UClass* cached = r::uclass::find("Class Core.Object");
        return cached;
    }

    virtual void resolveInto(ResolvedValue& out, void* valuePtr) const {}

    virtual auto getType() const -> EClassTypes {
        if (type_ != EClassTypes::Unresolved) {
            return type_;
        }
        return EClassTypes::Unresolved;
    }

    //if (getObject()->IsA<UByteProperty>())           type_ = EClassTypes::UByteProperty;
    //else if (getObject()->IsA<UIntProperty>())       type_ = EClassTypes::UIntProperty;
    //else if (getObject()->IsA<UBoolProperty>())      type_ = EClassTypes::UBoolProperty;
    //else if (getObject()->IsA<UFloatProperty>())     type_ = EClassTypes::UFloatProperty;
    //else if (getObject()->IsA<UNameProperty>())      type_ = EClassTypes::UNameProperty;
    //else if (getObject()->IsA<UStrProperty>())       type_ = EClassTypes::UStrProperty;
    //else if (getObject()->IsA<UQWordProperty>())     type_ = EClassTypes::UQWordProperty;
    //else if (getObject()->IsA<UArrayProperty>())     type_ = EClassTypes::UArrayProperty;
    //else if (getObject()->IsA<UMapProperty>())       type_ = EClassTypes::UMapProperty;
    //else if (getObject()->IsA<UDelegateProperty>())  type_ = EClassTypes::UDelegateProperty;
    //else if (getObject()->IsA<UStructProperty>())    type_ = EClassTypes::UStructProperty;
    //else if (getObject()->IsA<UClassProperty>())     type_ = EClassTypes::UClassProperty;
    //else if (getObject()->IsA<UObjectProperty>())    type_ = EClassTypes::UObjectProperty;
    //else if (getObject()->IsA<UInterfaceProperty>()) type_ = EClassTypes::UInterfaceProperty;

    //// Structs (most specific first)
    //else if (getObject()->IsA<UFunction>())          type_ = EClassTypes::UFunction;
    //else if (getObject()->IsA<UScriptStruct>())      type_ = EClassTypes::UScriptStruct;
    //else if (getObject()->IsA<UClass>())             type_ = EClassTypes::UClass;
    //else if (getObject()->IsA<UStruct>())            type_ = EClassTypes::UStruct;  // Must be after Function/ScriptStruct/Class

    //// Other
    //else if (getObject()->IsA<UEnum>())              type_ = EClassTypes::UEnum;
    //else if (getObject()->IsA<UConst>())             type_ = EClassTypes::UConst;

    //    else type_ = EClassTypes::Unknown;

    //    return type_;
    //}

    auto fullName() const -> std::string {
        std::string fullName = this->getName();

        for (const UObject* uOuter = uObject_->Outer; uOuter; uOuter = uOuter->Outer) {
            fullName = (uOuter->GetName() + "." + fullName);
        }

        fullName = (uObject_->Class->GetName() + " " + fullName);
        return fullName;
    }

//    auto getNameCPP() -> std::string {
//        std::string nameCPP;
//
//        if (strcmp(uObject_->baseClassName.data(), "Class Core.Class") == 0) {
//            auto uClass = reinterpret_cast<UClass*>(this);
//
//            while (uClass) {
//                if (std::string clName = uClass->GetName(); clName == "Actor") {
//                    nameCPP += "A";
//                    break;
//                } else if (clName == "Object") {
//                    nameCPP += "U";
//                    break;
//                }
//
//                uClass = reinterpret_cast<UClass*>(uClass->SuperField);
//            }
//        } else {
//            nameCPP += "F";
//        }
//
//        nameCPP += uObject_->GetName();
//        return nameCPP;
//    }

    auto getClassNameCPP() const -> std::string {
        if (UObject* outer = getObject()->Outer; outer && outer->Class->GetFullName() == "Class Core.Class") {
            return outer->GetNameCPP();
        }
        return "";
    }

    inline static const std::unordered_set<std::string> EMPTY_STR_SET;

    auto hasAnyFlags(const EObjectFlags Flags) const -> bool {
        return (uObject_->ObjectFlags & Flags) != 0;
    }

    auto hasAllFlags(const EObjectFlags Flags) const -> bool {
        return (uObject_->ObjectFlags & Flags) == Flags;
    }

    auto getPackageObj() const -> UObject* {
        UObject* uPackage = nullptr;
        for (UObject* uOuter = uObject_->Outer; uOuter; uOuter = uOuter->Outer) {
            uPackage = uOuter;
        }
        return uPackage;
    }

    virtual void iterateDependencies() {};
    virtual void iterateProperties() {};

    void markIterated() { iterated_ = true; }
    void markChildrenIterated() { childrenIterated_ = true; }
    virtual auto wasIterated() const -> bool { return iterated_; }
    virtual auto wereChildrenIterated() const -> bool { return childrenIterated_; }

    virtual auto isOptional()          const -> bool { return false; }
    virtual auto isOutParam()          const -> bool { return false; }
    virtual auto isReturnParam()       const -> bool { return false; }
    virtual auto isArgument()          const -> bool { return false; }
    virtual auto isTriviallyCopyable() const -> bool { return false; }

    virtual auto getEmitType(const std::string& currentPackage = "") const -> std::string& { return emitTypeStr_; };
    virtual auto getCanonicalType() const -> std::string { return "<unknown>"; }
    virtual auto getCanonicalTypeStr() const -> std::string { return "<unknown>"; }
    virtual auto getDefaultClassName() const -> std::string { return "<unknown>"; }
    virtual auto getCacheType() const -> std::string { return "CacheEntry"; }

    static auto getGameAlignment() -> uint64_t {
        static uint64_t cached = -1;

        return cached;
    }

    template<typename T>
    T* as() {
        return dynamic_cast<T*>(this);
    }

    virtual void emitForwardDeclaration(FILE* file) {};
    virtual void emit(FILE* file, const std::string& package) {};

    // clang-format off
    static std::string createUnknownDataFieldStr(
        const ptrdiff_t index
        , const ptrdiff_t offset
        , const ptrdiff_t size
        , const std::string& reason
    ) {
        if (size <= 0 || size < getGameAlignment()) return "";

        return fmt::format(
            "    uint8_t UnknownData{0:03}[0x{1:08X}]; // Offset: 0x{2:08X} | Size: 0x{3:08X} | {4}\n"
            , index, size, offset, size, reason
        );
    }
    // clang-format on

    static void printUnknownDataField(
        FILE* file
        , const ptrdiff_t index
        , const ptrdiff_t offset
        , const ptrdiff_t size
        , const std::string& reason
    ) {
        fputs(createUnknownDataFieldStr(index, offset, size, reason).c_str(), file);
    }

    void addReferrer(std::string from) {
        referrers_.emplace_back(std::move(from));
    }

    const auto& getReferrers() const {
        return referrers_;
    }

    virtual auto getPackage() const -> UObject* {
        if (!package_) {
            package_ = uObject_->GetPackageObj();
        }
        return package_;
    }

    virtual auto getPackageName() const -> std::string {
        if (packageName_.empty()) {
            if (auto* package = getPackage()) {
                packageName_ = package->GetName();
            }
        }
        return packageName_;
    }

    virtual auto getOuter() const -> UObject* {
        return getObject()->Outer;
    }

    [[nodiscard]] virtual auto getGroupName() const -> std::string {
        const UObject* topOuter = getObject();
        while (topOuter && topOuter->Outer) {
            topOuter = topOuter->Outer;
        }
        return topOuter ? topOuter->GetName() : "<unknown>";
    }

    auto getOrigin() const -> std::string { return origin_; }
    void setOrigin(const std::string& origin) { origin_ = origin; }

    virtual auto isOverridden() const -> bool { return isOverridden_; }
    virtual void setOverridden() { isOverridden_ = true; }
    virtual void setOverwriteName(const std::string& newName) { overwriteName_ = newName; }
    virtual auto getOverwriteName() const -> std::string { return overwriteName_; }

    virtual auto isBlacklisted() const -> bool {
        if (!blacklistChecked_) {
            if (TypeRules::instance().isBlacklisted(getNameCPP())) {
                isBlacklisted_ = true;
            }
            blacklistChecked_ = true;
        }
        return isBlacklisted_;
    }

    // todo: invalidate cache if uObject_ isn't valid
    auto isValid() const -> bool {
        // ReSharper disable once CppDFAConstantConditions too dumb to understand cache
        // fixme
        // proper validation && (getType() != EClassTypes::Unresolved);
        return getObject();
    }

    // todo: invalidate cache if uObject_ isn't valid
    auto getObject() const -> UObject* {
        //if (!uObject_ || uObject_->ObjectFlags & (RF_PendingKill | RF_Transient)) {
        //    Logger::instance().log("getObject() invalid object {}", getName());
        //    return nullptr;
        //}
        return uObject_;
    }

    virtual auto getName() const -> std::string {
        if (name_.empty()) {
            if (getObject() && getObject()->Name.IsValid()) {
                name_ = getObject()->Name.ToString();
            }
        }
        return name_;
    }

    virtual auto getFullName() const -> std::string {
        if (!getObject()->Class || !getObject()->Outer || !getObject()->Name.IsValid()) {
            return "<incomplete>";
        }
        if (getObject()->Outer && getObject()->Class) {
            fullName_ = getObject()->GetFullName();
            return fullName_;
        }
        return getObject()->Name.ToString();
    }

    //auto classify(const UObject* obj) -> EClassTypes {
    //    for (auto* cls = obj->Class; cls; cls = static_cast<UClass*>(cls->SuperField)) {
    //        auto name = r::uclass::name(cls); // string_view
    //        if (auto it = kClassKind.find(name); it != kClassKind.end()) {
    //            return it->second;
    //        }
    //    }
    //    return EClassTypes::Unknown;
    //}

    auto hasChildren() const -> bool {
        auto* obj = getObject();
        if (Runtime::types::inheritsFrom(obj->Class, Runtime::uclass::find("Core.Struct"))) {
            return static_cast<UStruct*>(obj)->Children != nullptr;
        }
        return false;
    }

    // fixme probably maybe should make this pure virtual
    virtual auto deps() -> const std::unordered_set<std::string>& { return EMPTY_STR_SET; }

    virtual auto getNameCPP() const -> std::string {
        if (nameCPP_.empty()) {
            std::string name = getObject()->GetNameCPP();
            // fixme -- add legacy stupid naming option
            // std::string name = getObject()->GetNameCPP();
            if (name.empty()) {
                nameCPP_ = "<unnamed_" + getCacheType() + ">";
            } else {
                nameCPP_ = name;
            }
        }
        return nameCPP_;
    }

    virtual auto getOuterNameCPP() const -> std::string {
        if (!outerNameCPP_.empty()) return outerNameCPP_;

        if (UObject* outer = getOuter(); outer && outer->Name.IsValid()) {
            outerNameCPP_ = outer->GetNameCPP();
        } else {
            outerNameCPP_ = "<no_outer_cpp>";
        }
        return outerNameCPP_;
    }

    virtual auto getOuterName() const -> std::string {
        if (outerName_.empty()) {
            if (getOuter()) {
                outerName_ = getOuter()->GetName();
            } else {
                outerName_ = "<no_outer>";
            }
        }
        return outerName_;
    }

    void setOverrideEmitName(const std::string& name) const {
        printf("[INFO] calling setOverrideEmitName: %s\n", name.c_str());

        if (name == getSanitizedName()) {
            printf("[WARN] requested override name is the same as the original name. Not setting\n");
            return;
        }
        if (name.empty()) {
            printf("[WARN] requested override name is empty. Not setting\n");
            return;
        }
        overrideEmitName_ = name;
    }

    auto getOverrideEmitName() const -> std::optional<std::string> {
        return overrideEmitName_;
    }

    void setAlternateEmitName(const std::string& name) const {
        printf("[INFO] calling setAlternateEmitName: %s\n", name.c_str());
        if (name == getSanitizedName()) {
            printf("[WARN] requested alternate name is the same as the original name. Not setting\n");
            return;
        }
        if (name.empty()) {
            printf("[WARN] requested alternate name is empty. Not setting\n");
            return;
        }
        alternateEmitName_ = name;
    }

    auto getAlternateEmitName() const -> std::optional<std::string> {
        return alternateEmitName_;
    }

    virtual auto
    getSanitizedName() const -> std::string {
        if (sanitizedName_.empty()) {
            // fixme fixme this is probably where overrides should live
            // fixme maybe getNameCPP ?
            const std::string& nameCopy = getName();
            sanitizedName_ = sanitizeStr(nameCopy);
            // fixme ?
            //if (ConfigManager::instance().getPlatform() == "Windows" && (getType() == EClassTypes::UFunction)) {
            //    for (const std::string& unsafe : unsafeNames_) {
            //        if (sanitizedName_.find(unsafe) != std::string::npos) {
            //            sanitizedName_ += "W";
            //            break;
            //        }
            //    }
            //}
        }
        return sanitizedName_;
    }

    static auto sanitizeStr(std::string str) -> std::string {
        if (str.empty()) { return ""; }

        // fixme use unordered set
        static constexpr std::string_view unsafeChars = "!@#$%^&*()-=+[]{}\\|;:'\",/?`";

        for (char& c : str) {
            if (unsafeChars.find(c) != std::string_view::npos) {
                c = '_';
            }
        }

        if (str == "_") { str = "instance"; }

        return str;
    }

    [[nodiscard]] auto asString() -> std::string {
        std::ostringstream oss;
        oss << "CacheEntry {\n";
        oss << "  CacheType:     \"" << getCacheType() << "\"\n";
        oss << "  FullName: \"" << getFullName() << "\"\n";
        oss << "  Name: \"" << getName() << "\"\n";
        oss << "  Origin: \"" << getOrigin() << "\"\n";
        oss << "  OuterName:     \"" << getOuterName() << "\"\n";
        oss << "  OuterNameCPP:  \"" << getOuterNameCPP() << "\"\n";
        oss << "  Package*: " << getPackage() << "\n";
        oss << "  Package:       \"" << getPackageName() << "\"\n";
        oss << "  SanitizedName: \"" << getSanitizedName() << "\"\n";
        oss << "  UObject*: " << getObject() << "\n";
        oss << "}";

        return oss.str();
    }

    bool operator==(const ObjectEntry& other) const {
        return getFullName() == other.getFullName();
    }

    bool operator<(const ObjectEntry& other) const {
        return getFullName() < other.getFullName();
    }

    // hash function for unordered containers
    struct ObjectEntryHash {
        std::size_t operator()(const ObjectEntry* obj) const {
            return std::hash<std::string>{}(obj->getFullName());
        }
    };

    virtual auto getParamKey() const -> std::string { return ""; };

protected:
    UObject* uObject_{nullptr};
    bool iterated_{false};
    bool childrenIterated_{false};
    mutable EClassTypes type_ = EClassTypes::Unresolved;
    mutable UObject* package_{nullptr};
    mutable std::string fullName_;
    mutable std::string nameCPP_;
    mutable std::string name_;
    mutable std::string outerNameCPP_;
    mutable std::string outerName_;
    mutable std::string packageName_;
    mutable std::string sanitizedName_;
    mutable std::optional<std::string> overrideEmitName_;
    mutable std::optional<std::string> alternateEmitName_;
    mutable std::string emitTypeStr_{"<unknown>"};
    mutable bool isOverridden_{false};
    mutable bool isBlacklisted_{false};
    mutable bool blacklistChecked_{false};
    std::string overwriteName_;
    ptrdiff_t size_{0};
    size_t cacheIndex{0}; // Optional: useful for IDX_ generation or stable sort
    std::string origin_;
    std::vector<std::string> referrers_;

    // but why tho
    const std::vector<std::string> unsafeNames_ = {
        "GetComputerName",
        "GetCurrentTime",
        "GetObject",
        "DeleteFile",
        "DrawText",
        "SendMessage",
        "GetMessage",
        "PlaySound"
    };
};
