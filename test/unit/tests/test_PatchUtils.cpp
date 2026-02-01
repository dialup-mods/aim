#pragma once
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <Windows.h>
#include <psapi.h>
#include <array>

#include "PatchUtils.h"

namespace p = patchutils;

alignas(16) uint8_t fakeMemory[64];

LONG CALLBACK Veh(EXCEPTION_POINTERS* e) {
    printf(
        "CRASH\nRIP=%p\nRSP=%p\n",
        (void*)e->ContextRecord->Rip,
        (void*)e->ContextRecord->Rsp
    );
    return EXCEPTION_EXECUTE_HANDLER;
}
void installVeh() {
    AddVectoredExceptionHandler(1, Veh);
}


TEST_CASE("copyBytes copies exact contents") {
    uint8_t src[] = { 1, 2, 3, 4, 5 };

    auto out = p::copyBytes(src, sizeof(src));

    CHECK(out.size() == 5);
    CHECK(std::equal(out.begin(), out.end(), src));
}

TEST_CASE("copyBytes handles empty input") {
    constexpr uint8_t src[] = { 0xAA };

    const auto out = p::copyBytes(src, 0);

    CHECK(out.empty());
}

TEST_CASE("makePattern produces correct byte array") {
    constexpr auto pat = p::makePattern(0x48, 0x89, 0xE5);

    REQUIRE(pat.size() == 3);
    CHECK(pat[0] == 0x48);
    CHECK(pat[1] == 0x89);
    CHECK(pat[2] == 0xE5);
}

TEST_CASE("hasBytes respects bounds correctly") {
    uint8_t buf[16]{};

    const uint8_t* base = buf;
    const uint8_t* p    = buf + 4;

    CHECK(p::hasBytes(base, p, 4, 16));   // 4+4 = 8 <= 16
    CHECK_FALSE(p::hasBytes(base, p, 13, 16)); // 4+13 = 17 > 16
}

TEST_CASE("disassembleOneInstruction handles null input") {
    p::Instruction instruction = p::disassembleOneInstruction(nullptr, 10);

    CHECK(instruction.length == 0);
    CHECK(std::string(instruction.mnemonic) == "(null)");
}

// critical for forward progress, you always consume at least 1 byte
TEST_CASE("unknown opcode falls back to db") {
    uint8_t code[] = { 0xFF };

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 1);
    CHECK(std::string(instruction.mnemonic) == "db");
}

TEST_CASE("push rax disassembles correctly") {
    uint8_t code[] = { 0x50 }; // push rax

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 1);
    CHECK(std::string(instruction.mnemonic) == "push");
    CHECK(std::string(instruction.operands) == "rax");
}

TEST_CASE("pop rbx disassembles correctly") {
    uint8_t code[] = { 0x5B }; // pop rbx

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 1);
    CHECK(std::string(instruction.mnemonic) == "pop");
    CHECK(std::string(instruction.operands) == "rbx");
}

TEST_CASE("REX push r8 disassembles correctly") {
    uint8_t code[] = { 0x41, 0x50 }; // push r8

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 2);
    CHECK(std::string(instruction.mnemonic) == "push");
}

// classic prologue
TEST_CASE("mov rbp, rsp pattern is recognized") {
    uint8_t code[] = { 0x48, 0x89, 0xE5 };

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 3);
    CHECK(std::string(instruction.mnemonic) == "mov");
}

TEST_CASE("lea instruction consumes expected length") {
    uint8_t code[] = {
        0x48, 0x8D, 0x05,
        0x11, 0x22, 0x33, 0x44
    };

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 7);
    CHECK(std::string(instruction.mnemonic) == "lea");
}

TEST_CASE("ret disassembles correctly") {
    uint8_t code[] = { 0xC3 };

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 1);
    CHECK(std::string(instruction.mnemonic) == "ret");
}

TEST_CASE("ret imm16 disassembles correctly") {
    uint8_t code[] = { 0xC2, 0x34, 0x12 };

    p::Instruction instruction = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(instruction.length == 3);
    CHECK(std::string(instruction.mnemonic) == "ret");
}

TEST_CASE("instruction does not read past maxBytes") {
    uint8_t code[] = { 0x48 }; // incomplete REX prefix

    p::Instruction instruction = p::disassembleOneInstruction(code, 1);

    CHECK(instruction.length == 1); // fallback db
    CHECK(std::string(instruction.mnemonic) == "db");
}

TEST_CASE("disassemble walks multiple instructions safely") {
    uint8_t code[] = {
        0x55,             // push rbp
        0x48, 0x89, 0xE5, // mov rbp, rsp
        0xC3              // ret
    };

    p::disassemble(code, sizeof(code));

    CHECK(true); // if we got here, it didn't explode
}

TEST_CASE("disassemble smoke: simple function prologue") {
    uint8_t code[] = {
        0x55,                   // push rbp
        0x48, 0x89, 0xE5,       // mov rbp, rsp
        0x48, 0x83, 0xEC, 0x20, // sub rsp, 0x20 (will likely fallback/db)
        0xC3                    // ret
    };

    p::disassemble(code, sizeof(code));

    CHECK(true); // if we got here, it didn't explode
}

TEST_CASE("findSafePatchBoundary: exact instruction boundary") {
    uint8_t code[] = {
        0x55,                   // push rbp
        0x48, 0x89, 0xE5        // mov rbp, rsp
    };

    size_t result = p::findSafePatchBoundary(code, 4, sizeof(code));
    CHECK(result == 4);
}

TEST_CASE("findSafePatchBoundary: rounds up to instruction boundary") {
    uint8_t code[] = {
        0x55,                   // 1
        0x48, 0x89, 0xE5,       // 3
        0xC3                    // 1
    };

    // minSize cuts through mov
    size_t result = p::findSafePatchBoundary(code, 2, sizeof(code));

    // Must return 4, not 2 or 3
    CHECK(result == 4);
}

TEST_CASE("findSafePatchBoundary: unknown opcode still advances") {
    uint8_t code[] = {
        0xFF, 0xFF, 0xFF, 0xC3
    };

    size_t result = p::findSafePatchBoundary(code, 3, sizeof(code));

    CHECK(result >= 3);
}

// NOTE: we are not asserting valid x86 semantics
// we are asserting "safe byte clipping"
TEST_CASE("findSafePatchBoundary: lone byte still advances safely") {
    constexpr uint8_t code[] = { 0x48 };
    const size_t result = p::findSafePatchBoundary(code, 1, sizeof(code));
    CHECK(result == 1);
}

TEST_CASE("findSafePatchBoundary: respects maxBytes limit") {
    const uint8_t code[] = {
        0x55,
        0x48, 0x89, 0xE5,
        0xC3
    };

    const size_t result = p::findSafePatchBoundary(code, 4, 3);
    CHECK(result == 0);
}

TEST_CASE("findSafePatchBoundary: zero minSize returns zero") {
    constexpr uint8_t code[] = { 0x90 };
    const size_t result = p::findSafePatchBoundary(code, 0, sizeof(code));
    CHECK(result == 0);
}

TEST_CASE("findSafePatchBoundary: truncated multi-byte instruction fails") {
    constexpr uint8_t code[] = {
        0x48, 0x89 // start of mov, missing ModRM
    };

    const auto result = p::findSafePatchBoundary(code, 3, sizeof(code));
    CHECK(result == 0);
}

TEST_CASE("deobfuscatePointer xor is reversible") {
    uintptr_t real = 0x12345678;
    uintptr_t frame = 0xABCDEF00;

    uintptr_t obf = real ^ frame;
    uintptr_t out = p::deobfuscatePointer(obf, frame);

    CHECK(out == real);
}

extern "C" __declspec(noinline)
int __fastcall testTarget(int a, int b) {
    return a + b;
}

extern "C" __declspec(noinline)
int __fastcall handler(int a, int b) {
    return a + b;
}

TEST_CASE("find real prologue") {
    auto fn = reinterpret_cast<uint8_t*>(&testTarget);

    printf("Function bytes:\n");
    for (int i = 0; i < 32; i++) {
        printf("%02X ", fn[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    // Follow the first jump
    if (fn[0] == 0xE9) {
        int32_t offset = *reinterpret_cast<int32_t*>(fn + 1);
        uint8_t* target = fn + 5 + offset;
        printf("\nFirst jump (E9) goes to: 0x%p\n", target);
        printf("Target bytes:\n");
        for (int i = 0; i < 32; i++) {
            printf("%02X ", target[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
    }
}

//extern "C" __declspec(noinline)
//int __fastcall testTargetAsm(int a, int b) {
//    __asm {
//        push rbp
//        mov rbp, rsp
//        mov eax, ecx
//        add eax, edx
//        pop rbp
//        ret
//    }
//}

TEST_CASE("trampoline with proper instruction relocation") {
    installVeh();

    auto fn = reinterpret_cast<uint8_t*>(&testTarget);

    // Resolve through jump chain to real function body
    uint8_t* realFn = static_cast<uint8_t*>(p::resolveExecutableEntry(fn));

    printf("Original entry: 0x%p\n", fn);
    printf("Real function:  0x%p\n", realFn);
    printf("Real function bytes:\n");
    for (int i = 0; i < 32; i++) {
        printf("%02X ", realFn[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    // Find safe patch boundary (need at least 14 bytes for indirect jump)
    constexpr size_t kHookSize = 14;  // FF 25 00 00 00 00 + 8-byte addr
    size_t bytesToCapture = p::findSafePatchBoundary(realFn, kHookSize, 64);

    REQUIRE(bytesToCapture > 0);
    printf("\nCapturing %zu bytes\n", bytesToCapture);

    uint8_t* slack = (uint8_t*)VirtualAlloc(
        nullptr, 256,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    // Build trampoline with instruction relocation
    uint8_t* trampolinePtr = slack;
    size_t offset = 0;

    while (offset < bytesToCapture) {
        const p::Instruction insn = p::disassembleOneInstruction(realFn + offset, bytesToCapture - offset);
        REQUIRE(insn.length > 0);

        uint8_t* origInstr = realFn + offset;

        // Check for instructions that need relocation
        if (origInstr[0] == 0xE9) {
            // Near jump - relocate offset
            int32_t oldOffset = *reinterpret_cast<int32_t*>(origInstr + 1);
            uintptr_t targetAddr = reinterpret_cast<uintptr_t>(origInstr + 5 + oldOffset);
            int32_t newOffset = targetAddr - (reinterpret_cast<uintptr_t>(trampolinePtr) + 5);

            *trampolinePtr++ = 0xE9;
            *reinterpret_cast<int32_t*>(trampolinePtr) = newOffset;
            trampolinePtr += 4;
        }
        else if (insn.length == 7 && origInstr[0] == 0x48 && (origInstr[1] == 0x8D || origInstr[1] == 0x8B)) {
            // RIP-relative LEA/MOV - use your helper
            p::fixRipRelativeInstruction(trampolinePtr, origInstr);
        }
        else if (origInstr[0] == 0xFF && origInstr[1] == 0x15) {
            // CALL [RIP+offset] - relocate
            int32_t oldOffset = *reinterpret_cast<int32_t*>(origInstr + 2);
            uintptr_t targetAddr = reinterpret_cast<uintptr_t>(origInstr + 6 + oldOffset);
            int32_t newOffset = targetAddr - (reinterpret_cast<uintptr_t>(trampolinePtr) + 6);

            *trampolinePtr++ = 0xFF;
            *trampolinePtr++ = 0x15;
            *reinterpret_cast<int32_t*>(trampolinePtr) = newOffset;
            trampolinePtr += 4;
        }
        else {
            // Copy instruction as-is
            memcpy(trampolinePtr, origInstr, insn.length);
            trampolinePtr += insn.length;
        }

        offset += insn.length;
    }

    // Add jump back to continuation
    *trampolinePtr++ = 0xFF;
    *trampolinePtr++ = 0x25;
    *reinterpret_cast<int32_t*>(trampolinePtr) = 0;
    trampolinePtr += 4;
    *reinterpret_cast<uintptr_t*>(trampolinePtr) = reinterpret_cast<uintptr_t>(realFn + bytesToCapture);
    trampolinePtr += 8;

    printf("\nTrampoline bytes (%zu bytes):\n", trampolinePtr - slack);
    for (size_t i = 0; i < (trampolinePtr - slack); i++) {
        printf("%02X ", slack[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    // Patch original function
    DWORD oldProtect;
    VirtualProtect(realFn, bytesToCapture, PAGE_EXECUTE_READWRITE, &oldProtect);

    // Write indirect jump to trampoline
    realFn[0] = 0xFF;
    realFn[1] = 0x25;
    *reinterpret_cast<int32_t*>(realFn + 2) = 0;
    *reinterpret_cast<uintptr_t*>(realFn + 6) = reinterpret_cast<uintptr_t>(slack);

    // NOP out remaining captured bytes
    for (size_t i = 14; i < bytesToCapture; i++) {
        realFn[i] = 0x90;  // NOP
    }

    VirtualProtect(realFn, bytesToCapture, oldProtect, &oldProtect);

    printf("Patched function:\n");
    for (size_t i = 0; i < 32; i++) {
        printf("%02X ", realFn[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    int result = testTarget(4, 7);
    printf("\nCall succeeded: %d\n", result);
    CHECK(result == 11);

    VirtualFree(slack, 0, MEM_RELEASE);
}

static int __fastcall testTarget1(int a, int b) { return a + b; }
TEST_CASE("debug resolveExecutableEntry") {
    auto fn = reinterpret_cast<uint8_t*>(&testTarget1);

    printf("testTarget function pointer: 0x%p\n", fn);

    // Check if it's readable
    if (!safe::memory::isAddressAccessible(fn, 16)) {
        printf("ERROR: fn is not accessible!\n");
    } else {
        printf("fn is accessible, first bytes:\n");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", fn[i]);
        }
        printf("\n");
    }

    // Walk the trampoline chain manually
    printf("\nWalking trampoline chain:\n");
    auto chain = p::walkTrampolineChain(fn);
    for (size_t i = 0; i < chain.size(); i++) {
        printf("  [%zu] 0x%p", i, chain[i]);
        if (safe::memory::isAddressAccessible(chain[i], 16)) {
            printf(" - ");
            uint8_t* p = static_cast<uint8_t*>(chain[i]);
            for (int j = 0; j < 8; j++) {
                printf("%02X ", p[j]);
            }
        } else {
            printf(" - NOT ACCESSIBLE");
        }
        printf("\n");
    }

    // Try resolveExecutableEntry
    void* resolved = p::resolveExecutableEntry(fn);
    printf("\nresolveExecutableEntry returned: 0x%p\n", resolved);

    if (!safe::memory::isAddressAccessible(resolved, 16)) {
        printf("ERROR: resolved address is NOT accessible!\n");
        FAIL("resolveExecutableEntry returned invalid address");
    } else {
        printf("Resolved address is accessible, first bytes:\n");
        uint8_t* p = static_cast<uint8_t*>(resolved);
        for (int i = 0; i < 16; i++) {
            printf("%02X ", p[i]);
        }
        printf("\n");
    }
}

static int __fastcall testTarget2(int a, int b) { return a + b; }
TEST_CASE("debug buildRIPTrampoline") {
    installVeh();

    auto fn = reinterpret_cast<uint8_t*>(&testTarget2);
    uint8_t* realFn = static_cast<uint8_t*>(p::resolveExecutableEntry(fn));

    printf("Real function at: 0x%p\n", realFn);
    printf("Real function bytes (first 64):\n");
    for (int i = 0; i < 64; i++) {
        printf("%02X ", realFn[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    // Manually trace through what buildRIPTrampoline does
    size_t stolenBytes = p::findSafePatchBoundary(realFn, 14, 64);
    printf("\nStolen bytes: %zu\n", stolenBytes);

    if (stolenBytes == 0) {
        FAIL("findSafePatchBoundary returned 0");
    }

    // Show what instructions we're stealing
    printf("\nInstructions being stolen:\n");
    size_t offset = 0;
    while (offset < stolenBytes) {
        p::Instruction insn = p::disassembleOneInstruction(realFn + offset, stolenBytes - offset);
        printf("  +%zu: ", offset);
        for (size_t i = 0; i < insn.length; i++) {
            printf("%02X ", realFn[offset + i]);
        }
        printf("(len=%zu)\n", insn.length);
        offset += insn.length;
    }

    // Now build it
    size_t actualStolen = 0;
    uint8_t* trampoline = p::buildRIPTrampoline(fn, &actualStolen);

    REQUIRE(trampoline != nullptr);
    printf("\nTrampoline at: 0x%p\n", trampoline);
    printf("Trampoline bytes:\n");
    for (size_t i = 0; i < 64; i++) {
        printf("%02X ", trampoline[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    printf("\nExpected flow:\n");
    printf("1. Execute stolen instructions (relocated)\n");
    printf("2. Jump to realFn+%zu (0x%p)\n", actualStolen, realFn + actualStolen);

    // Check if continuation address is valid
    printf("\nContinuation bytes at 0x%p:\n", realFn + actualStolen);
    for (int i = 0; i < 16; i++) {
        printf("%02X ", realFn[actualStolen + i]);
    }
    printf("\n");
}

static int __fastcall testTarget3(int a, int b) { return a + b; }
TEST_CASE("buildRIPTrampoline - simple function call") {
    installVeh();

    // Verify original behavior
    REQUIRE(testTarget(2, 3) == 5);
    REQUIRE(testTarget(10, 20) == 30);

    auto fn = reinterpret_cast<uint8_t*>(&testTarget3);
    uint8_t* realFn = static_cast<uint8_t*>(p::resolveExecutableEntry(fn));

    printf("Testing trampoline for function at: 0x%p\n", realFn);
    printf("Original bytes:\n");
    for (int i = 0; i < 32; i++) {
        printf("%02X ", realFn[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    // Build trampoline
    size_t stolenBytes = 0;
    uint8_t* trampoline = p::buildRIPTrampoline(fn, &stolenBytes);

    REQUIRE(trampoline != nullptr);
    REQUIRE(stolenBytes >= 14);

    printf("\nStole %zu bytes\n", stolenBytes);
    printf("Trampoline at: 0x%p\n", trampoline);
    printf("Trampoline bytes:\n");
    for (size_t i = 0; i < 64; i++) {
        printf("%02X ", trampoline[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    // Test calling through trampoline directly
    auto trampolineFunc = reinterpret_cast<int(__fastcall*)(int, int)>(trampoline);

    int result = trampolineFunc(7, 8);
    printf("\nDirect trampoline call: trampolineFunc(7, 8) = %d\n", result);
    CHECK(result == 15);

    // Now patch the original function to jump elsewhere
    uint8_t* hookHandler = (uint8_t*)VirtualAlloc(
        nullptr, 64,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    // Simple handler that doubles the result
    // mov eax, ecx
    // add eax, edx
    // add eax, eax  ; double it
    // ret
    hookHandler[0] = 0x89; hookHandler[1] = 0xC8;  // mov eax, ecx
    hookHandler[2] = 0x01; hookHandler[3] = 0xD0;  // add eax, edx
    hookHandler[4] = 0x01; hookHandler[5] = 0xC0;  // add eax, eax
    hookHandler[6] = 0xC3;                         // ret

    // Patch original to jump to handler
    p::writeIndirectJump(realFn, hookHandler);

    printf("\nPatched function bytes:\n");
    for (int i = 0; i < 32; i++) {
        printf("%02X ", realFn[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    // Call through original (now hooked) - should double
    int hookedResult = testTarget3(3, 4);
    printf("\nHooked call: testTarget(3, 4) = %d (expected 14, doubled from 7)\n", hookedResult);
    CHECK(hookedResult == 14);

    // Call through trampoline - should still return original behavior
    int trampolineResult = trampolineFunc(3, 4);
    printf("Trampoline call: trampolineFunc(3, 4) = %d (expected 7, original)\n", trampolineResult);
    CHECK(trampolineResult == 7);

    VirtualFree(trampoline, 0, MEM_RELEASE);
    VirtualFree(hookHandler, 0, MEM_RELEASE);
}

__declspec(noinline)
static int __fastcall testTarget4(int a, int b) {
    return a + b;
}
TEST_CASE("buildRIPTrampoline - use in actual hook pattern") {
    installVeh();

    REQUIRE(testTarget4(5, 10) == 15);

    struct Hook {
        void* target_;
        uint8_t* trampoline_;
        size_t patchSize_;
        uint8_t originalBytes_[32];

        ~Hook() {
            if (target_) {
                DWORD old;
                VirtualProtect(target_, patchSize_, PAGE_EXECUTE_READWRITE, &old);
                memcpy(target_, originalBytes_, patchSize_);
                VirtualProtect(target_, patchSize_, old, &old);
            }
            if (trampoline_) {
                VirtualFree(trampoline_, 0, MEM_RELEASE);
            }
        }
    };

    Hook hook;
    hook.target_ = (void*)&testTarget4;
    hook.trampoline_ = p::buildRIPTrampoline(hook.target_, &hook.patchSize_);

    printf("Target before building trampoline: 0x%p\n", hook.target_);
    for (int i = 0; i < 32; i++) {
        printf("%02X ", ((uint8_t*)hook.target_)[i]);
    }
    printf("\n");

    REQUIRE(hook.trampoline_ != nullptr);

    // Save original bytes for restore
    memcpy(hook.originalBytes_, hook.target_, hook.patchSize_);

    // Create a handler that adds 100 to the result
    static uint8_t* s_trampoline = hook.trampoline_;
    auto handler = +[](int a, int b) -> int {
        auto original = reinterpret_cast<int(__fastcall*)(int, int)>(s_trampoline);
        return original(a, b) + 100;
    };
    printf("Testing handler directly:\n");
    auto testFunc = reinterpret_cast<int(__fastcall*)(int, int)>(hook.trampoline_);
    int directResult = testFunc(5, 10);
    printf("Direct trampoline call: %d\n", directResult);

    int handlerResult = handler(5, 10);
    printf("Direct handler call: %d (should be %d + 100)\n", handlerResult, directResult);

    // Install hook
    p::writeIndirectJump(hook.target_, (void*)handler);

    printf("After installing hook:\n");
    uint8_t* target = static_cast<uint8_t*>(hook.target_);
    for (int i = 0; i < 32; i++) {
        printf("%02X ", target[i]);
    }
    printf("\n");

    printf("Handler address: 0x%p\n", (void*)handler);
    printf("Expected jump target in bytes 6-13: ");
    uint64_t expected = (uint64_t)(void*)handler;
    for (int i = 0; i < 8; i++) {
        printf("%02X ", ((uint8_t*)&expected)[i]);
    }
    printf("\n");

    if (&testTarget4 != hook.target_) {
        printf("MISMATCH! Calling testTarget4 won't hit the hook!\n");

        // Show what &testTarget4 actually points to
        uint8_t* fn = reinterpret_cast<uint8_t*>(&testTarget4);
        printf("&testTarget4 points to: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", fn[i]);
        }
        printf("\n");
    }
    printf("\nVerifying patch is still in place:\n");
    uint8_t* checkTarget = static_cast<uint8_t*>(hook.target_);
    for (int i = 0; i < 16; i++) {
        printf("%02X ", checkTarget[i]);
    }
    printf("\n");

    printf("About to call testTarget4(5, 10)...\n");

    int result = testTarget4(5, 10);
    printf("Hooked result: testTarget4(5, 10) = %d (expected 115)\n", result);
    auto funcPtr = &testTarget4;
    result = funcPtr(5, 10);
    CHECK(result == 115);

    // Hook destructor will restore original
}