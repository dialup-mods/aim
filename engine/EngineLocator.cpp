#include <Windows.h>
#include <Psapi.h>
#include <cstddef>
#include <iomanip>
#include <sstream>

#include "EngineLocator.h"
#include "ILogger.h"

auto EngineLocator::int_to_hex(uintptr_t value, size_t width) -> std::string {
    std::ostringstream oss;
    oss << "0x" << std::setfill('0') << std::setw(static_cast<int>(width)) << std::hex << std::uppercase << value;
    return oss.str();
}

auto EngineLocator::findPattern(HMODULE module, const unsigned char* pattern, const char* mask) -> uintptr_t {
    MODULEINFO info = {};
    GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

    const auto start = reinterpret_cast<uintptr_t>(module);
    const size_t length = info.SizeOfImage;

    size_t pos = 0;
    const size_t maskLength = std::strlen(mask) - 1; // wtf

    for (uintptr_t retAddress = start; retAddress < start + length; retAddress++) {
        if (*reinterpret_cast<unsigned char*>(retAddress) == pattern[pos] || mask[pos] == '?') {
            if (pos == maskLength) {
                return (retAddress - maskLength);
            }
            pos++;
        } else {
            retAddress -= pos;
            pos = 0;
        }
    }
    return NULL;
}

inline uintptr_t findRipRelativeAddr(uintptr_t startAddr, int offsetToDisplacementInt32) {
    if (!startAddr)
        return 0;

    uintptr_t ripRelativeOffsetAddr = startAddr + offsetToDisplacementInt32;
    int32_t displacement = *reinterpret_cast<int32_t*>(ripRelativeOffsetAddr);

    return ripRelativeOffsetAddr + 4 + displacement;
}

auto EngineLocator::getFNameEntriesAddress() -> uintptr_t {
    constexpr unsigned char fNamesPattern[] =
        "\x49\x63\x4E\x08\x48\x8B\x05\x00\x00\x00\x00\x4C\x89\x34\xC8\xEB\x08";

    char fNamesMask[] = "xxxxxxx????xxxxxx";

    const auto module = GetModuleHandleW(L"RocketLeague.exe");
    const auto moduleBase = reinterpret_cast<uintptr_t>(module);

    const uintptr_t fNameEntriesInstruction =
        findPattern(module, fNamesPattern, fNamesMask);

    const uintptr_t fNameEntriesAddress =
        findRipRelativeAddr(fNameEntriesInstruction, 7);

    log_->info("Rocket League base:        {}", int_to_hex(moduleBase));
    log_->info("GNames instruction:         {}", int_to_hex(fNameEntriesInstruction));
    log_->info("GNames address:             {}", int_to_hex(fNameEntriesAddress));
    log_->info("GNames offset:              {}", int_to_hex(fNameEntriesAddress - moduleBase));

    return fNameEntriesAddress;
}

auto EngineLocator::getUObjectsAddress() -> uintptr_t {
    const auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"RocketLeague.exe"));
    const auto uObjectsAddress = getFNameEntriesAddress() + 0x48;
    const auto uObjectsOffset = uObjectsAddress - moduleBase;
    log_->info("Rocket League base: {}", int_to_hex(moduleBase));
    log_->info("GObjects address:   {}", int_to_hex(uObjectsAddress));
    log_->info("GObjects offset:    {}", int_to_hex(uObjectsOffset));
    return uObjectsAddress;
}