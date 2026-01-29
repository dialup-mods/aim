#include <Windows.h>
#include <Psapi.h>

#include "TestConsole.h"
#include "test_Runtime.h"
#include "test_ObjectProvider.h"
#include "test_Detour_ProcessEvent.h"
#include "test_Detour_ProcessInternal.h"

uintptr_t gDllStart = 0;
uintptr_t gDllEnd   = 0;

void initDllBounds(HMODULE hModule) {
    MODULEINFO info{};
    GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info));
    gDllStart = reinterpret_cast<uintptr_t>(info.lpBaseOfDll);
    gDllEnd   = gDllStart + info.SizeOfImage;
}
static PVOID gVeh = nullptr;

static LONG WINAPI crashShield(PEXCEPTION_POINTERS p) {
    const auto addr = reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress);

    if (addr < gDllStart || addr > gDllEnd)
        return EXCEPTION_CONTINUE_SEARCH;

    if (p->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    OutputDebugStringA("[TEST DLL] caught access violation, bailing\n");

    // Abort test execution cleanly
    ExitThread(0);
}

void installCrashShield() {
    gVeh = AddVectoredExceptionHandler(1, crashShield);
}

void removeCrashShield() {
    if (gVeh) {
        RemoveVectoredExceptionHandler(gVeh);
        gVeh = nullptr;
    }
}

DWORD WINAPI
Worker(const LPVOID lpParam) {
    Sleep(100);
    installCrashShield();

    {
        test::terminal::tryHookConsoleIO();
        printf("Test Worker started.\n");


        // NOTE: this test must be run first to populate runtime
        RuntimeTest rtTest;
        rtTest.run();

        ObjectProviderTest objTest;
        objTest.run();

        ProcessEventTest peTest;
        peTest.run();

        ProcessInternalTest piTest;
        piTest.run();

    }
    removeCrashShield();

    Sleep(300);
    FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
}

BOOL APIENTRY
DllMain(const HMODULE hModule, const DWORD reason, LPVOID) {
    initDllBounds(hModule);
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, Worker, hModule, 0, nullptr);
    }
    return TRUE;
}
