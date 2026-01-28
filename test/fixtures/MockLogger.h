#pragma once
#include "ILogger.h"
#include <iostream>
#include <string_view>
#include "fmt/format.h"

class MockLogger : public ILogger {
    AIM_INJECTABLE(MockLogger)

    ~MockLogger() = default;
    MockLogger() = default;

    void setLogLevel(LogLevel level) override { currentLevel_ = level; }
    void addSink(const std::shared_ptr<LogSink>&) override {}
    void setLogPath(const std::string&) override {}
    auto toString(LogCategory category) -> std::string override { return "category"; }
    auto toString(LogLevel level) -> std::string override { return "level"; }

    void log(std::string_view message, LogCategory category, LogLevel level) {
        if (level < currentLevel_) return;

        std::string formattedMessage = fmt::format("[{:>5}] [{}] {}", toString(level), toString(category), message);

        std::cout << "[" << toString(level) << "][" << toString(category) << "] "
                  << formattedMessage << std::endl;
        std::cout.flush();
    }

    void printRaw(std::string_view sv, bool newline, bool flush) override {};

private:
    LogLevel currentLevel_ = LogLevel::LOG_DEBUG;
    void logImpl(std::string_view message, LogLevel level) override {};
};