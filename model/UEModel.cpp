#include "UEModel.h"

#include "Schema.h"
#include "Object.h"
#include "UClass.h"
#include "UConst.h"
#include "UEnum.h"
#include "UObject.h"
#include "UScriptStruct.h"

#include "UArrayProperty.h"
#include "UBoolProperty.h"
#include "UByteProperty.h"
#include "UClassProperty.h"
#include "UDelegateProperty.h"
#include "UFloatProperty.h"
#include "UFunction.h"
#include "UIntProperty.h"
#include "UInterfaceProperty.h"
#include "UMapProperty.h"
#include "UNameProperty.h"
#include "UObjectProperty.h"
#include "UQWordProperty.h"
#include "UStrProperty.h"
#include "UStructProperty.h"

namespace UEModel {
auto assignClass(UObject* rawObj) -> std::shared_ptr<ObjectEntry> {
    std::unique_ptr<ObjectEntry> entry;
    if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ArrayProperty"))) {
        entry = std::make_unique<UArrayPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.StrProperty"))) {
        entry = std::make_unique<UStrPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.IntProperty"))) {
        entry = std::make_unique<UIntPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.FloatProperty"))) {
        entry = std::make_unique<UFloatPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.DelegateProperty"))) {
        entry = std::make_unique<UDelegatePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.NameProperty"))) {
        entry = std::make_unique<UNamePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.StructProperty"))) {
        entry = std::make_unique<UStructPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ClassProperty"))) {
        entry = std::make_unique<UClassPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ObjectProperty"))) {
        entry = std::make_unique<UObjectPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.MapProperty"))) {
        entry = std::make_unique<UMapPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.InterfaceProperty"))) {
        entry = std::make_unique<UInterfacePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.QWordProperty"))) {
        entry = std::make_unique<UQWordPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.BoolProperty"))) {
        entry = std::make_unique<UBoolPropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ByteProperty"))) {
        entry = std::make_unique<UBytePropertyEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Enum"))) {
        entry = std::make_unique<EnumEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Class"))) {
        entry = std::make_unique<ClassEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Function"))) {
        entry = std::make_unique<UFunctionEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.ScriptStruct"))) {
        entry = std::make_unique<UScriptStructEntry>(rawObj);
        // fixme getFullName is empty
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.State"))) {
        //Logger::instance().log("[WARN] UState. Not yet implemented. {}", entry->getFullName());
        return nullptr;
        // fixme unimplemented
        //entry = std::make_unique<UState>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Struct"))) {
        Logger::instance().log("[WARN] Raw UStruct.");
        entry = std::make_unique<UStructEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Const"))) {
        entry = std::make_unique<ConstEntry>(rawObj);
    } else if (r::types::inheritsFrom(rawObj, r::uclass::find("Class Core.Object"))) {
        entry = std::make_unique<UObjectEntry>(rawObj);
    } else {
        entry = std::make_unique<ObjectEntry>(rawObj);
        // fixme getFullName will crash on invalid elements
        Logger::instance().log("[WARN] Using fallback type for: {}", entry->getFullName());
    }
    return entry;
}

auto assignClassFromRawPtr(void* raw) -> std::shared_ptr<ObjectEntry> {
    return assignClass(reinterpret_cast<UObject*>(raw));
}
}