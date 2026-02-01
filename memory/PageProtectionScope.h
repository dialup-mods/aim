#pragma once
#include <Windows.h>
#include <unordered_set>
#include "ILogger.h"
#include "PatchUtils.h"
namespace p = patchutils;

class PageProtectionScope {
public:
    PageProtectionScope(
        const std::unordered_set<void*>& pages
    )
        : pages_(pages)
    {
        for (auto page : pages_) {
            DWORD oldProtect;
            if (VirtualProtect(page, p::PAGE_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                oldProtect_[page] = oldProtect;
            } else {
                printf(
                    "Failed to change memory protection for page at 0x%llu\n",
                    reinterpret_cast<uintptr_t>(page)
                );
                failed_ = true;
                return;
            }
        }
    }

    ~PageProtectionScope() {
        if (failed_) return;

        for (auto& [page, prot] : oldProtect_) {
            VirtualProtect(page, p::PAGE_SIZE, prot, &prot);
        }

        for (auto page : pages_) {
            FlushInstructionCache(GetCurrentProcess(), page, p::PAGE_SIZE);
        }
    }

    bool ok() const { return !failed_; }

private:
    std::unordered_set<void*> pages_;
    std::unordered_map<void*, DWORD> oldProtect_;
    bool failed_ = false;
};