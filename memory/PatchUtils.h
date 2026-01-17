#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>
#include <excpt.h>
#include <span>
#include <vector>
#include <windows.h>

#include "SafeMemory.h"
#include "Capture.h"

#include <Psapi.h>


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


// fixme, turn to class, fix log
namespace patchutils {

namespace s = safe::memory;

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

static const char* hexByte(uint8_t b) {
    snprintf(hex_buffer, sizeof(hex_buffer), "0x%02X", b);
    return hex_buffer;
};

static void* baseAddress() {
    static void* base = GetModuleHandle(NULL);
    return base;
}

static DWORD gameSize() {
    static const DWORD size = [] {
        MODULEINFO modInfo = { 0 };
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
        MODULEINFO modInfo = { 0 };
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
inline Instruction disassembleOneInstruction(const uint8_t* code, size_t maxBytes) {
    if (!code || maxBytes == 0)
        return {0, "(null)", ""};

    // Handle REX prefix (simplified)
    bool has_rex = (code[0] & 0xF0) == 0x40;
    const uint8_t rex = has_rex ? code[0] : 0;
    const uint8_t* p = code + (has_rex ? 1 : 0);

    if (!hasBytes(code, p, 1, maxBytes))
        return {1, "db", hexByte(*p)};

    switch (*p) {
        case 0x0F: {
            if (!hasBytes(code, p, 3, maxBytes)) break;
            switch (p[1]) {
                // === SETB reg (0F 92 /r)
                case 0x92: {
                    const char* reg = reg_name[p[2] & 0x7];
                    return {3, "setb", reg};
                }
                // Add more 0F-prefixed instructions as needed
            }
            break;
        }

        // push r32/r64 (0x50-0x57)
        case 0x50: case 0x51: case 0x52: case 0x53: 
        case 0x54: case 0x55: case 0x56: case 0x57: {
            const char* reg = reg_name[*p - 0x50];
            return {1, "push", reg};
        }

        // pop r32/r64 (0x58-0x5F)
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F: {
            const char* reg = reg_name[*p - 0x58];
            return {1, "pop", reg};
        }

        case 0x41: {
            if (!hasBytes(code, p, 2, maxBytes)) break;
            switch (p[1]) {
                case 0x50: case 0x51: case 0x52: case 0x53:
                case 0x54: case 0x55: case 0x56: case 0x57:
                    return {2, "push", rreg_name[p[1] - 0x50]};
                case 0x58: case 0x59: case 0x5A: case 0x5B:
                case 0x5C: case 0x5D: case 0x5E: case 0x5F:
                    return {2, "pop", rreg_name[p[1] - 0x58]};
            }
            break;
        }

        case 0x48: {
            if (!hasBytes(code, p, 2, maxBytes)) break;
            switch (p[1]) {
                case 0x89: return {3, "mov", "[mem], reg"};
                case 0x8B: return {3, "mov", "reg, [mem]"};
                case 0x8D: return {7, "lea", "reg, [mem]"};
                case 0xB8: case 0xB9: case 0xBA: case 0xBB:
                case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                    return {10, "mov", reg_name[p[1] - 0xB8]};
            }
            break;
        }

        // RET
        case 0xC3:
            return {1, "ret", ""};

        case 0xC2: {
            if (!hasBytes(code, p, 3, maxBytes)) break;
            uint16_t imm = *reinterpret_cast<const uint16_t*>(p + 1);
            static char buf[16];
            std::snprintf(buf, sizeof(buf), "0x%04X", imm);
            return {3, "ret", buf};
        }
    }

    // Unknown instruction fallback
    return {1, "db", hexByte(*p)};
}

inline void disassemble(const uint8_t* code, size_t maxBytes) {
    if (!code || maxBytes == 0) {
        printf("<disassemble skipped: invalid input>\n");
        return;
    }

    size_t offset = 0;
    while (offset < maxBytes) {
        Instruction insn = disassembleOneInstruction(code + offset, maxBytes);

        printf("%p: %-6s %-12s (%zu byte%s)\n",
               code + offset,
               insn.mnemonic,
               insn.operands,
               insn.length,
               insn.length != 1 ? "s" : "");

        if (insn.length == 0) {
            printf("<invalid instruction detected, stopping>\n");
            break;
        }

        offset += insn.length;
    }
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

inline std::vector<uint8_t>
copyBytes(const uint8_t* src, size_t size) {
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

        if (!safe::memory::isAddressAccessible(code + offset, 16)) {
            return 0;
        }

        size_t remaining = maxBytes - offset;
        Instruction insn = disassembleOneInstruction(code + offset, remaining);

        if (insn.length == 0 || insn.length > remaining) {
            return 0;
        }

        offset += insn.length;
    }

    return offset;
}

inline void*
buildTrampoline(void* realAddress, size_t stolenBytes) {
    uint8_t* trampoline =
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
    uint8_t* patch = reinterpret_cast<uint8_t*>(trampolineMemory);
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
    uint8_t* patch = reinterpret_cast<uint8_t*>(trampolineMemory);
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
    const size_t PAGE_SIZE = 4096;
    const size_t TRAMPOLINE_SIZE = 512;
    const size_t TOTAL_SIZE = TRAMPOLINE_SIZE + 2 * PAGE_SIZE;


    // 1. Allocate
    void* basePtr = VirtualAlloc(nullptr, TRAMPOLINE_SIZE, 
                                MEM_COMMIT|MEM_RESERVE, 
                                PAGE_READWRITE);
    if (!basePtr) return nullptr;

    uint8_t* trampInPtr = (uint8_t*)basePtr; // + PAGE_SIZE;
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
    uint8_t* p = (uint8_t*)addr;
    for (size_t i = 0; i < numBytes; ++i) {
        //if (i % 8 == 0) log_->debug(""); // new line every 8 bytes
        //log_->logf_debug("{:02X} ", p[i]);
    }
}


inline void writeTrampoline(void* hopefullyAllocatedMemoryPtr, void* handleFunction) {
        uint8_t* patch = reinterpret_cast<uint8_t*>(hopefullyAllocatedMemoryPtr);
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
    BYTE* bytes = (BYTE*)addr;
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
    uint8_t* ptr = reinterpret_cast<uint8_t*>(from);

    // Instruction format:
    // 48 B8 <8-byte-addr>    → mov rax, <addr>
    // FF E0                 → jmp rax

    if (ptr[0] != 0x48 || ptr[1] != 0xB8)
        return false;

    uintptr_t actualTarget = *reinterpret_cast<uintptr_t*>(ptr + 2);

    if (ptr[10] != 0xFF || ptr[11] != 0xE0)
        return false;

    return actualTarget == expectedTo;
}

inline std::vector<uint8_t>
forgeShortJumpBytes(uintptr_t from, uintptr_t to) {
    intptr_t relOffset = static_cast<intptr_t>(to - (from + 5));

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

//    auto pattern = searchPattern;
//    //logger->debug("pattern: " + reinterpret_cast<uintptr_t>(pattern.data()));
//    if (resolveJump) {
//        pattern = memory::replacePrologueWithJump(searchPattern.data(), searchPattern.size());
//        //logger->debug("pattern (replaced prologue with jump): " + reinterpret_cast<uintptr_t>(pattern.data()));
//    }
    
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
    uint8_t reg = (origInstrPtr[2] >> 3) & 0b111;      // Extract the destination register
    int32_t ripOffset = *(int32_t*)(origInstrPtr + 3); // 4 byte signed offset

    uintptr_t realTarget = (uintptr_t)(origInstrPtr + 7 + ripOffset); // +7 because 48 8D/8B ?? offset (7 bytes total)

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
// Profit 🧠💀🚀
//
//
// ✅ The frame pointer (rbp) you use must match the frame where Rocket League expects it.
// ✅ If you call this in some random context, your rbp will be wrong.
//
// ✅ Ideally call this inside your ProcessEvent / ProcessInternal / CallFunction hook,
// or immediately around there.
inline uintptr_t
deobfuscatePointer(uintptr_t obfuscated, uintptr_t framePointer) {
    return obfuscated ^ framePointer;
}

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
            int32_t ripOffset = *reinterpret_cast<int32_t*>(regionStart + i + 3);
            uintptr_t addressOfPtr = reinterpret_cast<uintptr_t>(regionStart + i + patternSize + 4 + ripOffset);
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
    const uint8_t* code = reinterpret_cast<const uint8_t*>(functionStart);
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
    const uint8_t* src = reinterpret_cast<const uint8_t*>(addr);
    return std::vector<uint8_t>(src, src + size);
}

inline std::vector<uint8_t>
getFunctionBytes(uintptr_t addr, size_t maxSearch = 512) {
    size_t size = getFunctionSizeByRet(addr, maxSearch);

    if (size == 0) {
        return {};
    }

    if (!isReadable(addr, size)) {
        return {};
    }

    return getBytes(addr, size);
}


struct TranslateResult {
    size_t bytesRead;
    size_t bytesWritten;
};


inline TranslateResult translateInstruction(uintptr_t originalAddr, const uint8_t* instrPtr, uint8_t* outPtr, size_t maxBytes) {
    if (!instrPtr || !outPtr) {
        printf("[ERROR] Null pointer in translateInstruction\n");
        return { 0, 0 };
    }

    if (!isReadable(reinterpret_cast<uintptr_t>(instrPtr), maxBytes)) {
        printf("[ERROR] Cannot read instruction at %p (maxBytes = %zu)\n", instrPtr, maxBytes);
        return { 0, 0 };
    }

    Instruction insn = disassembleOneInstruction(instrPtr, maxBytes);
    if (insn.length == 0 || insn.length > maxBytes) {
        printf("[ERROR] Failed to disassemble or invalid length at %p\n", instrPtr);
        return { 0, 0 };
    }

    uint8_t opcode = instrPtr[0];
    uint8_t* originalOutPtr = outPtr;
    
    // Track if we've handled this instruction
    bool handled = false;

    // === REX prefix handling ===
    bool hasRexPrefix = (opcode & 0xF0) == 0x40;
    uint8_t rex = hasRexPrefix ? opcode : 0;
    const uint8_t* p = instrPtr + (hasRexPrefix ? 1 : 0);
    
    // === MOV r64, [mem] with RIP-relative addressing ===
    if ((opcode == 0x48 || opcode == 0x4C) && instrPtr[1] == 0x8B) {  // MOV r64, [mem]
        uint8_t modrm = instrPtr[2]; // Get ModRM byte
        uint8_t mod = (modrm >> 6) & 0x3; // Extract mod
        uint8_t reg = (modrm >> 3) & 0x7;  // Extract reg
        uint8_t rm = modrm & 0x7;  // Extract rm

        // If Mod == 0x0 and RM == 0x5, it's a RIP-relative address
        if (mod == 0x0 && rm == 0x5) {
            try {
                int32_t disp = *reinterpret_cast<const int32_t*>(instrPtr + 3);
                uintptr_t absAddr = originalAddr + 7 + disp;

                printf("[DEBUG] MOV r64, [mem]: reg=%d, target=0x%p\n", reg, (void*)absAddr);

                // Now write: mov reg64, absAddr (safer approach)
                *outPtr++ = opcode;  // Preserve REX prefix
                *outPtr++ = 0xB8 + reg;  // mov rX, imm64
                
                // Write address byte by byte to avoid alignment issues
                for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                    *outPtr++ = static_cast<uint8_t>((absAddr >> (i * 8)) & 0xFF);
                }
                
                return {7, static_cast<size_t>(outPtr - originalOutPtr)};
            } catch (...) {
                printf("[ERROR] Exception in MOV translation\n");
                return { 0, 0 };
            }
        }
    }
    
    // === MOV [mem], r64 with RIP-relative addressing ===
    if ((opcode == 0x48 || opcode == 0x4C) && instrPtr[1] == 0x89) {  // MOV [mem], r64
        uint8_t modrm = instrPtr[2]; // Get ModRM byte
        uint8_t mod = (modrm >> 6) & 0x3; // Extract mod
        uint8_t reg = (modrm >> 3) & 0x7;  // Extract reg
        uint8_t rm = modrm & 0x7;  // Extract rm

        // If Mod == 0x0 and RM == 0x5, it's a RIP-relative address
        if (mod == 0x0 && rm == 0x5) {
            try {
                int32_t disp = *reinterpret_cast<const int32_t*>(instrPtr + 3);
                uintptr_t absAddr = originalAddr + 7 + disp;

                printf("[DEBUG] MOV [mem], r64: reg=%d, target=0x%p\n", reg, (void*)absAddr);

                // Load the absolute address into RAX
                *outPtr++ = 0x48; *outPtr++ = 0xB8;  // mov rax, imm64
                for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                    *outPtr++ = static_cast<uint8_t>((absAddr >> (i * 8)) & 0xFF);
                }
                
                // Move from the register to [rax]
                *outPtr++ = opcode;  // Preserve REX prefix
                *outPtr++ = 0x89;
                *outPtr++ = (reg << 3) | 0x00;  // ModRM for [rax]
                
                return {7, static_cast<size_t>(outPtr - originalOutPtr)};
            } catch (...) {
                printf("[ERROR] Exception in MOV [mem], r64 translation\n");
                return { 0, 0 };
            }
        }
    }
    
    // === LEA r64, [mem] with RIP-relative addressing ===
    if ((opcode == 0x48 || opcode == 0x4C) && instrPtr[1] == 0x8D) {  // LEA r64, [mem]
        uint8_t modrm = instrPtr[2]; // Get ModRM byte
        uint8_t mod = (modrm >> 6) & 0x3; // Extract mod
        uint8_t reg = (modrm >> 3) & 0x7;  // Extract reg
        uint8_t rm = modrm & 0x7;  // Extract rm

        // If Mod == 0x0 and RM == 0x5, it's a RIP-relative address
        if (mod == 0x0 && rm == 0x5) {
            try {
                int32_t disp = *reinterpret_cast<const int32_t*>(instrPtr + 3);
                uintptr_t absAddr = originalAddr + 7 + disp;

                printf("[DEBUG] LEA r64, [mem]: reg=%d, target=0x%p\n", reg, (void*)absAddr);

                // Now write: mov reg64, absAddr (safer approach)
                *outPtr++ = opcode;  // Preserve REX prefix
                *outPtr++ = 0xB8 + reg;  // mov rX, imm64
                
                // Write address byte by byte to avoid alignment issues
                for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                    *outPtr++ = static_cast<uint8_t>((absAddr >> (i * 8)) & 0xFF);
                }
                
                return {7, static_cast<size_t>(outPtr - originalOutPtr)};
            } catch (...) {
                printf("[ERROR] Exception in LEA translation\n");
                return { 0, 0 };
            }
        }
    }

    // === CMP r64, [mem] with RIP-relative addressing ===
    if ((opcode == 0x48 || opcode == 0x4C) && 
        (instrPtr[1] == 0x39 || instrPtr[1] == 0x3B || instrPtr[1] == 0x83)) {
        uint8_t modrm = instrPtr[2];
        uint8_t mod = (modrm >> 6) & 0x3;
        uint8_t reg = (modrm >> 3) & 0x7;
        uint8_t rm = modrm & 0x7;

        // Handle RIP-relative CMP
        if (mod == 0x0 && rm == 0x5) {
            try {
                int32_t disp = *reinterpret_cast<const int32_t*>(instrPtr + 3);
                uintptr_t absAddr = originalAddr + 7 + disp;

                printf("[DEBUG] CMP with RIP-relative: target=0x%p\n", (void*)absAddr);

                // Load the absolute address into RAX
                *outPtr++ = 0x48; *outPtr++ = 0xB8;  // mov rax, imm64
                for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                    *outPtr++ = static_cast<uint8_t>((absAddr >> (i * 8)) & 0xFF);
                }
                
                // Then do the comparison with that register
                if (instrPtr[1] == 0x39) {
                    // CMP [mem], reg
                    *outPtr++ = opcode;
                    *outPtr++ = 0x39;
                    *outPtr++ = (reg << 3) | 0x00;  // ModRM for [rax]
                } else if (instrPtr[1] == 0x3B) {
                    // CMP reg, [mem]
                    *outPtr++ = opcode;
                    *outPtr++ = 0x3B;
                    *outPtr++ = (reg << 3) | 0x00;  // ModRM for [rax]
                } else if (instrPtr[1] == 0x83) {
                    // CMP [mem], imm8
                    *outPtr++ = opcode;
                    *outPtr++ = 0x83;
                    *outPtr++ = (reg << 3) | 0x00;  // ModRM for [rax]
                    *outPtr++ = instrPtr[7];  // Copy the immediate value
                }
                
                return {7, static_cast<size_t>(outPtr - originalOutPtr)};
            } catch (...) {
                printf("[ERROR] Exception in CMP translation\n");
                return { 0, 0 };
            }
        }
    }
    
    // === TEST r64, [mem] with RIP-relative addressing ===
    if ((opcode == 0x48 || opcode == 0x4C) && instrPtr[1] == 0x85) {
        uint8_t modrm = instrPtr[2];
        uint8_t mod = (modrm >> 6) & 0x3;
        uint8_t reg = (modrm >> 3) & 0x7;
        uint8_t rm = modrm & 0x7;

        // Handle RIP-relative TEST
        if (mod == 0x0 && rm == 0x5) {
            try {
                int32_t disp = *reinterpret_cast<const int32_t*>(instrPtr + 3);
                uintptr_t absAddr = originalAddr + 7 + disp;

                printf("[DEBUG] TEST with RIP-relative: target=0x%p\n", (void*)absAddr);

                // Load the absolute address into RAX
                *outPtr++ = 0x48; *outPtr++ = 0xB8;  // mov rax, imm64
                for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                    *outPtr++ = static_cast<uint8_t>((absAddr >> (i * 8)) & 0xFF);
                }
                
                // Then do the test with that register
                *outPtr++ = opcode;
                *outPtr++ = 0x85;
                *outPtr++ = (reg << 3) | 0x00;  // ModRM for [rax]
                
                return {7, static_cast<size_t>(outPtr - originalOutPtr)};
            } catch (...) {
                printf("[ERROR] Exception in TEST translation\n");
                return { 0, 0 };
            }
        }
    }

    // === CALL rel32 ===
    if (opcode == 0xE8) {
        try {
            int32_t rel = *reinterpret_cast<const int32_t*>(instrPtr + 1);
            uintptr_t target = originalAddr + 5 + rel;

            printf("[DEBUG] CALL rel32: target=0x%p\n", (void*)target);

            // mov rax, target
            *outPtr++ = 0x48; *outPtr++ = 0xB8;
            
            // Write address byte by byte
            for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                *outPtr++ = static_cast<uint8_t>((target >> (i * 8)) & 0xFF);
            }
            
            // call rax
            *outPtr++ = 0xFF; *outPtr++ = 0xD0;

            return { 5, static_cast<size_t>(outPtr - originalOutPtr) };
        } catch (...) {
            printf("[ERROR] Exception in CALL translation\n");
            return { 0, 0 };
        }
    }

    // === JMP rel32 ===
    if (opcode == 0xE9) {
        try {
            int32_t rel = *reinterpret_cast<const int32_t*>(instrPtr + 1);
            uintptr_t target = originalAddr + 5 + rel;

            printf("[DEBUG] JMP rel32: target=0x%p\n", (void*)target);

            // mov rax, target
            *outPtr++ = 0x48; *outPtr++ = 0xB8;
            
            // Write address byte by byte
            for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                *outPtr++ = static_cast<uint8_t>((target >> (i * 8)) & 0xFF);
            }
            
            // jmp rax
            *outPtr++ = 0xFF; *outPtr++ = 0xE0;

            return { 5, static_cast<size_t>(outPtr - originalOutPtr) };
        } catch (...) {
            printf("[ERROR] Exception in JMP rel32 translation\n");
            return { 0, 0 };
        }
    }

    // === JMP rel8 ===
    if (opcode == 0xEB) {
        try {
            int8_t rel = *reinterpret_cast<const int8_t*>(instrPtr + 1);
            uintptr_t target = originalAddr + 2 + rel;

            printf("[DEBUG] JMP rel8: target=0x%p\n", (void*)target);

            // mov rax, target
            *outPtr++ = 0x48; *outPtr++ = 0xB8;
            
            // Write address byte by byte
            for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                *outPtr++ = static_cast<uint8_t>((target >> (i * 8)) & 0xFF);
            }
            
            // jmp rax
            *outPtr++ = 0xFF; *outPtr++ = 0xE0;

            return { 2, static_cast<size_t>(outPtr - originalOutPtr) };
        } catch (...) {
            printf("[ERROR] Exception in JMP rel8 translation\n");
            return { 0, 0 };
        }
    }

    // === Short JCC (70–7F rel8) ===
    if (opcode >= 0x70 && opcode <= 0x7F) {
        try {
            int8_t rel = *reinterpret_cast<const int8_t*>(instrPtr + 1);
            uintptr_t target = originalAddr + 2 + rel;
            uint8_t inverted = opcode ^ 0x01;  // Invert condition

            printf("[DEBUG] Short JCC: target=0x%p, inverted=0x%02X\n", (void*)target, inverted);

            // First write the inverted conditional jump that skips over our absolute jump
            *outPtr++ = inverted;
            *outPtr++ = 0x0E;  // Skip over the mov+jmp (14 bytes)

            // mov rax, target
            *outPtr++ = 0x48; *outPtr++ = 0xB8;
            
            // Write address byte by byte
            for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                *outPtr++ = static_cast<uint8_t>((target >> (i * 8)) & 0xFF);
            }
            
            // jmp rax
            *outPtr++ = 0xFF; *outPtr++ = 0xE0;

            return {2, static_cast<size_t>(outPtr - originalOutPtr)};
        } catch (...) {
            printf("[ERROR] Exception in short JCC translation\n");
            return { 0, 0 };
        }
    }

    // === Long JCC (0F 80–8F rel32) ===
    if (opcode == 0x0F && instrPtr[1] >= 0x80 && instrPtr[1] <= 0x8F) {
        try {
            uint8_t jcc = instrPtr[1];
            int32_t rel = *reinterpret_cast<const int32_t*>(instrPtr + 2);
            uintptr_t target = originalAddr + 6 + rel;
            uint8_t flipped = jcc ^ 0x01;  // Invert condition

            printf("[DEBUG] Long JCC: target=0x%p, flipped=0x%02X\n", (void*)target, flipped);

            // First write the inverted conditional jump that skips over our absolute jump
            *outPtr++ = 0x0F; 
            *outPtr++ = flipped;
            *outPtr++ = 0x0E;  // Skip over the mov+jmp (14 bytes)
            *outPtr++ = 0x00;  // Complete the 32-bit offset
            *outPtr++ = 0x00;
            *outPtr++ = 0x00;

            // mov rax, target
            *outPtr++ = 0x48; *outPtr++ = 0xB8;
            
            // Write address byte by byte
            for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                *outPtr++ = static_cast<uint8_t>((target >> (i * 8)) & 0xFF);
            }
            
            // jmp rax
            *outPtr++ = 0xFF; *outPtr++ = 0xE0;

            return {6, static_cast<size_t>(outPtr - originalOutPtr)};
        } catch (...) {
            printf("[ERROR] Exception in long JCC translation\n");
            return { 0, 0 };
        }
    }
    
    // === MOVSS/MOVSD with RIP-relative addressing ===
    if (opcode == 0xF3 && instrPtr[1] == 0x0F && instrPtr[2] == 0x10) {
        uint8_t modrm = instrPtr[3];
        uint8_t mod = (modrm >> 6) & 0x3;
        uint8_t reg = (modrm >> 3) & 0x7;
        uint8_t rm = modrm & 0x7;

        if (mod == 0x0 && rm == 0x5) {
            try {
                int32_t disp = *reinterpret_cast<const int32_t*>(instrPtr + 4);
                uintptr_t absAddr = originalAddr + 8 + disp;

                printf("[DEBUG] MOVSS/MOVSD with RIP-relative: target=0x%p\n", (void*)absAddr);

                // Load the absolute address into RAX
                *outPtr++ = 0x48; *outPtr++ = 0xB8;  // mov rax, imm64
                for (size_t i = 0; i < sizeof(uintptr_t); i++) {
                    *outPtr++ = static_cast<uint8_t>((absAddr >> (i * 8)) & 0xFF);
                }
                
                // Then do the MOVSS/MOVSD with that register
                *outPtr++ = 0xF3;
                *outPtr++ = 0x0F;
                *outPtr++ = 0x10;
                *outPtr++ = (reg << 3) | 0x00;  // ModRM for [rax]
                
                return {8, static_cast<size_t>(outPtr - originalOutPtr)};
            } catch (...) {
                printf("[ERROR] Exception in MOVSS/MOVSD translation\n");
                return { 0, 0 };
            }
        }
    }

    // === Default: raw copy (safely) ===
    try {
        // Only log raw copy for instructions longer than 1 byte
        static size_t rawCopyCount = 0;
        if (insn.length > 1) {
            printf("[DEBUG] Raw Copy: %zu bytes (instruction: %s %s)\n", 
                   insn.length, insn.mnemonic, insn.operands);
        } else {
            // For single-byte instructions, batch the logging
            rawCopyCount++;
            if (rawCopyCount % 10 == 0) {
                printf("[DEBUG] Raw copied %zu single-byte instructions\n", rawCopyCount);
            }
        }
        
        for (size_t i = 0; i < insn.length; i++) {
            outPtr[i] = instrPtr[i];
        }
        
        return { insn.length, insn.length };
    } catch (...) {
        printf("[ERROR] Exception in raw copy\n");
        return { 0, 0 };
    }
}

inline void* buildTranslatedFunction(uintptr_t originalAddr, const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        printf("[ERROR] Empty byte array passed to buildTranslatedFunction\n");
        return nullptr;
    }

    // Allocate more space than needed to account for instruction expansion
    const size_t estimatedLen = bytes.size() * 4 + 1024; 
    uint8_t* newFn = reinterpret_cast<uint8_t*>(
        VirtualAlloc(nullptr, estimatedLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
    );

    if (!newFn) {
        printf("[ERROR] Failed to allocate memory for translated function\n");
        return nullptr;
    }

    // Fill with INT3 (breakpoint) to catch any execution errors
    memset(newFn, 0xCC, estimatedLen);

    uint8_t* writeCursor = newFn;
    const uint8_t* readCursor = bytes.data();
    size_t offset = 0;
    size_t totalBytesWritten = 0;
    size_t instructionsTranslated = 0;
    size_t relativeInstructionsFixed = 0;
    
    // Add a safety counter to prevent infinite loops
    size_t safetyCounter = 0;
    const size_t MAX_INSTRUCTIONS = 2000;
    
    printf("[INFO] Starting translation of function at 0x%p (%zu bytes)\n", 
           (void*)originalAddr, bytes.size());
    
    while (offset < bytes.size() && safetyCounter++ < MAX_INSTRUCTIONS) {
        // Check if we're getting close to the end of our buffer
        if (totalBytesWritten > estimatedLen - 128) {
            printf("[ERROR] Translation buffer nearly full - aborting\n");
            VirtualFree(newFn, 0, MEM_RELEASE);
            return nullptr;
        }
        
        TranslateResult result;
        
        try {
            size_t remaining = bytes.size() - offset;
            result = translateInstruction(originalAddr + offset, readCursor + offset, writeCursor, remaining);
            instructionsTranslated++;
            
            // Check if this was a relative instruction that we fixed
            if (result.bytesWritten > result.bytesRead) {
                relativeInstructionsFixed++;
            }
        } catch (...) {
            printf("[ERROR] Exception during instruction translation at offset %zu\n", offset);
            VirtualFree(newFn, 0, MEM_RELEASE);
            return nullptr;
        }
        
        if (result.bytesRead == 0 || result.bytesWritten == 0) {
            printf("[ERROR] Failed to translate instruction at offset %zu\n", offset);
            // Add a return instruction to safely terminate execution
            *writeCursor = 0xC3;  // RET
            break;
        }
        
        offset += result.bytesRead;
        writeCursor += result.bytesWritten;
        totalBytesWritten += result.bytesWritten;
        
        // Check for RET instruction - we might be done
        if (readCursor[offset-1] == 0xC3 || 
            (readCursor[offset-2] == 0xC2 && offset >= 2)) {
            printf("[INFO] Found return instruction at offset %zu\n", offset-1);
            break;
        }
    }
    
    // Add a safety return at the end if we didn't end with one
    if (offset > 0 && readCursor[offset-1] != 0xC3) {
        *writeCursor = 0xC3;  // RET
        totalBytesWritten++;
    }
    
    printf("[INFO] Translation complete: %zu bytes read, %zu bytes written\n", 
           offset, totalBytesWritten);
    printf("[INFO] Translated %zu instructions, fixed %zu relative instructions\n",
           instructionsTranslated, relativeInstructionsFixed);
    
    // Make the memory executable
    DWORD oldProtect;
    if (!VirtualProtect(newFn, totalBytesWritten, PAGE_EXECUTE_READ, &oldProtect)) {
        printf("[ERROR] Failed to make memory executable\n");
        VirtualFree(newFn, 0, MEM_RELEASE);
        return nullptr;
    }
    
    // Flush instruction cache to ensure CPU sees our changes
    FlushInstructionCache(GetCurrentProcess(), newFn, totalBytesWritten);
    return newFn;
}


}
