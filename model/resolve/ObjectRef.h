#pragma once
#include "ValueResolver.h"

struct ObjectRefResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr, UObject* obj) {
        if (!obj) {
            out.mKind = MyResolvedValue::Kind::Null;
            return;
        }

        out.mStorage = MyResolvedValue::StorageType::UInt32;
        out.mKind = MyResolvedValue::Kind::ObjectRef;

        out.object = obj;
    }
};