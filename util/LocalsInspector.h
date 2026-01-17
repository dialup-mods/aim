#pragma once

inline void
inspectLocalsRaw(char* out, size_t outSize, uint8_t* data, size_t size) {
    size_t offset = 0;
    size_t cursor = 0;

    __try {
        while (offset + 4 <= size && cursor + 128 < outSize) {
            int32_t asInt = *reinterpret_cast<int32_t*>(data + offset);
            float asFloat = *reinterpret_cast<float*>(data + offset);
            uintptr_t asPtr = *reinterpret_cast<uint32_t*>(data + offset);

            cursor += snprintf(out + cursor, outSize - cursor, "[%02zu] int32: %11d  float: %11.3f  ptr: 0x%08X\n", offset, asInt,
                asFloat, (uint32_t)asPtr);

            offset += 4;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { snprintf(out + cursor, outSize - cursor, "[!] Exception reading stack.Locals\n"); }
}

inline std::string
inspectLocals(uint8_t* data, size_t size) {
    constexpr size_t bufSize = 2048;
    static char buffer[bufSize]; // thread-unsafe, can make it thread_local if needed
    memset(buffer, 0, bufSize);
    inspectLocalsRaw(buffer, bufSize, data, size);
    return std::string(buffer);
}