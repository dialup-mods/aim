#pragma once
#include <string>
#include <vector>

enum class Category {
    ProcessEvent,
    ProcessInternal,
    CallFunction,
    UI,
    Network,
    Miscellaneous
};

enum class PatchStatus {
    NotApplied,
    Applied,
    Failed,
    Reverted
};

enum class PatchStepType {
    SetPosition,
    OverwriteBytes,
    ShortJump,
    AbsoluteJump,
    Padding,
    Offset,
    Bytes,
    GenerateTrampolineJump,
};

struct PatchStep {
    PatchStepType type;
    uintptr_t address; // if 0, use cursor
    uintptr_t jumpDestination = 0; // if it's a jump, store where we're jumping to
    size_t size = 0; // for padding or offset steps
    std::vector<uint8_t> bytes; // for raw bytes
    std::vector<uint8_t> expectedBytes;    // What should already be there
    std::vector<uint8_t> originalBytes;    // what we nuked
};

struct PatchDefinition {
    std::string name;
    std::vector<PatchStep> steps;
    
    bool validate() const {
        if (steps.empty())
            return false;

        if (steps[0].type != PatchStepType::SetPosition)
            return false;

        for (const auto& step : steps) {
            switch (step.type) {
                case PatchStepType::Bytes:
                    if (step.bytes.empty()) {
                        return false;
                    }
                break;

                case PatchStepType::AbsoluteJump:
                case PatchStepType::ShortJump:
                    // allow empty: bytes will be computed at apply time
                    if (step.jumpDestination == 0) {
                        return false; // but ensure it's not a null jump
                    }
                break;

                case PatchStepType::Padding:
                    if (step.size == 0) {
                        return false;
                    }
                break;

                case PatchStepType::OverwriteBytes:
                    if (step.bytes.empty() || step.expectedBytes.empty()) {
                        return false;
                    }
                break;

                default:
                    break;
            }
        }

        return true;
    }
   
    
};