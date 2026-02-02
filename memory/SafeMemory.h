#pragma once

namespace safe::memory {

// Safe memory read for any pointer type
template<typename T>
bool
readPtrSafe(const void* address, T& value) {
    __try {
        value = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

template<typename T>
bool
writePtrSafe(T* ptr, const T& value) {
    __try {
        *ptr = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// Safe pointer dereference
template<typename T>
T*
derefPtrSafe(T* const* ptr) {
    T* result = nullptr;
    __try {
        result = *ptr;
        return result;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

// Safe void pointer dereference
inline void*
derefVoidPtrSafe(void** ptr) {
    void* result = nullptr;
    __try {
        result = *ptr;
        return result;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

inline bool
isAddressAccessible(const void* address, size_t size = 8) {
    if (!address)
        return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0) {
        return false;
    }

    // Check if memory is committed and accessible
    if (!(mbi.State & MEM_COMMIT) || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        return false;
    }

    return true;
}

template<typename T, typename Func>
bool
withSafePtr(T* ptr, Func operation) {
    if (!isAddressAccessible(ptr)) {
        return false;
    }

    __try {
        operation(ptr);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

inline bool writeBytes(void* dest, const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) { return true; }
    if (!isAddressAccessible(dest, bytes.size())) { return false; }

    DWORD oldProtect;
    if (!VirtualProtect(dest, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy(dest, bytes.data(), bytes.size());

    DWORD dummy;
    VirtualProtect(dest, bytes.size(), oldProtect, &dummy);

    return true;
}

inline bool writeBytes(void* dest, const uint8_t* src, const size_t size) {
    if (size == 0) { return true; }

    if (!isAddressAccessible(dest, size)) { return false; }

    DWORD oldProtect;
    if (!VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy(dest, src, size);

    DWORD dummy;
    VirtualProtect(dest, size, oldProtect, &dummy);

    return true;
}

// Overload for filling NOPs / padding
inline bool fillBytes(void* dest, uint8_t value, size_t size) {
    if (size == 0) {
        return true;
    }

    if (!isAddressAccessible(dest, size)) {
        return false;
    }

    DWORD oldProtect;
    if (!VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memset(dest, value, size);

    DWORD dummy;
    VirtualProtect(dest, size, oldProtect, &dummy);

    return true;
}

inline void read(uintptr_t address, void* buffer, size_t size) {
    if (size == 0 || buffer == nullptr) return;
    std::memcpy(buffer, reinterpret_cast<const void*>(address), size);
}

}