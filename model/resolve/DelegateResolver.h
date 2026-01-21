#pragma once
#include "ValueResolver.h"
#include "SDK.h"

struct DelegateResolver {
    static void resolve(ResolvedValue& out, const void* valuePtr) {
        const FScriptDelegate* del =
            reinterpret_cast<const FScriptDelegate*>(valuePtr);

        if (!del || !del->Object) {
            out.kind = ResolvedValue::Kind::Null;
            return;
        }

        out.kind = ResolvedValue::Kind::Delegate;
        out.delegate->object = del->Object;

        std::memcpy(
            out.delegate->unknownData.data(),
            del->UnknownData,
            0x10
        );
    }

    // once FScriptDelegate is properly reversed
    //const FScriptDelegate* del =
    //    reinterpret_cast<const FScriptDelegate*>(valuePtr);

    //if (!del || !del->Object || del->Function.IsNone()) {
    //    out.kind = ResolvedValue::Kind::Null;
    //    return;
    //}

    //out.kind = ResolvedValue::Kind::Delegate;

    //out.delegate.object = del->Object;
    //out.delegate.functionName = del->Function.GetName();

    //// Optional enrichments (safe, read-only)
    //out.delegate.objectName =
    //    del->Object->GetFullName();

    //out.delegate.className =
    //    del->Object->Class
    //        ? del->Object->Class->GetName()
    //        : "<Unknown>";
};