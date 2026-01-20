#pragma once
#include "ValueResolver.h"

struct ClassResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr) {
        UClass* cls = *reinterpret_cast<UClass**>(valuePtr);

        if (!cls) {
            out.mKind = MyResolvedValue::Kind::Null;
            return;
        }

        out.mStorage = MyResolvedValue::StorageType::UInt32;
        out.mKind = MyResolvedValue::Kind::Class;
        out.object = cls;
        out.className = cls->GetName();
        out.fullName  = cls->GetFullName();
        out.superName = cls->SuperField
            ? cls->SuperField->GetName()
            : "<None>";
    }
};