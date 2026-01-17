#pragma once
#include "ILogger.h"

#define ENABLE_REGISTER_SPY

extern bool isPEDetoured;

struct SavedRegisters {
    // Calling convention registers (must preserve)
    uint64_t rcx, rdx, r8, r9;
    
    // Commonly used preserved registers
    uint64_t rbx, rsi, rdi, rbp;
    
    // Additional registers that might be needed
    uint64_t r12, r13, r14, r15;
    
    // SSE registers if used
    // __m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5;
};

// Define register info with correct encoding
struct RegInfo {
    const void* targetAddr;
    uint8_t rex;       // REX prefix (0x48 for general regs, 0x4C for r8-r15)
    uint8_t regCode;   // Register code (0-7)
    std::string name;
};

inline void emitRegisterTransfer(uint8_t* patch, int& offset, const SavedRegisters& target, bool restore) {

    RegInfo regs[] = {
        // Calling convention registers (essential for parameter passing)
        { &target.rcx, 0x48, 1, "rcx" },  // RCX = reg code 1
        { &target.rdx, 0x48, 2, "rdx" },  // RDX = reg code 2
        { &target.r8,  0x4C, 0, "r8" },   // R8  = reg code 0 (REX.R prefix)
        { &target.r9,  0x4C, 1, "r9" },   // R9  = reg code 1 (REX.R prefix)
        
        // Commonly used preserved registers (often used by VMs)
        { &target.rdi, 0x48, 7, "rdi" },  // RDI = reg code 7
        { &target.rsi, 0x48, 6, "rsi" },  // RSI = reg code 6
        { &target.rbx, 0x48, 3, "rbx" },  // RBX = reg code 3
        { &target.rbp, 0x48, 5, "rbp" },  // RBP = reg code 5
        
        // Additional volatile registers (sometimes used by optimizations/VMs)
        // see other note.. which is.. um.. elsewhere
        //{ &target.r10, 0x4C, 2, "r10" },  // R10 = reg code 2 (REX.R prefix)
        //{ &target.r11, 0x4C, 3, "r11" },  // R11 = reg code 3 (REX.R prefix)
        
        // Preserved registers (less likely needed but good for completeness)
        { &target.r12, 0x4D, 4, "r12" },  // R12 = reg code 4 (REX.R+B prefix)
        { &target.r13, 0x4D, 5, "r13" },  // R13 = reg code 5 (REX.R+B prefix)
        { &target.r14, 0x4D, 6, "r14" },  // R14 = reg code 6 (REX.R+B prefix)
        { &target.r15, 0x4D, 7, "r15" },  // R15 = reg code 7 (REX.R+B prefix)
        
        // If you need SSE registers later (unlikely for ProcessEvent but here for reference)
        // { &target.xmm0, 0x66, 0, "xmm0" }, // Different encoding scheme
        // { &target.xmm1, 0x66, 1, "xmm1" },
        // etc...
    };

    for (const auto& reg : regs) {
        // movabs rax, &target (10 bytes total)
        patch[offset++] = 0x48;  // REX.W prefix
        patch[offset++] = 0xB8;  // MOV RAX, imm64 opcode
        std::memcpy(patch + offset, &reg.targetAddr, sizeof(void*));
        offset += 8;
    
        if (restore) {
            // mov reg, [rax] (3 bytes)
            patch[offset++] = reg.rex;       // REX prefix (may include R/X/B bits)
            patch[offset++] = 0x8B;          // MOV r64, r/m64 opcode
            patch[offset++] = 0xC0 | (reg.regCode << 3); // ModRM: mod=11 (register), reg=regCode, rm=000 (RAX)
        } else {
            // mov [rax], reg (3 bytes)
            patch[offset++] = reg.rex;       // REX prefix
            patch[offset++] = 0x89;          // MOV r/m64, r64 opcode
            patch[offset++] = 0xC0 | (reg.regCode << 3); // ModRM: mod=11 (register), rm=000 (RAX), reg=regCode
        }
    
        // More detailed logging
       // logf_debug("Emitting: {}{} {}, [rax] (target:{:p}, offset:{})",
       //           restore ? "load " : "store",
       //           reg.name,
       //           restore ? reg.name : "",
       //           reg.targetAddr,
       //           offset);
    }
}


struct RegSaveInfo {
    const void* targetAddress;
    uint8_t movOpcode;
};

enum class RegisterCapturePoint {
    BeforeDetourLogic,
    AfterDetourLogic,
    BeforeFixup,
    AfterFixup,
    MAX_CAPTURE_POINTS // <-- Always put this last
};

//inline void safeSaveRegisters(uint8_t* patch, int& offset, SavedRegisters& target) {
//    // Save RCX only first
//    const RegInfo regs[] = {
//        {&target.rcx, 0x48, 1, "rcx"} // Just test RCX first
//    };
//
//    for (const auto& reg : regs) {
//        // movabs rax, &target.rcx
//        patch[offset++] = 0x48; patch[offset++] = 0xB8;
//        *reinterpret_cast<void**>(patch + offset) = reg.targetAddr;
//        offset += 8;
//
//        // mov [rax], rcx
//        patch[offset++] = reg.rex;
//        patch[offset++] = 0x89;
//        patch[offset++] = 0x08; // ModRM: [rax], rcx
//    }
//}

void dumpRegisterSpy();

extern SavedRegisters g_savedRegsSpy[static_cast<int>(RegisterCapturePoint::MAX_CAPTURE_POINTS)];
extern SavedRegisters g_savedRegsReplay[static_cast<int>(RegisterCapturePoint::MAX_CAPTURE_POINTS)];
extern void* targetFunc;




/*
 *
 * on r10 and r11
 *u just do a push rax; pop rax-style basic call to your handler and don't call any more nested functions from inside your hook,
  ✅ or if you only log to a debug log that doesn't smash r10/r11,
  → then you never need to worry about r10 and r11.
  🧠 But if you do want "fun" with r10/r11 later:
  
  Some spicy things you can do if you want:
  Fun Thing	Description	Requires caring about r10/r11?
  Inline modify function call targets	E.g., hijack (this->*Func->Func) to something else	✅ Sometimes need r10 fixup
  Inline stack manipulation	Modifying rsp between calls or handling naked call frames	✅ Yes (r11/rsp very sensitive)
  Aggressive trampoline chaining	Like building a double-trampoline where first trampoline sets up r10 to route to a second handler	✅ Yes
  Calling complex stdlib functions in your hook	Anything that can itself smash r10/r11 without restoring it	✅ Yes
  Saving/restoring absolute CPU state (hypervisor-style)	Insane mode.	✅ Absolutely
  */