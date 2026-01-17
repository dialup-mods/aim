#pragma once
#include "SafeMemory.h"

#define SAFE_READ(ptr, fallback) (safe::memory::readPtrSafeOr(ptr, fallback))
#define SAFE_WRITE(ptr, value) (safe::memory::readPtrSafeOr(ptr, value))
#define SAFE_DEREF(ptr) (safe::memory::derefPtrSafe(ptr))
#define SAFE_DEREF_VOID(ptr) (safe::memory::derefPtrSafe(ptr))
#define IS_ADDRESS_ACCESSIBLE(ptr) (safe::memory::isAddressAccessible(ptr)

// run code if pointer valid
#define WITH_SAFE_PTR(ptr, code) (safe::memory::withSafePtr(ptr, [&](auto* _ptr_) { code; }))

// run fallback logic if pointer invalid
#define SAFE_READ_OR_ELSE(ptr, fallbackCode)                                   \
    ([&]() -> auto {                                                           \
        auto result = fallbackCode;                                            \
        safe::memory::withSafePtr(ptr, [&](auto* _ptr_) { result = *_ptr_; }); \
        return result;                                                         \
    }())

// alias for WITH_SAFE_PTR
#define SAFE_BLOCK(ptr, code) WITH_SAFE_PTR(ptr, code)

// Try safely evaluating a chained pointer expression
// e.g. auto weapon = SAFE_CHAIN(pPlayer, pPlayer->inventory->currentWeapon->data);
#define SAFE_CHAIN(ptr, expr)                                      \
    ([&]() -> std::remove_pointer_t<decltype(expr)>* {             \
        if (!safe::memory::isAddressAccessible(ptr))               \
            return nullptr;                                        \
        __try {                                                    \
            return expr;                                           \
        } __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; } \
    }())

#define SAFE_EVAL(expr)                                                                         \
    ([&]() -> std::optional<std::remove_reference_t<decltype((expr))>> {                        \
        using T = std::remove_reference_t<decltype((expr))>;                                    \
        static_assert(std::is_destructible_v<T>, "SAFE_EVAL: Expression must be destructible"); \
        if (!safe::memory::isAddressAccessible(reinterpret_cast<const void*>(&(expr))))         \
            return std::nullopt;                                                                \
        __try {                                                                                 \
            return (expr);                                                                      \
        } __except (EXCEPTION_EXECUTE_HANDLER) { return std::nullopt; }                         \
    }())

// Instead of this (which might crash):
//
//     int value = *somePtr;
//
// Do this:
//
//     int value;
//     if (SAFE_READ(somePtr, value)) {
//         // value is updated
//         // use value safely
//     } else {
//         // value doesn't change -- is fallback
//         // Handle the error
//     }
//
// Or even simpler:
//
//     int value = SAFE_READ(somePtr, 0);  // Returns 0 if pointer is invalid

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

//std::string
//safeFString(UObject* maybeFStringObj);
//std::string
//readStringField(UObject* obj, size_t offset, size_t len);


// Write bytes safely
inline bool writeBytes(void* dest, const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return true; // nothing to write
    }

    if (!isAddressAccessible(dest, bytes.size())) {
        return false;
    }

    DWORD oldProtect;
    if (!VirtualProtect(dest, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy(dest, bytes.data(), bytes.size());

    DWORD dummy;
    VirtualProtect(dest, bytes.size(), oldProtect, &dummy);

    return true;
}

// Overload for raw data pointer
inline bool writeBytes(void* dest, const uint8_t* src, size_t size) {
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