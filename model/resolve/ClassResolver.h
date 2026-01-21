#pragma once
#include "ValueResolver.h"
#include "Schema.h"

struct ClassResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        UClass* cls = *reinterpret_cast<UClass**>(valuePtr);

        if (!cls) {
            out.kind = ResolvedValue::Kind::Null;
            return;
        }

        out.storage = ResolvedValue::StorageType::UInt32;
        out.kind = ResolvedValue::Kind::Class;
        out.object = cls;

        // fixme engine gate
        //out.className = cls->GetName();
        //out.fullName  = cls->GetFullName();
        //out.superName = cls->SuperField
        //    ? cls->SuperField->GetName()
        //    : "<None>";
    }
};