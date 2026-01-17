#pragma once

#include <cstdint>

class UObject;
class UStruct;

//struct FFrame {
//    FFrame* PreviousFrame; // 0x00
//    void* OutParms;        // 0x08 ← pointer
//    void* ReturnValue;     // 0x10 ← pointer
//    uint32_t ProbeMask;    // 0x18 ← 4-byte flags
//    uint32_t CallDepth;    // 0x1C ← 4-byte depth value
//    UStruct* Node;         // 0x28 ← points to UFunction/UStruct
//    UObject* Object;       // 0x30
//    uint8_t* Code;         // 0x38 ← bytecode pointer (nullptr in native)
//    uint8_t* Locals;       // 0x40 ← stack locals for function
//    int LineNumber;        // 0x48 ← optional, not always meaningful
//};