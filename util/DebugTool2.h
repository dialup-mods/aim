#pragma once
#include "SdkHeaders.hpp"

#include <algorithm>

// run from within PE detour
void findProcessInternal(UFunction* fn) {
    static std::unordered_map<uintptr_t, int> funcCounts;
    static int totalCalls = 0;

    if (fn) {
        bool isNative = (fn->FunctionFlags & FUNC_Native) != 0;

        // For non-native functions, check if they have a Func pointer
        if (!isNative && fn->Func.Dummy != 0) {
            uintptr_t funcPtr = fn->Func.Dummy;
            funcCounts[funcPtr]++;

            // Log the first few occurrences of each function pointer
            if (funcCounts[funcPtr] <= 3) {
                printf("[SCRIPT] %s -> Func: 0x%p (count: %d)\n",
                       fn->GetFullName().c_str(),
                       reinterpret_cast<void*>(funcPtr),
                       funcCounts[funcPtr]);
            }

            // Periodically print the most common function pointers
            if (++totalCalls % 1000 == 0) {
                std::vector<std::pair<uintptr_t, int>> sortedFuncs(funcCounts.begin(), funcCounts.end());
                std::sort(sortedFuncs.begin(), sortedFuncs.end(),
                          [](const auto& a, const auto& b) { return a.second > b.second; });

                printf("\n=== Potential ProcessInternal Candidates ===\n");
                for (size_t i = 0; i < std::min<size_t>(5, sortedFuncs.size()); i++) {
                    printf("0x%p: %d calls\n",
                           reinterpret_cast<void*>(sortedFuncs[i].first),
                           sortedFuncs[i].second);
                }
            }
        }
    }
}

void inspectUFunctionHierarchy(UFunction* fn) {
    if (!fn || !safe::memory::isAddressAccessible(fn, sizeof(void*))) {
        printf("[ERROR] Invalid UFunction pointer\n");
        return;
    }

    printf("\n[DEBUG] UFunction Hierarchy Inspection for: %s\n", fn->GetFullName().c_str());

    // UObject level
    printf("--- UObject Level ---\n");
    printf("ObjectFlags: 0x%llx\n", fn->ObjectFlags);
    printf("ObjectInternalInteger: %d\n", fn->ObjectInternalInteger);
    printf("Name: %s\n", fn->Name.ToString().c_str());

    if (fn->Outer && safe::memory::isAddressAccessible(fn->Outer, sizeof(void*))) {
        printf("Outer: %s (0x%p)\n", fn->Outer->Name.ToString().c_str(), reinterpret_cast<void*>(fn->Outer));
    } else {
        printf("Outer: <inaccessible>\n");
    }

    // UField level
    printf("\n--- UField Level ---\n");
    UField* asField = static_cast<UField*>(fn);

    if (asField->Next && safe::memory::isAddressAccessible(asField->Next, sizeof(void*))) {
        printf("Next: %s (0x%p)\n", asField->Next->Name.ToString().c_str(), reinterpret_cast<void*>(asField->Next));
    } else {
        printf("Next: <null or inaccessible>\n");
    }

    // UStruct level
    printf("\n--- UStruct Level ---\n");
    UStruct* asStruct = static_cast<UStruct*>(fn);

    if (asStruct->Children && safe::memory::isAddressAccessible(asStruct->Children, sizeof(void*))) {
        printf("Children: %s (0x%p)\n", asStruct->Children->Name.ToString().c_str(), reinterpret_cast<void*>(asStruct->Children));
    } else {
        printf("Children: <null or inaccessible>\n");
    }

    printf("PropertySize: %d\n", asStruct->PropertySize);
    
    if (asStruct->SuperField && safe::memory::isAddressAccessible(asStruct->SuperField, sizeof(void*))) {
        printf("SuperField: %s (0x%p)\n", asStruct->SuperField->Name.ToString().c_str(), reinterpret_cast<void*>(asStruct->SuperField));
    } else {
        printf("SuperField: <null or inaccessible>\n");
    }


    // UFunction level
    printf("\n--- UFunction Level ---\n");
    printf("FunctionFlags: 0x%p\n", reinterpret_cast<void*>(fn->FunctionFlags));
    printf("Func: 0x%p\n", reinterpret_cast<void*>(fn->Func.Dummy));

    // Raw memory dump for verification
    printf("\n--- Raw Memory Dump ---\n");
    for (int offset = 0; offset < 0x160; offset += 0x8) {
        if (safe::memory::isAddressAccessible(reinterpret_cast<uint8_t*>(fn) + offset, 16)) {
            printf("Offset +0x%02X: ", offset);
            for (int i = 0; i < 16; i++) {
                printf("%02X ", *(reinterpret_cast<uint8_t*>(fn) + offset + i));
            }
            printf("\n");
        }
    }
}

void inspectOffset(void* base, size_t offset, size_t size = 8, const char* label = "Unknown") {
    if (!safe::memory::isAddressAccessible(reinterpret_cast<uint8_t*>(base) + offset, size)) {
        printf("[ERROR] Cannot access offset 0x%zX (%s)\n", offset, label);
        return;
    }

    printf("Offset 0x%zX (%s): ", offset, label);
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", *(reinterpret_cast<uint8_t*>(base) + offset + i));
    }

    // Try to interpret as pointer and value
    if (size >= 8) {
        uint64_t value = *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(base) + offset);
        printf("(u64: %llu, ptr: 0x%p)", value, reinterpret_cast<void*>(value));
    } else if (size >= 4) {
        uint32_t value = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(base) + offset);
        printf("(u32: %u)", value);
    }

    printf("\n");
}

void printMemoryBytes(void* ptr, size_t size) {
    unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);
    printf("[DEBUG] Memory at address 0x%p:\n", ptr);
    for (size_t i = 0; i < size; ++i) {
        printf("%02X ", bytePtr[i]);
        if ((i + 1) % 16 == 0) {  // Print 16 bytes per line
            printf("\n");
        }
    }
    printf("\n");
}


    inline void reallyDumpParms(UFunction* fn, void* parms) {
        for (UField* field = fn->Children; field; field = field->Next) {
            if (!field) break; // belt-and-suspenders

            // ASSUME field is UProperty* if we're walking function parameters
            UProperty* prop = reinterpret_cast<UProperty*>(field);

            // Now you can safely spy:
            uint8_t* base = reinterpret_cast<uint8_t*>(parms) + logger->fmtSafeOffset(prop->Offset);

            if (prop->PropertyFlags & 0x8) {  // CPF_Int
                int32_t value = *reinterpret_cast<int32_t*>(base);
                int offset = logger->fmtSafeOffset(prop->Offset);
                logger->log(" → int param offset: " + stringutil::toHex(offset) + " : " + std::to_string(value));
            }
            else if (prop->PropertyFlags & 0x10) {  // CPF_Float
                float value = *reinterpret_cast<float*>(base);
                auto offset = logger->fmtSafeOffset(prop->Offset);
                //                logf_debug(" → float param (offset {}) = {}", offset, value);
            }
            else if (prop->PropertyFlags & 0x20) {  // CPF_Bool
                bool value = *reinterpret_cast<uint8_t*>(base) != 0;
                auto offset = logger->fmtSafeOffset(prop->Offset);
                //               logf_debug(" → bool param (offset {}) = {}", offset, value);
            }
            else {
                auto offset = logger->fmtSafeOffset(prop->Offset);
                logf_debug(" → Unknown param (offset {}, flags 0x{:X})", offset, logger->fmtSafeOffset64(prop->PropertyFlags));
            }
        }
    }

void
ChatManager::dumpChatInstance(UGFxData_Chat_TA* chat) {
    if (!chat) {
        log_->warn("object was null");
    }
    auto name = chat->Name.ToString();
    auto outer = chat->Outer ? chat->Outer->Name.ToString() : "null";
    auto flags = chat->ObjectFlags;

    auto vt = *(void**)chat;

    auto shell = chat->Shell;
    auto ds = shell ? shell->DataStore : nullptr;

    log_->logf_debug("chat = {}", fmt::ptr(chat));
    log_->logf_debug("name = {}", name);
    log_->logf_debug("outer = {}", outer);
    log_->logf_debug("flags = 0x{:X}", flags);
    log_->logf_debug("vtable = {}", fmt::ptr(vt));
    log_->logf_debug("shell = {}", fmt::ptr(shell));
    log_->logf_debug("datastore = {}", fmt::ptr(ds));

    if (ds) {
        log_->logf_debug("Printing DS:");
        ds->PrintData(FName(L"ChatPresetMessages"));
    }
    
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ds, &mbi, sizeof(mbi))) {
        log_->debug("Base address: " + std::to_string((uintptr_t)mbi.BaseAddress));
        log_->debug("Region size: " + std::to_string(mbi.RegionSize));
        log_->debug("State: " + std::to_string(mbi.State)); // MEM_COMMIT = 0x1000
        log_->debug("Protect: " + std::to_string(mbi.Protect));
        // PAGE_READWRITE = 0x04, PAGE_READONLY = 0x02, PAGE_EXECUTE_READ = 0x20
        log_->debug("Type: " + std::to_string(mbi.Type)); // MEM_PRIVATE = 0x20000
    }

    if (VirtualQuery(ds->Tables.data(), &mbi, sizeof(mbi))) {
        log_->debug("Tables array protect: " + std::to_string(mbi.Protect));
    }

    for (auto& table : ds->Tables) {
        if (table.Name == FName(TEXT("ChatPresetMessages"))) {
            if (VirtualQuery(&table, &mbi, sizeof(mbi))) {
                log_->debug("chat message array protect: " + std::to_string(mbi.Protect));
            }
        }
    }
}

void
ChatManager::printDataStores(UGFxData_Chat_TA* chat) const {
    log_->debug("datastores callback");

    auto datastores = objectProvider_->getAllInstancesOf<UGFxDataStore_X>();
    for (auto* datastore : datastores) {
        log_->debug("x");
        if (datastore == nullptr) {
            continue;
        }
        log_->debug(datastore->GetFullName());
        auto tables = datastore->Tables;
        for (auto table : tables) {
            log_->debug(" found table: " + table.Name.ToString());
        }
    }
}

void
ChatManager::printDataStoreTables(UGFxData_Chat_TA* chat) const {
    auto ds = chat->Shell->DataStore;
    for (auto table : ds->Tables) {
        log_->debug(" found table: " + table.Name.ToString());
    }
}
