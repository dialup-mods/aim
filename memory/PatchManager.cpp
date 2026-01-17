#include <Windows.h>
#include <unordered_set>
#include <stdexcept>

#include "PatchManager.h"

#include "ILogger.h"

#include "PatchDefinition.h"

#include "PatchUtils.h"
#include "SafeMemory.h"


PatchManager::~PatchManager() {
    printf("PatchManager unloaded\n");
}

void
PatchManager::init() {
    printf("\n\n patchmanager init\n\n");
}

void PatchManager::add(const PatchDefinition& patch) {
    patches_.emplace_back(patch);
}

void PatchManager::writeMemory(uintptr_t address, const void* data, size_t size) {
    if (size == 0) return;
    std::memcpy(reinterpret_cast<void*>(address), data, size);
}

void PatchManager::captureBytes(void* cursor, PatchStep& step) {
    if (!safe::memory::isAddressAccessible(cursor, step.bytes.size())) {
        MessageBoxA(nullptr, "memory not accessible", "captureBytes()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return;
    }

    step.originalBytes.resize(step.bytes.size());
    safe::memory::read(reinterpret_cast<uintptr_t>(cursor), step.originalBytes.data(), step.originalBytes.size());
}

bool PatchManager::applyPatchStep(PatchStep& step, uintptr_t& cursor, bool dryRun) {
    uintptr_t addr = step.address ? step.address : cursor;
    //printf("[applyPatchStep] type: %d, addr: 0x%llX, bytes: %zu, dryRun: %d\n",
    //    static_cast<int>(step.type),
    //    static_cast<unsigned long long>(addr),
    //    step.bytes.size(),
    //    dryRun);
    
    if (!dryRun) {
        if (!safe::memory::isAddressAccessible(reinterpret_cast<void*>(cursor), step.bytes.size())) {
            MessageBoxA(nullptr, "memory not accessible", "applyPatchStep()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
            return false;
        }
    }

    switch (step.type) {
        case PatchStepType::Bytes:
            if (dryRun) {
                //log_->logf_debug("[DryRun] Write {} bytes to 0x{:X}", step.bytes.size(), cursor);
                patchutils::disassemble(step.bytes.data(), step.bytes.size());
            } else {
                captureBytes(reinterpret_cast<void*>(cursor), step);
                safe::memory::writeBytes(reinterpret_cast<void*>(cursor), step.bytes);
            }
            cursor += step.bytes.size();
            break;

        case PatchStepType::ShortJump: {
            auto jumpBytes = patchutils::forgeShortJumpBytes(cursor, step.jumpDestination);
            //printf("[shortJump] cursor: 0x%llX → 0x%llX | bytes: %zu\n",
            //    cursor,
            //    step.jumpDestination,
            //    jumpBytes.size());
        
            if (dryRun) {
                //log_->logf_debug("[DryRun] ShortJump from 0x{:X} to 0x{:X}", cursor, step.jumpDestination);
        
                if (!jumpBytes.empty()) {
                    patchutils::disassemble(jumpBytes.data(), jumpBytes.size());
                }
            } else {
                captureBytes(reinterpret_cast<void*>(cursor), step);
                safe::memory::writeBytes(reinterpret_cast<void*>(cursor), jumpBytes);
            }
        
            cursor += jumpBytes.size();
            break;
        }

        case PatchStepType::AbsoluteJump:
            if (dryRun) {
                //log_->logf_debug("[DryRun] AbsoluteJump writing {} bytes at 0x{:X}", step.bytes.size(), cursor);
            } else {
                captureBytes(reinterpret_cast<void*>(cursor), step);
                safe::memory::writeBytes(reinterpret_cast<void*>(cursor), step.bytes);
                patchutils::disassemble(step.bytes.data(), step.bytes.size());
            }
            cursor += step.bytes.size();
            break;

        case PatchStepType::Padding:
            if (dryRun) {
                //log_->logf_debug("[DryRun] Padding {} bytes at 0x{:X} with 0x90", step.size, cursor);
            } else {
                captureBytes(reinterpret_cast<void*>(cursor), step);
                safe::memory::fillBytes(reinterpret_cast<void*>(cursor), 0x90, step.size);
            }
            cursor += step.size;
            break;

        case PatchStepType::OverwriteBytes: {
            std::vector<uint8_t> existing(step.expectedBytes.size());
            safe::memory::read(step.address, existing.data(), existing.size());

            if (existing != step.expectedBytes) {
                //log_->logf_debug("Memory mismatch at 0x{:X}. Aborting patch.", step.address);
                MessageBoxA(nullptr, "memory mismatch", "applyPatchStep()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
                return false;
            }

            if (dryRun) {
                //log_->logf_debug("[DryRun] Expected bytes: {}", stringutil::toHex(step.expectedBytes));
                //log_->logf_debug("[DryRun] Overwrite {} bytes at 0x{:X}", step.bytes.size(), step.address);
                patchutils::disassemble(step.bytes.data(), step.bytes.size());
            } else {
                writeMemory(step.address, step.bytes.data(), step.bytes.size());
                ////log_->logf_debug("Patched address 0x{:X} successfully.", step.address);
            }
            break;
        }

        default:
            return false;
    }

    return true;
}

std::unordered_set<void*> calculatePagesToProtect(const PatchDefinition& patch) {
    std::unordered_set<void*> pages;

    uintptr_t cursor = 0;

    for (auto& step : patch.steps) {
        if (step.type == PatchStepType::SetPosition) {
            cursor = step.address;
            continue;
        }

        if (step.type == PatchStepType::Bytes) {
            // Mark all pages touched by these bytes
            uintptr_t start = cursor;
            uintptr_t end = cursor + step.bytes.size();
            for (uintptr_t addr = start; addr < end; addr = (addr & ~(PAGE_SIZE - 1)) + PAGE_SIZE) {
                pages.insert(reinterpret_cast<void*>(addr & ~(PAGE_SIZE - 1)));
            }
            cursor += step.bytes.size();
        }
        else if (step.type == PatchStepType::ShortJump) {
            uintptr_t start = cursor;
            uintptr_t end = cursor + 5;
            for (uintptr_t addr = start; addr < end; addr = (addr & ~(PAGE_SIZE - 1)) + PAGE_SIZE) {
                pages.insert(reinterpret_cast<void*>(addr & ~(PAGE_SIZE - 1)));
            }
            cursor += 5;
        }
        else if (step.type == PatchStepType::AbsoluteJump) {
            uintptr_t start = cursor;
            uintptr_t end = cursor + step.bytes.size();
            for (uintptr_t addr = start; addr < end; addr = (addr & ~(PAGE_SIZE - 1)) + PAGE_SIZE) {
                pages.insert(reinterpret_cast<void*>(addr & ~(PAGE_SIZE - 1)));
            }
            cursor += step.bytes.size();
        }
        else if (step.type == PatchStepType::Padding) {
            uintptr_t start = cursor;
            uintptr_t end = cursor + step.size;
            for (uintptr_t addr = start; addr < end; addr = (addr & ~(PAGE_SIZE - 1)) + PAGE_SIZE) {
                pages.insert(reinterpret_cast<void*>(addr & ~(PAGE_SIZE - 1)));
            }
            cursor += step.size;
        }
        else {
            throw std::runtime_error("Unknown PatchStepType encountered during page protection calculation.");
        }
    }

    return pages;
}

bool
PatchManager::apply(PatchDefinition& patch, bool dryRun) {
    printf("[PatchManager] apply() - step count: %zu\n", patch.steps.size());
    
    std::unordered_set<void*> pagesToProtect;
    std::unordered_map<void*, DWORD> changedPages;
    uintptr_t cursor = 0;

    if (!dryRun) {
        pagesToProtect = calculatePagesToProtect(patch);
        
        // make all pages writable
        for (auto page : pagesToProtect) {
            DWORD oldProtect;
            if (VirtualProtect(page, PAGE_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                changedPages[page] = oldProtect;
            } else {
                //log_->logf_debug("Failed to change memory protection for page at 0x{:X}", reinterpret_cast<uintptr_t>(page));
                return false;
            }
        }
    }


    // first pass: apply non-jump steps
    for (auto& step : patch.steps) {
        if (step.type == PatchStepType::SetPosition) {
            cursor = step.address;
        }
        else if (step.type == PatchStepType::Bytes ||
                 step.type == PatchStepType::Padding) 
        {
            if (!applyPatchStep(step, cursor, dryRun)) {
                //log_->logf_debug("Patch step at 0x{:X} failed memory check. Skipping.", cursor);
                MessageBoxA(nullptr, "error applying patch step", "apply()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
                return false;
            }
        }
    }

    // second pass: apply jump steps
    cursor = 0;
    for (auto& step : patch.steps) {
        if (step.type == PatchStepType::SetPosition) {
            cursor = step.address;
        }
        else if (step.type == PatchStepType::ShortJump ||
                 step.type == PatchStepType::AbsoluteJump) 
        {
            if (!applyPatchStep(step, cursor, dryRun)) {
                //log_->logf_debug("Patch step at 0x{:X} failed memory check. Skipping.", cursor);
                MessageBoxA(nullptr, "error applying Patchstep", "apply()", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
                return false;
            }
        }
    }

    if (!dryRun) {
        // restore original protections
        for (auto& [page, oldProtect] : changedPages) {
            VirtualProtect(page, PAGE_SIZE, oldProtect, &oldProtect);
        }

        // flush cache
        for (auto& page : pagesToProtect) {
            FlushInstructionCache(GetCurrentProcess(), page, PAGE_SIZE);
        }
    }

    return true;
}

void
PatchManager::restore(const PatchDefinition& patch) {
    auto pagesToProtect = calculatePagesToProtect(patch);
    std::unordered_map<void*, DWORD> changedPages;

    // make all pages writable
    for (auto page : pagesToProtect) {
        DWORD oldProtect;
        if (VirtualProtect(page, PAGE_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            changedPages[page] = oldProtect;
        } else {
            //log_->logf_debug("Failed to change memory protection for page at 0x{:X}", reinterpret_cast<uintptr_t>(page));
        }
    }

    uintptr_t cursor = 0;

    for (auto& step : patch.steps) {
        if (step.type == PatchStepType::SetPosition) {
            cursor = step.address;
        }
        else if (!step.originalBytes.empty()) {
            safe::memory::writeBytes(reinterpret_cast<void*>(cursor), step.originalBytes);
            cursor += step.originalBytes.size();
        }
    }

    // restore original protections
    for (auto& [page, oldProtect] : changedPages) {
        VirtualProtect(page, PAGE_SIZE, oldProtect, &oldProtect);
    }

    // flush cache
    for (auto& page : pagesToProtect) {
        FlushInstructionCache(GetCurrentProcess(), page, PAGE_SIZE);
    }
}

void
PatchManager::saveRegisters() {}

void
PatchManager::restoreRegisters() {}

void PatchManager::applyAll() {
    for (auto& patch : patches_) {
        this->apply(patch);
    }
}

void PatchManager::restoreAll() {
    for (auto& patch : patches_) {
        this->restore(patch);
    }
}

void PatchManager::dryRun(PatchDefinition& patch) {
    return;
}

void PatchManager::dryRunAll() {
    for (auto& patch : patches_) {
        this->apply(patch, true);
    }
}

void PatchManager::debugPrintPatches() const {
    for (const auto& patchDef : patches_) {
        //log_->logf_debug("Patch: {}", patchDef.name);

        for (auto& step : patchDef.steps) {
            //log_->logf_debug("  Step: src = 0x{:X}, size = {}", step.address, step.bytes.size());
        }
    }
}

void PatchManager::debugPrintPatchesVerbose() const {
    for (const auto& patchDef : patches_) {
        //log_->logf_debug("Patch: {}", patchDef.name);

        for (auto& step : patchDef.steps) {
            //log_->logf_debug("  Step: src = 0x{:X}, size = {}", step.address, step.bytes.size());
            
            if (!step.bytes.empty() && step.bytes[0] == 0xE9 && step.bytes.size() >= 5) {
                // This is a short jump (JMP rel32)
                int32_t relOffset;
                memcpy(&relOffset, &step.bytes[1], sizeof(relOffset));
        
                uintptr_t dest = step.address + 5 + static_cast<intptr_t>(relOffset);
        
                //log_->logf_debug("    -> JMP destination: 0x{:X}", dest);
            }

            if (step.type == PatchStepType::OverwriteBytes) {
                patchutils::disassemble(step.bytes.data(), step.bytes.size());
            }
        }
    }
}

//void PatchManager::debugPrintPatches() const {
//    for (const auto& patch : patches_) {
//        std::cout << "[PatchManager] Patch @ " << std::hex << patch.targetAddress
//                  << " (" << patch.length << " bytes) Type: " << static_cast<int>(patch.type) << "\n";
//    }
//}