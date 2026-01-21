#pragma once
#include "ValueResolver.h"
#include "SDK.h"
#include "StringTool.h"

struct ByteResolver {
    static void resolve(ResolvedValue& out, void* valuePtr, UEnum* enumMaybe) {
        const int32_t raw = *reinterpret_cast<int32_t*>(valuePtr);

        out.storage = ResolvedValue::StorageType::UInt32;

        if (enumMaybe) {
            out.kind = ResolvedValue::Kind::Enum;
            out.uEnum = enumMaybe;
            if (raw < enumMaybe->Names.size()) {
                const FName& name = enumMaybe->Names[raw];
                if (!name.FNameEntryId) {
                    out.invalid = true;
                    return;
                }
                out.primitiveStr = "FNameEntryId: " + std::to_string(name.FNameEntryId) + "InstanceNumber: " + std::to_string(name.InstanceNumber);
                // fixme engine gate
                //{
                //    const auto nameEntry = Runtime::getFNameEntry(name.FNameEntryId);
                //    if (!nameEntry) {
                //        out.mInvalid = true;
                //        return;
                //    }
                //    const std::wstring string(nameEntry->Name);
                //    out.primitiveStr = string_tool::enshrinken(string);
                //}
            } else {
                out.invalid = true;
                out.primitiveStr =
                    "<invalid:" + std::to_string(raw) + ">";
            }
        } else {
            out.kind = ResolvedValue::Kind::Int32;
            out.primitiveStr = std::to_string(raw);
        }
    }
};