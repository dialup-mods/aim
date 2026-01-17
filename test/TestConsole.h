#pragma once
#include <Windows.h>
#include <thread>
#include <mutex>
#include <string>
#include <iostream>

namespace test::terminal {
inline auto hasConsole() -> bool { return GetConsoleWindow() != nullptr; }

inline int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 0;
}

inline void enableVT() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

inline void disableConsoleQuickEdit() {
    const HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hIn, &mode);

    // Remove QuickEdit and Insert modes
    mode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT);

    // Keep input enabled
    mode |= ENABLE_EXTENDED_FLAGS;

    SetConsoleMode(hIn, mode);
}

//inline void tryHookConsoleIO() {
//    if (!hasConsole()) {
//        AllocConsole();
//    }
//    //const HWND hwndConsole = GetConsoleWindow();
//    //if (hwndConsole) {
//    //    ShowWindow(hwndConsole, SW_SHOW);   // make sure it's visible
//    //    SetForegroundWindow(hwndConsole);   // bring to front
//    //}
//    FILE* dummy = nullptr;
//    freopen_s(&dummy, "CONOUT$", "w", stdout);
//    freopen_s(&dummy, "CONOUT$", "w", stderr);
//    freopen_s(&dummy, "NUL", "r", stdin);
//    std::ios::sync_with_stdio(true);
//    SetConsoleOutputCP(CP_UTF8);
////    SetConsoleOutputCP(1200);
//    disableConsoleQuickEdit();
//    enableVT();
//}

static void
tryHookConsoleIO() {
    if (hasConsole()) {
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        freopen_s(&dummy, "CONIN$", "r", stdin);
        std::ios::sync_with_stdio();
    } else {
        AllocConsole();
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        freopen_s(&dummy, "CONIN$", "r", stdin);
        SetConsoleOutputCP(CP_UTF8);
        std::cout.sync_with_stdio(true);
        std::ios::sync_with_stdio(); // belt + suspenders
    }
}

static void
tryFreeConsole() {
    if (hasConsole()) {
        FreeConsole();
    }
}

}
