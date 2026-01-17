#pragma once
#include <Windows.h>
#include <psapi.h>
#include <filesystem>

namespace test::library::loader {
inline auto
loadDll(const std::wstring& path) -> void* {
    if (path.empty()) {
        MessageBoxW(nullptr, L"Path is empty", L"loadLib", MB_OK);
        return nullptr;
    }

    if (!std::filesystem::exists(path)) {
        MessageBoxW(nullptr, L"File not found (fs::exists)", L"loadLib", MB_OK);
        return nullptr;
    }

    if (HMODULE handle = LoadLibraryW(path.c_str())) {
        return handle;
    }

    // NOTE: this message can say "could not load" even though the library DID load and it shit out somewhere else
    DWORD err = GetLastError();
    wchar_t msg[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, 0, msg, 512, nullptr);
    MessageBoxW(nullptr, msg, L"LoadLibraryW error", MB_OK | MB_SYSTEMMODAL);
    return nullptr;
}

inline void
unloadDll(const void* raw) {
    auto handle = reinterpret_cast<HMODULE>(const_cast<void*>(raw));
    __try {
        // Check if DLL is still referenced
        HMODULE testHandle = nullptr;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(handle), &testHandle)) { // NOLINT
            printf("Warning: DLL %p still has active references\n", handle);
        }

        // Get module info for flushing
        MODULEINFO modInfo{};
        if (GetModuleInformation(GetCurrentProcess(), handle, &modInfo, sizeof(modInfo))) {
            FlushInstructionCache(GetCurrentProcess(), modInfo.lpBaseOfDll, modInfo.SizeOfImage);
        }

        const BOOL result = FreeLibrary(handle);
        //return result != 0;
        return;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("Exception during DLL unload: %p, GetLastError: %d\n", handle, GetLastError());
        //return true; // just keep chugging anyway
    }
}

inline void tryUnloadDllByName(const std::wstring& dllNameOrPath) {
    HMODULE hMod = nullptr;

    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            dllNameOrPath.c_str(),
            &hMod))
    {
        printf("DialUp-SDK.dll not loaded (this is good)\n");
        return;
    }

    // Drain refcount
    while (FreeLibrary(hMod)) {
        // keep releasing until refcount hits zero
    }

    //return GetLastError() == ERROR_SUCCESS;
}
}
