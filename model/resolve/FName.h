#pragma once
#include "ValueResolver.h"

struct FNameResolver {
    static void resolve(MyResolvedValue& out, void* valuePtr) {
        FName* fName = *reinterpret_cast<FName**>(valuePtr);

        if (!fName) {
            out.mKind = MyResolvedValue::Kind::Null;
            return;
        }
        out.mKind = MyResolvedValue::Kind::String;
        out.mStorage = MyResolvedValue::StorageType::InlineStruct;

        // fixme make public, regenerate sdk
        //out.name->entryId = fName->FNameEntryId;
        //out.name->instanceId = fName->InstanceNumber;

        // Optional / gated
        //if (canResolveNames()) {
        out.primitiveStr = fName->ToString();
        if (!fName->IsValid()) {
            out.mInvalid = true;
        }
        //}
    }
};