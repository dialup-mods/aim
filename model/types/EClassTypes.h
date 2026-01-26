#pragma once
#include <cstdint>
#include <string>

enum class EClassTypes : uint8_t {
    Unresolved, // internal, have not tried to find type yet type
    Unknown,
    FNameEntry,
    UObject,
    UClass,
    UField,
    UEnum,
    UConst,
    UProperty,
    UStruct,
    UScriptStruct,
    UFunction,
    UStructProperty,
    UObjectProperty,
    UClassProperty,
    UMapProperty,
    UInterfaceProperty,
    UQWordProperty,
    UByteProperty,
    UBoolProperty,
    UArrayProperty,
    UIntProperty,
    UInt64Property,
    UFloatProperty,
    UStrProperty,
    UNameProperty,
    UDelegateProperty
};

inline std::string ToString(const EClassTypes type) {
    switch (type) {
        case EClassTypes::FNameEntry: return "FNameEntry";
        case EClassTypes::UObject: return "UObject";
        case EClassTypes::UClass: return "UClass";
        case EClassTypes::UField: return "UField";
        case EClassTypes::UEnum: return "UEnum";
        case EClassTypes::UConst: return "UConst";
        case EClassTypes::UProperty: return "UProperty";
        case EClassTypes::UStruct: return "UStruct";
        case EClassTypes::UScriptStruct: return "UScriptStruct";
        case EClassTypes::UFunction: return "UFunction";
        case EClassTypes::UStructProperty: return "UStructProperty";
        case EClassTypes::UObjectProperty: return "UObjectProperty";
        case EClassTypes::UClassProperty: return "UClassProperty";
        case EClassTypes::UMapProperty: return "UMapProperty";
        case EClassTypes::UInterfaceProperty: return "UInterfaceProperty";
        case EClassTypes::UQWordProperty: return "UQWordProperty";
        case EClassTypes::UByteProperty: return "UByteProperty";
        case EClassTypes::UBoolProperty: return "UBoolProperty";
        case EClassTypes::UArrayProperty: return "UArrayProperty";
        case EClassTypes::UIntProperty: return "UIntProperty";
        case EClassTypes::UInt64Property: return "UInt64Property";
        case EClassTypes::UFloatProperty: return "UFloatProperty";
        case EClassTypes::UStrProperty: return "UStrProperty";
        case EClassTypes::UNameProperty: return "UNameProperty";
        case EClassTypes::UDelegateProperty: return "UDelegateProperty";
        case EClassTypes::Unresolved: return "";
        default: return "Unknown";
    }
}