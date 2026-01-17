#include <Windows.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "PatchBuilder.h"

#include "PatchDefinition.h"
#include "PatchUtils.h"

PatchBuilder::PatchBuilder()
    : def_()
{}

PatchBuilder& PatchBuilder::name(std::string n) {
    def_.name = std::move(n);
    return *this;
}

PatchBuilder& PatchBuilder::setPosition(uintptr_t pos) {
    PatchStep step;
    step.type = PatchStepType::SetPosition;
    step.address = pos;
    def_.steps.emplace_back(std::move(step));
    return *this;
}

PatchBuilder& PatchBuilder::shortJump(uintptr_t to) {
    PatchStep step;
    step.type = PatchStepType::ShortJump;
    step.jumpDestination = to;
    def_.steps.emplace_back(std::move(step));
    return *this;
}

PatchBuilder& PatchBuilder::absoluteJump(uintptr_t to) {
    std::vector<uint8_t> bytes = {
        0x48, 0xB8,             // mov rax, <dst>
        0, 0, 0, 0, 0, 0, 0, 0, // (fill in address)
        0xFF, 0xE0              // jmp rax
    };
    
    *reinterpret_cast<void**>(&bytes[2]) = reinterpret_cast<void*>(to);
    
    PatchStep step;
    step.type = PatchStepType::Bytes;
    step.bytes.assign(std::begin(bytes), std::end(bytes));
    
    def_.steps.emplace_back(std::move(step));
    
    return *this;
}

PatchBuilder& PatchBuilder::bytes(std::vector<uint8_t> bytes) {
    
    PatchStep step;
    step.type = PatchStepType::Bytes;
    step.bytes.assign(std::begin(bytes), std::end(bytes));
    
    return *this;
}

PatchBuilder& PatchBuilder::hookWithTrampoline(uintptr_t originalFunction, void* handlerFunction) {
    PatchStep step;
    step.type = PatchStepType::GenerateTrampolineJump;
    step.address = originalFunction;
    step.jumpDestination = reinterpret_cast<uintptr_t>(handlerFunction); // optional
    def_.steps.emplace_back(std::move(step));
    return *this;
}

PatchBuilder& PatchBuilder::padding() {
    // Stub for now
    return *this;
}

PatchBuilder& PatchBuilder::offset() {
    // Stub for now
    return *this;
}

PatchBuilder& PatchBuilder::saveRegisters() {
    // Stub for now
    return *this;
}

PatchBuilder& PatchBuilder::restoreRegisters() {
    // Stub for now
    return *this;
}

PatchBuilder& PatchBuilder::overwriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes, const std::vector<uint8_t>& expected) {
    // example:
    //     .overwriteBytes(0x403200, { 0x90, 0x90, 0x90 }, { 0x75, 0x05, 0xE9 }) // expects JNZ short jump
    
    return *this;
}

PatchDefinition PatchBuilder::finalize() {
    bool firstStep = true;
    
    if (finalized_) {
        MessageBoxA(nullptr, "PatchBuilder::finalize() called more than once", "PatchBuilder", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return {};
    }
    finalized_ = true;

    for (const auto& step : steps_) {
        if (firstStep) {
            if (step.address == 0 || step.type != PatchStepType::SetPosition) {
                throw std::invalid_argument("First patch step must define a starting address.");
            }
            firstStep = false;
        }
        if (step.type == PatchStepType::OverwriteBytes && step.expectedBytes.empty()) {
            throw std::invalid_argument("OverwriteBytes must have expectedBytes");
        }
    }
    

    return std::exchange(def_, {});
    

}