#pragma once
#include "ValueResolver.h"
#include "SDK.h"

// fixme
// You’ll eventually want printers like:
// Interface:
//   Object: Car_TA / GameEvent_Soccar
//   Class: Car_TA
//   InterfacePtr: 0x7FF6A1C93040
//
// That is extremely useful for:
//
//     diffing
//
//     detecting unexpected interface bindings
//
//     correlating behavior across calls

// NOTE: Treat FScriptInterface as ObjectRef + opaque pointer
// Copy data, never reference engine memory
// Do not attempt reflection here
// Keep resolvers boring and honest
struct DelegateResolver {
    void resolve(ResolvedValue& out, void* valuePtr) const {
        const FScriptInterface* interface = reinterpret_cast<const FScriptInterface*>(valuePtr);

        if (!interface || !interface->Object) {
            out.kind = ResolvedValue::Kind::Null;
            return;
        }

        out.kind = ResolvedValue::Kind::Interface;

        out.interface->object = interface->Object;
        // fixme engine gate
        //out.interface->objectName = interface->Object->GetFullName();
        //out.interface->className =
        //    interface->Object->Class ? interface->Object->Class->GetName() : "<Unknown>";

        out.interface->interfacePtr = reinterpret_cast<uintptr_t>(interface->Interface);
    }
};