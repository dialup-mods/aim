#pragma once
#include "ILogger.h"
#include <iostream>
#include <string_view>
#include "fmt/format.h"

class MockLogger : public ILogger {
    AIM_INJECTABLE(MockLogger)

public:
    MockLogger() = default;

    void setLogLevel(LogLevel level) override {
        currentLevel_ = level;
    }

    void addSink(const std::shared_ptr<LogSink>&) override {
        // No-op for test logger
    }

    void setLogPath(const std::string&) override {
        // No-op for test logger
    }

    auto logLevelToString(LogLevel level) -> std::string override {
        return toString(level);
    }

    void log(std::string_view message, LogCategory category, LogLevel level) override {
        if (level < currentLevel_) return;

        std::string formattedMessage = fmt::format("[{:>5}] [{}] {}", toString(level), toString(category), message);

        std::cout << "[" << toString(level) << "][" << toString(category) << "] "
                  << formattedMessage << std::endl;
        std::cout.flush();
    }

private:
    LogLevel currentLevel_ = LogLevel::LOG_DEBUG;
};