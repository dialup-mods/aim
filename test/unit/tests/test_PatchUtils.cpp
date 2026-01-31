#pragma once
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <array>
#include "PatchUtils.h"
namespace p = patchutils;

alignas(16) uint8_t fakeMemory[64];


TEST_CASE("copyBytes copies exact contents") {
    uint8_t src[] = { 1, 2, 3, 4, 5 };

    auto out = p::copyBytes(src, sizeof(src));

    CHECK(out.size() == 5);
    CHECK(std::equal(out.begin(), out.end(), src));
}

TEST_CASE("copyBytes handles empty input") {
    const uint8_t src[] = { 0xAA };

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
    p::Instruction insn = p::disassembleOneInstruction(nullptr, 10);

    CHECK(insn.length == 0);
    CHECK(std::string(insn.mnemonic) == "(null)");
}

// critical for forward progress, you always consume at least 1 byte
TEST_CASE("unknown opcode falls back to db") {
    uint8_t code[] = { 0xFF };

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 1);
    CHECK(std::string(insn.mnemonic) == "db");
}

TEST_CASE("push rax disassembles correctly") {
    uint8_t code[] = { 0x50 }; // push rax

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 1);
    CHECK(std::string(insn.mnemonic) == "push");
    CHECK(std::string(insn.operands) == "rax");
}

TEST_CASE("pop rbx disassembles correctly") {
    uint8_t code[] = { 0x5B }; // pop rbx

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 1);
    CHECK(std::string(insn.mnemonic) == "pop");
    CHECK(std::string(insn.operands) == "rbx");
}

TEST_CASE("REX push r8 disassembles correctly") {
    uint8_t code[] = { 0x41, 0x50 }; // push r8

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 2);
    CHECK(std::string(insn.mnemonic) == "push");
}

// classic prologue
TEST_CASE("mov rbp, rsp pattern is recognized") {
    uint8_t code[] = { 0x48, 0x89, 0xE5 };

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 3);
    CHECK(std::string(insn.mnemonic) == "mov");
}

TEST_CASE("lea instruction consumes expected length") {
    uint8_t code[] = {
        0x48, 0x8D, 0x05,
        0x11, 0x22, 0x33, 0x44
    };

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 7);
    CHECK(std::string(insn.mnemonic) == "lea");
}

TEST_CASE("ret disassembles correctly") {
    uint8_t code[] = { 0xC3 };

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 1);
    CHECK(std::string(insn.mnemonic) == "ret");
}

TEST_CASE("ret imm16 disassembles correctly") {
    uint8_t code[] = { 0xC2, 0x34, 0x12 };

    p::Instruction insn = p::disassembleOneInstruction(code, sizeof(code));

    CHECK(insn.length == 3);
    CHECK(std::string(insn.mnemonic) == "ret");
}

TEST_CASE("instruction does not read past maxBytes") {
    uint8_t code[] = { 0x48 }; // incomplete REX prefix

    p::Instruction insn = p::disassembleOneInstruction(code, 1);

    CHECK(insn.length == 1); // fallback db
    CHECK(std::string(insn.mnemonic) == "db");
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

// we are not asserting valid x86 semantics
// we are asserting "safe byte clipping"
TEST_CASE("findSafePatchBoundary: lone byte still advances safely") {
    uint8_t code[] = { 0x48 };

    const size_t result = p::findSafePatchBoundary(code, 1, sizeof(code));

    CHECK(result == 1);
}

TEST_CASE("findSafePatchBoundary: respects maxBytes limit") {
    const uint8_t code[] = {
        0x55,
        0x48, 0x89, 0xE5,
        0xC3
    };

    size_t result = p::findSafePatchBoundary(code, 4, 3);

    CHECK(result == 0);
}

TEST_CASE("findSafePatchBoundary: zero minSize returns zero") {
    uint8_t code[] = { 0x90 };

    size_t result = p::findSafePatchBoundary(code, 0, sizeof(code));

    CHECK(result == 0);
}

TEST_CASE("findSafePatchBoundary: truncated multi-byte instruction fails") {
    uint8_t code[] = {
        0x48, 0x89 // start of mov, missing ModRM
    };

    size_t result = p::findSafePatchBoundary(code, 3, sizeof(code));

    CHECK(result == 0);
}

TEST_CASE("deobfuscatePointer xor is reversible") {
    uintptr_t real = 0x12345678;
    uintptr_t frame = 0xABCDEF00;

    uintptr_t obf = real ^ frame;
    uintptr_t out = p::deobfuscatePointer(obf, frame);

    CHECK(out == real);
}