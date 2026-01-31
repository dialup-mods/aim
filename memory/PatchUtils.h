#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <excpt.h>
#include <span>
#include <vector>

#include "Capture.h"
#include "SafeMemory.h"

namespace patchutils {
    namespace s = safe::memory;

    struct Instruction {
        size_t length;
        const char* mnemonic;
        const char* operands;
    };

    const size_t PAGE_SIZE = []() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwPageSize;
    }();

    template<typename T>
    static uintptr_t ptr_to_uintptr(T* ptr) {
        return reinterpret_cast<uintptr_t>(ptr);
    }

    // Register name tables
    static const char* reg_name[] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi"
    };

    static const char* rreg_name[] = {
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };

    // Helper to format byte as hex
    static char hex_buffer[5]; // Thread-unsafe but fine for debugging

    static const char* hexByte(const uint8_t b) {
        snprintf(hex_buffer, sizeof(hex_buffer), "0x%02X", b);
        return hex_buffer;
    };

    static void* baseAddress() {
        static void* base = GetModuleHandle(nullptr);
        return base;
    }

    static DWORD gameSize() {
        static const DWORD size = [] {
            MODULEINFO modInfo = { nullptr };
            GetModuleInformation(GetCurrentProcess(), static_cast<HMODULE>(baseAddress()), &modInfo, sizeof(modInfo));
            return modInfo.SizeOfImage;
        }();
        return size;
    }

    static uintptr_t& bakkesBase_() {
        static uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA("bakkesmod.dll"));
        return base;
    }

    static DWORD& bakkesSize_() {
        static DWORD size = []() {
            MODULEINFO modInfo = { nullptr };
            GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(bakkesBase_()), &modInfo, sizeof(modInfo));
            return modInfo.SizeOfImage;
        }();
        return size;
    }

    static void**& unrealVTablePtr_() {
        static void** ptr;
        return ptr;
    }

    template<typename... T>
    constexpr auto makePattern(T... bytes) {
        return std::array<BYTE, sizeof...(T)>{ static_cast<BYTE>(bytes)... };
    }

    inline bool hasBytes(const uint8_t* base, const uint8_t* p, size_t needed, size_t maxBytes) {
        return (p - base + needed) <= maxBytes;
    }

    // Only enough to safely walk through function prologues (mov, push, lea, sub, call, jmp, ret, etc.)
    // this is for finding patch boundaries
    // i.e. “Advance conservatively until I have ≥ N bytes without splitting instructions.”
    inline Instruction disassembleOneInstruction(const uint8_t* code, size_t maxBytes) {
        if (!code || maxBytes == 0)
            return {0, "(null)", ""};

        size_t offset = 0;

        // ---- Phase 1: prefixes (only REX, single-byte)
        uint8_t rex = 0;
        if ((code[offset] & 0xF0) == 0x40) {
            rex = code[offset];
            offset++;

            if (offset >= maxBytes)
                return {1, "db", hexByte(code[0])};
        }

        const uint8_t* p = code + offset;
        size_t remaining = maxBytes - offset;

        if (remaining < 1)
            return {offset + 1, "db", hexByte(*p)};

            // ---- Phase 2: opcode dispatch
            switch (*p) {

                // ======================
                // push r64 (50–57)
                // ======================
                case 0x50: case 0x51: case 0x52: case 0x53:
                case 0x54: case 0x55: case 0x56: case 0x57: {
                    bool rex_b = rex & 0x01;
                    const char* reg = rex_b
                        ? rreg_name[*p - 0x50]
                        : reg_name[*p - 0x50];

                    return { offset + 1, "push", reg };
                }

                // ======================
                // pop r64 (58–5F)
                // ======================
                case 0x58: case 0x59: case 0x5A: case 0x5B:
                case 0x5C: case 0x5D: case 0x5E: case 0x5F: {
                    bool rex_b = rex & 0x01;
                    const char* reg = rex_b
                        ? rreg_name[*p - 0x58]
                        : reg_name[*p - 0x58];

                    return { offset + 1, "pop", reg };
                }

                // ======================
                // mov r/m64, r64
                // ======================
                case 0x89: {
                    if (remaining < 2)
                        break;

                    return { offset + 2, "mov", "[mem], reg" };
                }

                // ======================
                // mov r64, r/m64
                // ======================
                case 0x8B: {
                    if (remaining < 2)
                        break;

                    return { offset + 2, "mov", "reg, [mem]" };
                }

                // ======================
                // lea r64, [mem]
                // ======================
                case 0x8D: {
                    if (remaining < 6)
                        break;

                    return { offset + 6, "lea", "reg, [mem]" };
                }

                // ======================
                // mov r64, imm64 (B8–BF)
                // ======================
                case 0xB8: case 0xB9: case 0xBA: case 0xBB:
                case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
                    bool rex_w = rex & 0x08;
                    size_t immSize = rex_w ? 8 : 4;

                    if (remaining < 1 + immSize)
                        break;

                    const char* reg = reg_name[*p - 0xB8];
                    return { offset + 1 + immSize, "mov", reg };
                }

                // ======================
                // ret
                // ======================
                case 0xC3:
                    return { offset + 1, "ret", "" };

                case 0xC2: {
                    if (remaining < 3)
                        break;

                    uint16_t imm = *reinterpret_cast<const uint16_t*>(p + 1);
                    static char buf[16];
                    std::snprintf(buf, sizeof(buf), "0x%04X", imm);

                    return { offset + 3, "ret", buf };
                }

                // ======================
                // 0F-prefixed instructions
                // ======================
                case 0x0F: {
                    if (remaining < 3)
                        break;

                    switch (p[1]) {
                        case 0x92: { // SETB r/m8
                            const char* reg = reg_name[p[2] & 0x7];
                            return { offset + 3, "setb", reg };
                        }
                    }
                    break;
                }
            }

        // ---- Fallback: consume exactly one byte at opcode position
        return { offset + 1, "db", hexByte(*p) };
    }

    inline void disassemble(const uint8_t* code, size_t maxBytes) {
        if (!code || maxBytes == 0) {
            printf("<disassemble skipped: invalid input>\n");
            return;
        }

        size_t offset = 0;
        while (offset < maxBytes) {
            size_t remaining = maxBytes - offset;
            Instruction insn = disassembleOneInstruction(code + offset, remaining);

            printf("%p: %-6s %-12s (%zu byte%s)\n",
                   static_cast<const void*>(code + offset),
                   insn.mnemonic,
                   insn.operands,
                   insn.length,
                   insn.length != 1 ? "s" : "");

            if (insn.length == 0 || insn.length > remaining) {
                printf("<invalid instruction detected, stopping>\n");
                break;
            }

            offset += insn.length;
            if (offset >= maxBytes) { break; }
        }
    }

    inline std::vector<uint8_t>
    copyBytes(const uint8_t* src, const size_t size) {
        std::vector<uint8_t> forged;

        forged.insert(forged.end(), src, src + size);

        return forged;
    }

    inline size_t
    findSafePatchBoundary(const uint8_t* code, size_t minSize, size_t maxBytes) {
        if (!safe::memory::isAddressAccessible(code, minSize)) {
            return 0;
        }

        size_t offset = 0;

        while (offset < minSize) {
            if (offset >= maxBytes) {
                // We've hit the max readable area — can't continue
                return 0;
            }

            constexpr size_t kMaxInstructionProbe = 16;
            if (!safe::memory::isAddressAccessible(code + offset, kMaxInstructionProbe)) {
                return 0;
            }

            const size_t remaining = maxBytes - offset;
            const Instruction insn = disassembleOneInstruction(code + offset, remaining);

            if (insn.length == 0 || insn.length > remaining) {
                return 0;
            }

            offset += insn.length;
        }

        return offset;
    }

    inline void*
    buildTrampoline(void* realAddress, size_t stolenBytes) {
        auto trampoline =
            reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, stolenBytes + 12, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

        // Copy original bytes
        memcpy(trampoline, realAddress, stolenBytes);

        // Add jump back to original + stolenBytes
        uint8_t jumpBack[] = {
            0x48, 0xB8,                        // mov rax, <realAddress+stolenBytes>
            0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0 // jmp rax
        };
        *reinterpret_cast<void**>(&jumpBack[2]) = reinterpret_cast<uint8_t*>(realAddress) + stolenBytes;
        memcpy(trampoline + stolenBytes, jumpBack, sizeof(jumpBack));

        return trampoline;
    }

    inline void writeTrampolineThatSaveRegisters(void* trampolineMemory, void* destinationPtr) {
        auto* patch = reinterpret_cast<uint8_t*>(trampolineMemory);
        int offset = 0;

        // 1. Standard function prologue (maintains stack alignment)
        patch[offset++] = 0x55;                   // push rbp
        patch[offset++] = 0x48; patch[offset++] = 0x89; patch[offset++] = 0xE5; // mov rbp, rsp

        if (isPEDetoured) {
            // 2. Save all general purpose registers
            auto& spy = g_savedRegsSpy[static_cast<int>(RegisterCapturePoint::BeforeDetourLogic)];
            emitRegisterTransfer(patch, offset, spy, false);
        }

        // 3. Prepare for jump to handler
        patch[offset++] = 0x48; patch[offset++] = 0xB8; // mov rax, destinationPtr
        std::memcpy(patch + offset, &destinationPtr, sizeof(destinationPtr));
        offset += 8;

        // 4. Call handler (pushes return address)
        patch[offset++] = 0xFF; patch[offset++] = 0xD0; // call rax

    //    // only save, no restore
    //    auto& replay = g_savedRegsReplay[static_cast<int>(RegisterCapturePoint::BeforeDetourLogic)];
    //    emitRegisterTransfer(patch, offset, replay, false);

        // 6. Standard function epilogue
        patch[offset++] = 0x48; patch[offset++] = 0x89; patch[offset++] = 0xEC; // mov rsp, rbp
        patch[offset++] = 0x5D;                   // pop rbp

        // 7. Return to original code
        patch[offset++] = 0xC3;                   // ret

        // Fill remainder with INT3
        static constexpr size_t kTrampolineSize = 128;
        std::fill(patch + offset, patch + kTrampolineSize, 0xCC);
    }

    inline void writeTrampolineThatReplaysRegisters(void* trampolineMemory, void* originalFuncPtr) {
        auto* patch = reinterpret_cast<uint8_t*>(trampolineMemory);
        int offset = 0;

        // 1. Standard prologue
        patch[offset++] = 0x55;                   // push rbp
        patch[offset++] = 0x48; patch[offset++] = 0x89; patch[offset++] = 0xE5; // mov rbp, rsp

        // 2. Restore modified registers
        auto& replay = g_savedRegsReplay[static_cast<int>(RegisterCapturePoint::BeforeDetourLogic)];
        emitRegisterTransfer(patch, offset, replay, true);

        // 3. Jump to original function
        patch[offset++] = 0x48; patch[offset++] = 0xB8; // mov rax, originalFuncPtr
        std::memcpy(patch + offset, &originalFuncPtr, sizeof(originalFuncPtr));
        offset += 8;

        // 4. Standard epilogue
        patch[offset++] = 0x48; patch[offset++] = 0x89; patch[offset++] = 0xEC; // mov rsp, rbp
        patch[offset++] = 0x5D;                   // pop rbp

        // 5. Jump to original function
        patch[offset++] = 0xFF; patch[offset++] = 0xE0; // jmp rax

        // Fill remainder with INT3
        std::fill(patch + offset, patch + 128, 0xCC);
    }

    inline void showAddressMessage(const char* title, void* address) {
        char buffer[256];
        sprintf_s(buffer, sizeof(buffer), "%s: 0x%p", title, address);
        MessageBoxA(nullptr, buffer, "Memory Address", MB_OK | MB_ICONINFORMATION);
    }

    inline uint8_t* allocateBuildTrampolines(void* inPtr, void* outPtr) {
        constexpr size_t TRAMPOLINE_SIZE = 512;
        const size_t TOTAL_SIZE = TRAMPOLINE_SIZE + 2 * PAGE_SIZE;

        // 1. Allocate
        void* basePtr = VirtualAlloc(nullptr, TRAMPOLINE_SIZE,
                                    MEM_COMMIT|MEM_RESERVE,
                                    PAGE_READWRITE);
        if (!basePtr) return nullptr;

        auto* trampInPtr = static_cast<uint8_t *>(basePtr); // + PAGE_SIZE;
        uint8_t* trampOutPtr = trampInPtr + 256;
    //    uint8_t* frontGuard = (uint8_t*)basePtr;
    //    uint8_t* rearGuard = (uint8_t*)basePtr + PAGE_SIZE + TRAMPOLINE_SIZE;
        showAddressMessage("new is updated", basePtr);

        showAddressMessage("trampoline-In Src", trampInPtr);
        showAddressMessage("trampoline-Out Src", trampOutPtr);
        showAddressMessage("trampoline-In Dest", inPtr);
        showAddressMessage("trampoline-Out Dest", outPtr);

    //    MessageBoxA(nullptr, "before fill", "", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
    //    // Fill with INT3 (0xCC) before use to detect premature execution
    //    memset(trampInPtr, 0xCC, TRAMPOLINE_SIZE);

        MessageBoxA(nullptr, "before write replay trampoline", "", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        writeTrampolineThatReplaysRegisters(trampOutPtr, outPtr);
        MessageBoxA(nullptr, "after write replay trampoline", "", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

        MessageBoxA(nullptr, "before write save trampoline", "", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        writeTrampolineThatSaveRegisters(trampInPtr, inPtr);
        MessageBoxA(nullptr, "after write save trampoline", "", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

        // 2. Flush instruction cache (important for multi-core systems)
        FlushInstructionCache(GetCurrentProcess(), basePtr, TOTAL_SIZE);
        MessageBoxA(nullptr, "after flush", "", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

        // 5. ONLY NOW set up guards
        DWORD oldProtect;
    //    VirtualProtect(frontGuard, PAGE_SIZE, PAGE_NOACCESS, &oldProtect);
    //    VirtualProtect(rearGuard, PAGE_SIZE, PAGE_NOACCESS, &oldProtect);

        // 6. Make usable region executable
        VirtualProtect(trampInPtr, TRAMPOLINE_SIZE, PAGE_EXECUTE_READ, &oldProtect);


        return trampInPtr;
    }

    inline void dumpMemoryBytes(void* addr, size_t numBytes) {
        auto* p = (uint8_t*)addr;
        for (size_t i = 0; i < numBytes; ++i) {
            //if (i % 8 == 0) log_->debug(""); // new line every 8 bytes
            //log_->logf_debug("{:02X} ", p[i]);
        }
    }

    inline void writeTrampoline(void* hopefullyAllocatedMemoryPtr, void* handleFunction) {
        const auto patch = static_cast<uint8_t*>(hopefullyAllocatedMemoryPtr);
        int offset = 0;

        // Now mov rax, handleFunction
        patch[offset++] = 0x48;
        patch[offset++] = 0xB8;
        *reinterpret_cast<void**>(patch + offset) = handleFunction;
        offset += 8;

        // jmp rax
        patch[offset++] = 0xFF;
        patch[offset++] = 0xE0;
    }

    //inline size_t
    //calculatePatchSize(void* functionAddress, size_t minSizeNeeded) {
    //    size_t size = 0;
    //    uint8_t* code = reinterpret_cast<uint8_t*>(functionAddress);
    //
    //    while (size < minSizeNeeded) {
    //        size_t instructionSize = disassembleOneInstruction(code + size);
    //        if (instructionSize == 0)
    //            break; // failed? be safe
    //        size += instructionSize;
    //    }
    //
    //    return size;
    //}

    // jump from trampoline address to find real function
    inline void*
    resolveRel32Jmp(void* addr) {
        auto bytes = (BYTE*)addr;
        if (bytes[0] != 0xE9)
            return nullptr;
        int32_t rel = *(int32_t*)(bytes + 1);
        return bytes + 5 + rel;
    }

    inline bool
    isShortJumpTo(uintptr_t from, uintptr_t expectedTo) {
        void* actual = patchutils::resolveRel32Jmp(reinterpret_cast<void*>(from));
        return reinterpret_cast<uintptr_t>(actual) == expectedTo;
    }

    inline bool
    isAbsoluteJumpTo(uintptr_t from, uintptr_t expectedTo) {
        // Instruction format:
        // 48 B8 <8-byte-addr>    → mov rax, <addr>
        // FF E0                 → jmp rax

        auto* ptr = reinterpret_cast<uint8_t*>(from);
        if (ptr[0] != 0x48 || ptr[1] != 0xB8)
            return false;

        uintptr_t actualTarget = *reinterpret_cast<uintptr_t*>(ptr + 2);

        if (ptr[10] != 0xFF || ptr[11] != 0xE0)
            return false;

        return actualTarget == expectedTo;
    }

    inline std::vector<uint8_t>
    forgeShortJumpBytes(uintptr_t from, uintptr_t to) {
        auto relOffset = static_cast<intptr_t>(to - (from + 5));

        // rel32 must be in range [-2^31, 2^31 - 1]
        if (relOffset < INT32_MIN || relOffset > INT32_MAX) {
            MessageBoxA(nullptr, "jump out of range","forgeShortJumpBytes", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
            return {}; // return empty if out of range
        }

        std::vector<uint8_t> bytes(5);
        bytes[0] = 0xE9;
        *reinterpret_cast<int32_t*>(&bytes[1]) = static_cast<int32_t>(relOffset);

        return bytes;
    }

    inline std::string
    generateMask(const BYTE* pattern, size_t patternLen) {
        std::string mask;
        mask.reserve(patternLen);
        for (size_t i = 0; i < patternLen; ++i)
            mask += (pattern[i] == 0x00) ? '?' : 'x';
        return mask;
    }

    inline void*
    patternScan(std::span<const BYTE> pattern, bool resolveJump = false) {
        //logger->debug("scanning...");
        //logger->info("RL base address: " + stringutil::toHex(reinterpret_cast<uintptr_t>(baseAddress())), LogCategory::HOOKS);
        //logger->debug("RL image size: " + stringutil::toHex(gameSize()), LogCategory::HOOKS);

        std::string maskStr = patchutils::generateMask(pattern.data(), pattern.size());
        auto* base = static_cast<uint8_t*>(baseAddress());

        if (!base || pattern.empty())
            return nullptr;

        for (size_t i = 0; i <= gameSize() - pattern.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (maskStr[j] != '?' && base[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                void* addr = &base[i];
                //logger->info("found: " + stringutil::toHex(reinterpret_cast<uintptr_t>(addr)), LogCategory::HOOKS);

                if (resolveJump) {
                    void* resolved = resolveRel32Jmp(&base[i]);
                    //logger->info("resolved to: " + stringutil::toHex(reinterpret_cast<uintptr_t>(resolved)), LogCategory::HOOKS);
                    return resolved;
                }

                return addr;
            }
        }
        return nullptr;
    }

    inline void
    alignDummy() {
        struct Dummy {
            uint8_t align[0x28];
        };
        (void)sizeof(Dummy);
    }

    inline void*
    safeDeref(void** ptr) {
        __try {
            return *ptr;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    inline std::vector<void*>
    walkTrampolineChain(void* startPtr, int maxDepth = 8) {
        std::vector<void*> chain;
        if (!startPtr) return chain;

        chain.push_back(startPtr);
        void* current = startPtr;

        for (int i = 0; i < maxDepth && current; ++i) {
            BYTE* b = reinterpret_cast<BYTE*>(current);

            BYTE firstByte;
            if (!s::readPtrSafe(b, firstByte)) {
                break;
            }

            if (firstByte == 0xE9) { // JMP rel32
                int32_t rel;
                if (!s::readPtrSafe(b + 1, rel)) {
                    break;
                }

                current = b + 5 + rel;
            }
            else if (firstByte == 0xFF) {
                BYTE secondByte;
                if (!s::readPtrSafe(b + 1, secondByte) || secondByte != 0x25) {
                    break;
                }

                int32_t relativeOffset;
                if (!s::readPtrSafe(b + 2, relativeOffset)) {
                    break;
                }

                void** ptr = reinterpret_cast<void**>(b + 6 + relativeOffset);
                current = s::derefVoidPtrSafe(ptr);

                if (!current) {
                    break;
                }
            }
            else {
                break; // not a jmp, we've hit the end of the chain
            }

            chain.push_back(current);
        }

        return chain;
    }

    inline void
    fixRipRelativeInstruction(uint8_t*& trampolinePtr, const uint8_t* origInstrPtr) {
        const uint8_t reg = (origInstrPtr[2] >> 3) & 0b111;      // Extract the destination register
        const int32_t ripOffset = *(int32_t*)(origInstrPtr + 3); // 4 byte signed offset

        auto realTarget = reinterpret_cast<uintptr_t>(origInstrPtr + 7 + ripOffset); // +7 because 48 8D/8B ?? offset (7 bytes total)

        // Emit: mov rX, imm64
        trampolinePtr[0] = 0x48;
        trampolinePtr[1] = 0xB8 + reg; // MOV rX, imm64
        *(uintptr_t*)(trampolinePtr + 2) = realTarget;

        trampolinePtr += 10; // Instruction is now 10 bytes (2 + 8)
    }

    // deobfuscatePointer:
    // usage:
    //
    // uintptr_t obfuscatedPtr = *reinterpret_cast<uintptr_t*>(RocketLeagueBase + 0x21F9680);
    // uintptr_t framePointer;
    // __asm {
    //     mov framePointer, rbp
    // }
    //
    // uintptr_t realPtr = deobfuscatePointer(obfuscatedPtr, framePointer);
    // //logger->debug("Real pointer: " + StringUtil::int_to_hex(realPtr), LogCategory::CORE);
    //
    // Load the obfuscated pointer from known address
    // Read current rbp (frame pointer register)
    // XOR them to get real pointer
    // Profit
    //
    // The frame pointer (rbp) you use must match the frame where Rocket League expects it.
    // If you call this in some random context, your rbp will be wrong.
    //
    // Ideally call this inside your ProcessEvent / ProcessInternal / CallFunction hook,
    // or immediately around there.
    inline uintptr_t
    deobfuscatePointer(uintptr_t obfuscated, const uintptr_t framePointer) {
        return obfuscated ^ framePointer;
    }

    // EXPERIMENTAL
    //
    // This code scans for a specific compiler pattern observed in Rocket League.
    // It is NOT general, NOT guaranteed, and NOT safe outside that context.
    //
    // Provided as a research breadcrumb, not production infrastructure."
    //
    // auto-fix obfuscated pointers
    // usage:
    // uintptr_t* obfuscatedPtrAddr = findStaticXorPointer(reinterpret_cast<uint8_t*>(rocketLeagueModuleBase), rocketLeagueModuleSize);
    //
    // if (obfuscatedPtrAddr) {
    //     uintptr_t obfuscatedValue = *obfuscatedPtrAddr;
    //     uintptr_t framePointer = /* capture from hook */;
    //     uintptr_t realPtr = deobfuscatePointer(obfuscatedValue, framePointer);
    //
    //     //logger->debug("Real pointer found: " + StringUtil::int_to_hex(realPtr), LogCategory::CORE);
    // } else {
    //     //logger->error("Failed to find static XOR pointer!", LogCategory::CORE);
    // }
    inline uintptr_t*
    findStaticXorPointer(uint8_t* regionStart, size_t regionSize) {
        constexpr uint8_t pattern[] = {
            0x48, 0x8B, 0x05, // mov rax, [rip+imm32]
        };
        constexpr size_t patternSize = sizeof(pattern);

        for (size_t i = 0; i < regionSize - patternSize - 4; ++i) {
            if (memcmp(regionStart + i, pattern, patternSize) == 0) {
                // Found potential match
                const int32_t ripOffset = *reinterpret_cast<int32_t*>(regionStart + i + 3);
                auto addressOfPtr = reinterpret_cast<uintptr_t>(regionStart + i + patternSize + 4 + ripOffset);
                return reinterpret_cast<uintptr_t*>(addressOfPtr);
            }
        }

        return nullptr; // Not found
    }

    inline uintptr_t
    findRetInstruction(uintptr_t address, size_t maxSearch = 512) {
        for (size_t i = 0; i < maxSearch; ++i) {
            uint8_t byte = *reinterpret_cast<uint8_t*>(address + i);
            if (byte == 0xC3) {
                return address + i;
            }
        }
        return 0; // Not found
    }

    inline size_t
    getFunctionSizeByRet(uintptr_t functionStart, size_t maxSearch = 512) {
        const auto* code = reinterpret_cast<const uint8_t*>(functionStart);
        size_t offset = 0;

        while (offset < maxSearch) {
            size_t remaining = maxSearch - offset;
            Instruction insn = disassembleOneInstruction(code + offset, remaining);

            if (insn.length == 0 || insn.length > remaining) {
                break;
            }

            // Check for RET
            if (strncmp(insn.mnemonic, "ret", 3) == 0) {
                return offset + insn.length;
            }

            offset += insn.length;
        }

        return 0;
    }

    inline bool
    isReadable(uintptr_t address, size_t size = 1) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            return false;
        }

        // Check that the entire region is committed and large enough
        if (mbi.State != MEM_COMMIT || size > mbi.RegionSize - (address - reinterpret_cast<uintptr_t>(mbi.BaseAddress))) {
            return false;
        }

        // Allow read, read/write, or executable regions
        DWORD prot = mbi.Protect;
        bool readable = (prot & PAGE_READONLY) ||
                        (prot & PAGE_READWRITE) ||
                        (prot & PAGE_EXECUTE_READ) ||
                        (prot & PAGE_EXECUTE_READWRITE);

        // Also handle "guard" and "no cache" pages carefully
        if (prot & PAGE_GUARD || prot & PAGE_NOACCESS) {
            return false;
        }

        return readable;
    }

    inline std::vector<uint8_t>
    getBytes(uintptr_t addr, size_t size) {
        const auto* src = reinterpret_cast<const uint8_t*>(addr);
        return std::vector<uint8_t>(src, src + size);
    }

    inline std::vector<uint8_t>
    getFunctionBytes(const uintptr_t addr, const size_t maxSearch = 512) {
        size_t size = getFunctionSizeByRet(addr, maxSearch);

        if (size == 0) {
            return {};
        }

        if (!isReadable(addr, size)) {
            return {};
        }

        return getBytes(addr, size);
    }

    inline void
    writeRegisterFixupStub(uint8_t* dst, void* object, void* frame, void* result, void* target) {
        uint8_t* cursor = dst;

        // mov rcx, imm64
        *cursor++ = 0x48;
        *cursor++ = 0xB9;
        memcpy(cursor, &object, 8);
        cursor += 8;

        // mov rdx, imm64
        *cursor++ = 0x48;
        *cursor++ = 0xBA;
        memcpy(cursor, &frame, 8);
        cursor += 8;

        // mov r8, imm64
        *cursor++ = 0x49;
        *cursor++ = 0xB8;
        memcpy(cursor, &result, 8);
        cursor += 8;

        // jmp [rip+0]
        *cursor++ = 0xFF;
        *cursor++ = 0x25;
        *cursor++ = 0x00;
        *cursor++ = 0x00;
        *cursor++ = 0x00;
        *cursor++ = 0x00;

        // target address (absolute)
        memcpy(cursor, &target, 8);
    }
}