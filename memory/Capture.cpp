#include "Capture.h"
#include "LogHandler.h"

// 🔥 DEFINE storage exactly once here
SavedRegisters g_savedRegsSpy[static_cast<int>(RegisterCapturePoint::MAX_CAPTURE_POINTS)] = {};
SavedRegisters g_savedRegsReplay[static_cast<int>(RegisterCapturePoint::MAX_CAPTURE_POINTS)] = {};

// Implement your dumpRegisterSpy() function here
void dumpRegisterSpy() {
    for (int i = 0; i < static_cast<int>(RegisterCapturePoint::MAX_CAPTURE_POINTS); ++i) {
        const auto& save = g_savedRegsSpy[i];
//        log_->logf_debug("Capture {}: rbx={:#x}, rsi={:#x}, rdi={:#x}, rbp={:#x}",
//                      i, save.rbx, save.rsi, save.rdi, save.rbp);
    }
}

void* targetFunc = nullptr;
bool isPEDetoured = false;