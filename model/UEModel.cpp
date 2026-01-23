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
    std::shared_ptr<ObjectEntry> entry;
    if (rawObj->IsA(UArrayProperty::StaticClass())) {
        entry = std::make_unique<UArrayPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UStrProperty::StaticClass())) {
        entry = std::make_unique<UStrPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UIntProperty::StaticClass())) {
        entry = std::make_unique<UIntPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UFloatProperty::StaticClass())) {
        entry = std::make_unique<UFloatPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UDelegateProperty::StaticClass())) {
        entry = std::make_unique<UDelegatePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UNameProperty::StaticClass())) {
        entry = std::make_unique<UNamePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UStructProperty::StaticClass())) {
        entry = std::make_unique<UStructPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UClassProperty::StaticClass())) {
        entry = std::make_unique<UClassPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UObjectProperty::StaticClass())) {
        entry = std::make_unique<UObjectPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UMapProperty::StaticClass())) {
        entry = std::make_unique<UMapPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UInterfaceProperty::StaticClass())) {
        entry = std::make_unique<UInterfacePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UQWordProperty::StaticClass())) {
        entry = std::make_unique<UQWordPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UBoolProperty::StaticClass())) {
        entry = std::make_unique<UBoolPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UByteProperty::StaticClass())) {
        entry = std::make_unique<UBytePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UEnum::StaticClass())) {
        entry = std::make_unique<EnumEntry>(rawObj);
    } else if (rawObj->IsA(UClass::StaticClass())) {
        entry = std::make_unique<ClassEntry>(rawObj);
    } else if (rawObj->IsA(UFunction::StaticClass())) {
        entry = std::make_unique<UFunctionEntry>(rawObj);
    } else if (rawObj->IsA(UScriptStruct::StaticClass())) {
        entry = std::make_unique<UScriptStructEntry>(rawObj);
    } else if (rawObj->IsA(UState::StaticClass())) {
        printf("[WARN] UState. Not yet implemented. %s\n", entry->getFullName().c_str());
        return nullptr;
        // fixme unimplemented
        //entry = std::make_unique<UState>(rawObj);
    } else if (rawObj->IsA(UStruct::StaticClass())) {
        printf("[WARN] Raw UStruct.\n");
        entry = std::make_unique<UStructEntry>(rawObj);
    } else if (rawObj->IsA(UConst::StaticClass())) {
        entry = std::make_unique<ConstEntry>(rawObj);
    } else if (rawObj->IsA(UObject::StaticClass())) {
        entry = std::make_unique<UObjectEntry>(rawObj);
    } else {
        entry = std::make_unique<ObjectEntry>(rawObj);
        // fixme getFullName will crash on invalid elements
        printf(("[WARN] Using fallback type for: %s\n", entry->getFullName().c_str()));
    }
    return entry;
}

auto assignClassFromRawPtr(void* raw) -> std::shared_ptr<ObjectEntry> {
    return assignClass(reinterpret_cast<UObject*>(raw));
}
}