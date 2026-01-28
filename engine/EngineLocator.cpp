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

    auto start = reinterpret_cast<uintptr_t>(module);
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

auto EngineLocator::getFNameEntriesAddress() -> uintptr_t {
    constexpr unsigned char fNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
    char fNamesMask[] = "??????xx??xxxxxx";

    const uintptr_t fNameEntriesAddress = findPattern(GetModuleHandleW(L"RocketLeague.exe"), fNamesPattern, fNamesMask);

    const auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"RocketLeague.exe"));
    const auto fNameEntriesOffset = fNameEntriesAddress - moduleBase;
    log_->info("Rocket League base: {}", int_to_hex(moduleBase));
    log_->info("GNames address:     {}", int_to_hex(fNameEntriesAddress));
    log_->info("GNames offset:      {}", int_to_hex(fNameEntriesOffset));
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