#pragma once
#include <windows.h>
#include <sstream>
#include <string>

// fixme promote to class, fix log
namespace stackdebug {
inline void copyToClipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();

    HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, text.size() + 1);
    if (!hGlob) {
        CloseClipboard();
        return;
    }

    memcpy(hGlob, text.c_str(), text.size() + 1);
    SetClipboardData(CF_TEXT, hGlob);
    CloseClipboard();
}

inline void showCopyableMessageBox(const char* title, const std::string& content) {
    // Copy to clipboard for easy paste
    copyToClipboard(content);

    // Show MessageBox
    MessageBoxA(nullptr, content.c_str(), title, MB_OK | MB_SETFOREGROUND);
}

inline void logRegisters(const char* stage) {
    CONTEXT ctx = {};
    RtlCaptureContext(&ctx);

    std::ostringstream oss;
    oss << "[BadgeGangStackDebug] Stage: " << stage << "\n";
    oss << "RSP = 0x" << std::hex << ctx.Rsp << "\n";
    oss << "RBP = 0x" << std::hex << ctx.Rbp << "\n";
    oss << "RCX = 0x" << std::hex << ctx.Rcx << "\n";
    oss << "RDX = 0x" << std::hex << ctx.Rdx << "\n";

    std::string message = oss.str();

//    log_->debug(message);
}
}