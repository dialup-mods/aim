#pragma once
#include "PatchDefinition.h"

#include <optional>
#include <string>
#include <vector>

class Patch;

class PatchBuilder {
    PatchDefinition def_;
    
public:
    PatchBuilder();
    PatchBuilder& name(std::string n);

    PatchBuilder& setPosition(uintptr_t pos);
    
    PatchBuilder& shortJump(uintptr_t to);

    PatchBuilder& absoluteJump(uintptr_t to);
    PatchBuilder& bytes(std::vector<uint8_t> bytes);
    PatchBuilder& hookWithTrampoline(
        uintptr_t originalFunction,
        void* handlerFunction);
    PatchBuilder& copyBytes(uintptr_t from,
        size_t size);
    PatchBuilder& padding();
    PatchBuilder& offset();

    PatchBuilder& saveRegisters();
    PatchBuilder& restoreRegisters();
    PatchBuilder& overwriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes, const std::vector<uint8_t>& expected);
    PatchBuilder& skip(uintptr_t from, size_t bytesToSkip);
    
    std::optional<size_t> findSafePatchBoundary(const uint8_t* patchSite, size_t minBytesNeeded);
    
    PatchBuilder& walkTrampolineChain();

    PatchDefinition finalize();

private:
    std::vector<PatchStep> steps_;
    bool finalized_ = false;
};