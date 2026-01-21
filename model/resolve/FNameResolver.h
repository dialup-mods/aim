#pragma once
#include "ValueResolver.h"
#include "Runtime.h"

struct FNameResolver {
    static void resolve(ResolvedValue& out, void* valuePtr) {
        const FName* name = *reinterpret_cast<FName**>(valuePtr);

        if (!name) {
            out.kind = ResolvedValue::Kind::Null;
            out.invalid = true;
            return;
        }
        out.kind = ResolvedValue::Kind::String;
        out.storage = ResolvedValue::StorageType::InlineStruct;

        out.name->entryId = name->FNameEntryId;
        out.name->instanceId = name->InstanceNumber;
        out.primitiveStr = "FNameEntryId: " + std::to_string(name->FNameEntryId) + "InstanceNumber: " + std::to_string(name->InstanceNumber);

        // fixme engine gate
        //out.primitiveStr = Runtime::getFNameEntryName(name->InstanceNumber);
    }
};