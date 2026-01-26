#pragma once
#include "AllModelTypes.h"

using ClassKindMap = std::unordered_map<std::string_view, EClassTypes>;

static const ClassKindMap kClassKind = {
    { UByteProperty::className,        EClassTypes::UByteProperty },
    { UBoolProperty::className,        EClassTypes::UBoolProperty },
    { UIntProperty::className,         EClassTypes::UIntProperty },
    { UFloatProperty::className,       EClassTypes::UFloatProperty },
    { UStrProperty::className,         EClassTypes::UStrProperty },
    { UNameProperty::className,        EClassTypes::UNameProperty },
    { UQWordProperty::className,       EClassTypes::UQWordProperty },

    { UObjectProperty::className,      EClassTypes::UObjectProperty },
    { UClassProperty::className,       EClassTypes::UClassProperty },
    { UInterfaceProperty::className,   EClassTypes::UInterfaceProperty },
    { UStructProperty::className,      EClassTypes::UStructProperty },
    { UArrayProperty::className,       EClassTypes::UArrayProperty },
    { UMapProperty::className,         EClassTypes::UMapProperty },
    { UDelegateProperty::className,    EClassTypes::UDelegateProperty },

    { UFunction::className,            EClassTypes::UFunction },
    { UConst::className,               EClassTypes::UConst },
    { UEnum::className,                EClassTypes::UEnum },
    { UScriptStruct::className,        EClassTypes::UScriptStruct },
    { UClass::className,               EClassTypes::UClass },
    { UStruct::className,              EClassTypes::UStruct },
};