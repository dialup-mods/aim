#pragma once

//void
//dumpVTable(void* vtableAddr) {
//    uintptr_t* vtable = static_cast<uintptr_t*>(vtableAddr);
//    for (int i = 0; i < 100; ++i) {
//        uintptr_t fnPtr = vtable[i];
//        logger->debug("vtable entry " + std::to_string(i) + ": " + StringUtil::int_to_hex(fnPtr), LogCategory::HOOKS);
//        uint8_t* fn = reinterpret_cast<uint8_t*>(fnPtr);
//        printf("  bytes: %02X %02X %02X %02X %02X\n", fn[0], fn[1], fn[2], fn[3], fn[4]);
//    }
//}
//
//uint64_t
//AIM::getFunctionFlags(UFunction* fn) {
//    return *(uint64_t*)((uintptr_t)fn + 0x130);
//}
//
//inline bool
//AIM::isScriptedFunction(UFunction* fn) {
//    return !AIM::isNativeFunction(fn);
//}
//
//inline bool
//AIM::isNativeFunction(UFunction* fn) {
//    uint64_t flags = AIM::getFunctionFlags(fn);
//    bool isNative = (flags & 0x400) != 0;
//
//    logger->debug("  [Inspector] isNativeFunction(): " + std::to_string(isNative) + " (Flags: " + StringUtil::int_to_hex(flags) + ")",
//        LogCategory::HOOKS);
//    return isNative;
//}
//
//inline void
//AIM::dumpFrame(FFrame frame) {
//    logger->debug("=== FFrame Dump ===", LogCategory::HOOKS);
//    logger->debug("PreviousFrame: " + StringUtil::int_to_hex((uintptr_t)frame.PreviousFrame), LogCategory::HOOKS);
//    logger->debug("OutParms: " + StringUtil::int_to_hex((uintptr_t)frame.OutParms), LogCategory::HOOKS);
//    logger->debug("ReturnValue: " + StringUtil::int_to_hex((uintptr_t)frame.ReturnValue), LogCategory::HOOKS);
//    logger->debug("ProbeMask: " + std::to_string(frame.ProbeMask), LogCategory::HOOKS);
//    logger->debug("CallDepth: " + std::to_string(frame.CallDepth), LogCategory::HOOKS);
//    logger->debug("Node: " + StringUtil::int_to_hex((uintptr_t)frame.Node), LogCategory::HOOKS);
//    logger->debug("Object: " + StringUtil::int_to_hex((uintptr_t)frame.Object), LogCategory::HOOKS);
//    logger->debug("Code: " + StringUtil::int_to_hex((uintptr_t)frame.Code), LogCategory::HOOKS);
//    logger->debug("Locals: " + StringUtil::int_to_hex((uintptr_t)frame.Locals), LogCategory::HOOKS);
//    logger->debug("LineNumber: " + std::to_string(frame.LineNumber), LogCategory::HOOKS);
//}
//
//bool
//AIM::isProbablyValidUFunction(UFunction* fn) {
//    //    if (!fn) return false;
//    //
//    //    uintptr_t addr = reinterpret_cast<uintptr_t>(fn);
//    //    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF)
//    //        return false; // garbage or null
//    //
//    //    if ((fn->FunctionFlags & 0xFFFFFFFF00000000) != 0)
//    //        return false; // unlikely flag overflow
//    //
//    //    if (fn->Func != nullptr && (uintptr_t)fn->Func < 0x10000)
//    //        return false; // Func points to invalid memory
//
//    return true;
//}
//
//bool
//AIM::isProbablyValidPtr(void* ptr) {
//    uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
//    return p >= 0x10000 && p < reinterpret_cast<uintptr_t>(baseAddressPtr_()) + gameSize_();
//}
//
//void
//AIM::logUFunctionSafely(UFunction* fn) {
//    if (!fn || !isProbablyValidPtr(fn)) {
//        logger->debug("- fn: <invalid>", LogCategory::HOOKS);
//        return;
//    }
//
//    try {
//        std::string name = fn->GetFullName();
//        logger->debug("- fn: " + name, LogCategory::HOOKS);
//    } catch (...) { logger->debug("- fn: <exception>", LogCategory::HOOKS); }
//}
//
//bool
//AIM::isProbablyValidUObject(UObject* obj) {
//    if (!obj)
//        return false;
//
//    uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
//    return addr > 0x10000 && addr < 0x7FFFFFFFFFFF;
//}
//
//bool
//AIM::isProbablySafeToTouch(void* ptr, size_t bytes = 16) {
//    MEMORY_BASIC_INFORMATION mbi;
//    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
//        return false;
//
//    // PAGE_GUARD or no access? avoid.
//    if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
//        return false;
//
//    // Optional: ensure it's readable (committed and not execute-only)
//    bool readable = (                                   //
//                        mbi.Protect & PAGE_READONLY) || //
//        (mbi.Protect & PAGE_READWRITE) ||               //
//        (mbi.Protect & PAGE_EXECUTE_READ) ||            //
//        (mbi.Protect & PAGE_EXECUTE_READWRITE);
//
//    return readable && mbi.State == MEM_COMMIT;
//}
//
//std::string
//AIM::SafeGetFullName(UFunction* fn) {
//    if (!fn)
//        return "[null]";
//
//    std::string className = "[bad class]";
//    std::string outerName = "[bad outer]";
//    std::string name = "[bad name]";
//
//    if (isProbablyValidPtr(fn->Class))
//        className = fn->Class->Name.ToString();
//
//    if (isProbablyValidPtr(fn->Outer))
//        outerName = fn->Outer->Name.ToString();
//
//    if (fn->Name.GetNumber() > 0 && fn->Name.GetNumber() < GNames->Num())
//        name = fn->Name.ToString();
//
//    return className + " " + outerName + "." + name;
//}
//
//int
//safeGetNameIndex(UFunction* fn) {
//    if (!fn || IsBadReadPtr(fn, sizeof(UFunction)))
//        return -1;
//
//    // Check if Name is safe to read (sizeof(FName) == 8)
//    if (IsBadReadPtr(&fn->Name, sizeof(int32_t)))
//        return -1;
//
//    return *(int32_t*)(&fn->Name); // Access FNameEntryId directly
//}
//
//uint32_t
//getFunctionFlags(UObject* maybeFunction) {
//    if (!maybeFunction)
//        return 0;
//
//    if (maybeFunction->IsA(UFunction::StaticClass())) {
//        // Safe cast path (assuming layout is correct)
//        auto fn = static_cast<UFunction*>(maybeFunction);
//        return fn->FunctionFlags; // only if UFunction is confirmed aligned
//    }
//
//    // Fallback — raw offset
//    uint8_t* base = reinterpret_cast<uint8_t*>(maybeFunction);
//    return *reinterpret_cast<uint32_t*>(base + 0x14);
//}
//
///*
//bool
//isLikelyValidFFrame(FFrame* frame) {
//    if (!frame) return false;
//
//    // Validate node pointer range
//    auto node = frame->Node;
//    if ((uintptr_t)node < 0x10000 || (uintptr_t)node > 0x7FFFFFFFFFFF) return false;
//
//    // Optionally validate NameIndex range
//    int nameIdx = *reinterpret_cast<int*>((uintptr_t)node + 0x48);
//    return nameIdx >= 0 && nameIdx < knownGNamesMaxIndex;
//}
//*/
//
//inline bool
//AIM::shouldFixResultPtr(UFunction* fn) {
//    bool shouldFix = isNativeFunction(fn);
//    logger->debug("  [Inspector] shouldFixResultPtr(): " + std::to_string(shouldFix), LogCategory::HOOKS);
//    return shouldFix;
//}
//
//auto
//AIM::getHookTargetsFromVTableEntry(int vTableEntry) -> HookTargets {
//    void* vTableEntryPtr = reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Dummy)[vTableEntry];
//    logger->info("found vtable entry: " + StringUtil::int_to_hex(vTableEntryPtr), LogCategory::HOOKS);
//
//    auto chain = AIM::walkTrampolineChain(vTableEntryPtr);
//
//    int i = 0;
//    for (void* addr : chain) {
//        logger->debug(std::to_string(i) + " - chain: " + StringUtil::int_to_hex(addr), LogCategory::HOOKS);
//        i++;
//    }
//
//    if (chain.size() < 2)
//        return { nullptr, nullptr };
//
//    return {
//        chain[1],
//        chain[0],
//    };
//}
