#pragma once
#include "ValueResolver.h"

struct ObjectRefResolver {
    static void resolve(ResolvedValue& out, void* valuePtr, UObject* obj) {
        if (!obj) {
            out.kind = ResolvedValue::Kind::Null;
            return;
        }

        out.storage = ResolvedValue::StorageType::UInt32;
        out.kind = ResolvedValue::Kind::ObjectRef;

        out.object = obj;
    }
};