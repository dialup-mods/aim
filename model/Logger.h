#pragma once
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <mutex>

#include "fmt/format.h"
#include "fmt/xchar.h"

#include "ConfigManager.h"
#include "MessageBox.h"

static std::mutex consoleMutex;

inline auto
enwiden(const std::string& input) -> std::wstring {
    if (input.empty()) return L"";

    int sizeRequired = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (sizeRequired == 0) return L"[enwiden error]";

    std::wstring result(sizeRequired, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &result[0], sizeRequired);

    // Remove null terminator Windows sticks in there
    result.pop_back();

    return result;
}

inline auto
enshrinken(const std::wstring& input) -> std::string {
    if (input.empty()) return "";

    int sizeRequired = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeRequired == 0) return "[enshrinken error]";

    std::string result(sizeRequired, '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, &result[0], sizeRequired, nullptr, nullptr);

    // Remove null terminator
    result.pop_back();

    return result;
}

class Logger {
    Logger() = default;
    ~Logger() { close(); }

public:
    static auto instance() -> Logger& {
        static Logger inst;
        return inst;
    }

    Logger(Logger&&) = delete;
    Logger(const Logger&) = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    auto operator=(const Logger&) -> Logger& = delete;

    void open() {
    }

    void close() {
        if (handle_) {
            fclose(handle_); // NOLINT(*-owning-memory)
            handle_ = nullptr;
        }
    }

    template <typename... Args>
    static void logImpl(const bool newline, fmt::format_string<Args...> fmtstr, Args&&... args) {
        std::lock_guard lock(consoleMutex);

        thread_local fmt::memory_buffer buf;
        buf.clear();

        fmt::format_to(std::back_inserter(buf), fmtstr, std::forward<Args>(args)...);
        if (newline) {
            buf.push_back('\n');
        }

        auto outStr = to_string(buf);

        fmt::print("{}", outStr);
        if (instance().handle_) {
            fmt::print(instance().handle_, "{}", outStr);
        }

        std::cout << std::flush;
        if (instance().handle_) {
            fflush(instance().handle_);
        }
    }

    template <typename... Args>
    void log(fmt::format_string<Args...> fmtstr, Args&&... args) {
        logImpl(true, fmtstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void logNoNewline(fmt::format_string<Args...> fmtstr, Args&&... args) {
        logImpl(false, fmtstr, std::forward<Args>(args)...);
    }

    //static void log(const std::wstring& ws, const bool newline = true) {
    //    logImpl(newline, "{}", enshrinken(ws));
    //}

    //static void log(const std::wstring& str, const bool bFlush = true, const bool newline = true) {
    //    std::wstring formattedMsg = fmt::format(L"{}{}", str, newline ? L"\n" : L"\b");
    //    std::wcout << formattedMsg.c_str();

    //    if (!handle_) { return; }
    //    fprintf(handle_, enshrinken(formattedMsg).c_str()); // NOLINT(*-pro-type-vararg)
    //    if (bFlush) { fflush(handle_); }
    //}

    template <typename... Args>
    void print(const bool newline, fmt::format_string<Args...> fmtstr, Args&&... args) {
        std::lock_guard lock(consoleMutex);

        thread_local fmt::memory_buffer buf;
        buf.clear();

        fmt::format_to(std::back_inserter(buf), fmtstr, std::forward<Args>(args)...);
        if (newline) {
            buf.push_back('\n');
        }

        fmt::print("{}", fmt::to_string(buf));
        std::cout << std::flush;
    }

    template <typename... Args>
    void print(fmt::format_string<Args...> fmtstr, Args&&... args) {
        print(true, fmtstr, std::forward<Args>(args)...);
    }

    void print(const std::string& str, const bool newline = true) {
        print(newline, "{}", str);
    }

    void print(const std::wstring& ws, const bool newline = true) {
        print(newline, "{}", enshrinken(ws));
    }

    void print(const std::wstring& str, const bool bFlush = true, const bool newline = true) {
        std::wstring formattedMsg = fmt::format(L"{}{}", str, newline ? L"\n" : L"\b");
        std::wcout << formattedMsg.c_str();

        if (!handle_) { return; }
        fprintf(handle_, enshrinken(formattedMsg).c_str()); // NOLINT(*-pro-type-vararg)
        if (bFlush) { fflush(handle_); }
    }

    void printRaw(std::string_view sv, bool newline = true, bool flush = true) {
        std::lock_guard lock(consoleMutex);
        std::cout << sv;
        if (newline) std::cout << '\n';
        if (flush)   std::cout << std::flush;
    }

private:
    static inline FILE* handle_{nullptr};
};