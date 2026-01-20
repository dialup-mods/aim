#pragma once
#include <string>

#include "ValueResolver.h"

//using ValueGetter = std::function<void*(void* params, const PropertyEntry&)>;
//
//using ValuePrinter = std::function<std::string(void* valuePtr)>;
//
//static const std::unordered_map<std::string, ValuePrinter> kPrinters = {
//    { "uint32_t", [](void* v) {
//        return std::to_string(*reinterpret_cast<uint32_t*>(v));
//    }},
//    { "int32_t", [](void* v) {
//        return std::to_string(*reinterpret_cast<int32_t*>(v));
//    }},
//    { "float", [](void* v) {
//        return std::to_string(*reinterpret_cast<float*>(v));
//    }},
//    { "bool", [](void* v) {
//        return (*reinterpret_cast<bool*>(v)) ? "true" : "false";
//    }},
//    { "UObject*", [](void* v) {
//        auto* obj = *reinterpret_cast<UObject**>(v);
//        return obj ? obj->GetFullName() : "nullptr";
//    }},
//};

namespace Printer {
    std::string print(const ValueResolver::ResolvedValue& v) {
        switch (v.kind) {
            case ResolvedValue::Kind::Primitive:
                return v.primitiveStr;

            case ResolvedValue::Kind::Object:
                return fmt::format(
                    "{} ({})",
                    v.objectName,
                    v.objectClass
                );

            case ResolvedValue::Kind::Null:
                return "nullptr";

            case ResolvedValue::Kind::Struct:
                return printStruct(v);

            case ResolvedValue::Kind::Array:
                return printArray(v);
        }
    }
}