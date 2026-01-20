#pragma once
#include "ValueResolver.h"

struct FNameResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        FName* fName = *reinterpret_cast<FName**>(valuePtr);

        if (!fName) {
            out.kind = ResolvedValue::Kind::Null;
            return;
        }
        out.kind = ResolvedValue::Kind::String;
        out.storage = ResolvedValue::StorageType::InlineStruct;

        // fixme make public, regenerate sdk
        //out.name->entryId = fName->FNameEntryId;
        //out.name->instanceId = fName->InstanceNumber;

        // Optional / gated
        //if (canResolveNames()) {
        out.primitiveStr = fName->ToString();
        if (!fName->IsValid()) {
            out.invalid = true;
        }
        //}
    }
};