#pragma once
#include <string>

#include "StructLike.h"
#include "ValueResolver.h"

struct PrintOptions {
    bool shallow = true;
    bool showAddresses = false;
    bool showTypes = true;
    int  maxDepth = 2;
};

//class Printer {
//public:
//    static std::string print(
//        const ResolvedValue& v,
//        const PrintOptions& opts = {}
//    );
//
//    static auto debugPrint(const ResolvedValue &v) -> std::string {
//        switch (v.kind) {
//            case ResolvedValue::Kind::ObjectRef:
//                return v.objectName.empty() ? "<object>" : v.objectName;
//
//            case ResolvedValue::Kind::Struct:
//                return "<struct " + std::string(v.schema->getName()) + ">";
//
//            case ResolvedValue::Kind::Array:
//                return "<array>";
//
//            default:
//                return v.primitiveStr.empty()
//                    ? "<unprinted>"
//                    : v.primitiveStr;
//        }
//    }
//};

//std::string print(const ResolvedValue& v, const PrintOptions& opts) {
//    switch (v.kind) {
//        case ResolvedValue::Kind::Bool:
//        case ResolvedValue::Kind::Int32:
//        case ResolvedValue::Kind::Int64:
//        case ResolvedValue::Kind::Float:
//        case ResolvedValue::Kind::Double:
//            return v.primitiveStr;
//
//        case ResolvedValue::Kind::String:
//            return "\"" + v.primitiveStr + "\"";
//
//        case ResolvedValue::Kind::Name:
//            return v.fullName;
//
//        case ResolvedValue::Kind::Enum:
//            return printEnum(v);
//
//        case ResolvedValue::Kind::ObjectRef:
//            return printObject(v, opts);
//
//        case ResolvedValue::Kind::Struct:
//            return printStruct(v, opts);
//
//        case ResolvedValue::Kind::Array:
//            return printArray(v, opts);
//
//        case ResolvedValue::Kind::Delegate:
//            return printDelegate(v);
//
//        case ResolvedValue::Kind::Interface:
//            return printInterface(v);
//
//        case ResolvedValue::Kind::Null:
//            return "null";
//
//        default:
//            return "<unprintable>";
//    }
//}