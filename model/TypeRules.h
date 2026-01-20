#pragma once
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <regex>
#include <optional>
#include <string_view>

#include "fmt/format.h"
#include "fkYAML/node.hpp"

#include "ConfigManager.h"
#include "Logger.h"

struct StringHash {
    using is_transparent = void;
    using hash_type = std::hash<std::string_view>;
    auto operator()(const std::string_view txt) const noexcept -> size_t { return hash_type{}(txt); }
};

struct StringEqual {
    using is_transparent = void;
    auto operator()(const std::string_view lhs, const std::string_view rhs) const noexcept -> bool { return lhs == rhs; }
};


class TypeRules {
public:
    enum class Method { Exact, Contains, Regex };
    enum class Action { Blacklist, Rename };

    struct Rule {
        Method method;
        Action action;
        std::string pattern;
        //std::string replacement; // only used for rename
        std::regex compiledRegex; // only valid if regex
    };

    static TypeRules& instance() {
        static TypeRules inst;
        return inst;
    }

    void initialize() {
    }

    [[nodiscard]] auto isBlacklisted(std::string_view typeName) const -> bool {
        if (blacklistExact_.contains(typeName)) {
            Logger::instance().log("[Blacklist] {} (exact)", typeName);
            return true;
        }
        for (const auto& sub : blacklistContains_) {
            if (typeName.find(sub) != std::string_view::npos) {
                Logger::instance().log("[Blacklist] {} (contains)", typeName);
                return true;
            }
        }
        for (const auto& re : blacklistRegex_) {
            if (std::regex_match(typeName.begin(), typeName.end(), re)) {
                Logger::instance().log("[Blacklist] {} (regex)", typeName);
                return true;
            }
        }
        return false;
    }

    //[[nodiscard]] auto hasOverride(const std::string_view typeName) const -> bool {
    //    return renameExact_.contains(typeName) || findContainsRename(typeName).has_value()
    //        || findRegexRename(typeName).has_value();
    //}

    //[[nodiscard]] auto getOverrideName(const std::string_view typeName) const -> const std::string& {
    //    if (const auto it = renameExact_.find(typeName); it != renameExact_.end()) return it->second;
    //    if (const auto repl = findContainsRename(typeName)) return **repl;
    //    if (const auto repl = findRegexRename(typeName)) return **repl;
    //    static const std::string empty;
    //    return empty;
    //}

    [[nodiscard]] auto applyRules(const std::string_view originalName) const -> std::optional<std::string> {
        if (isBlacklisted(originalName)) return std::nullopt;

        if (const auto it = renameExact_.find(originalName); it != renameExact_.end())
            return it->second;
        //if (const auto repl = findContainsRename(originalName)) return **repl;
        //if (const auto repl = findRegexRename(originalName)) return **repl;

        return std::string(originalName); // no change
    }

private:
    std::unordered_set<std::string, StringHash, StringEqual> blacklistExact_;
    std::unordered_map<std::string, std::string, StringHash, StringEqual> renameExact_;

    std::vector<std::string> blacklistContains_;
    std::vector<std::pair<std::string, std::string>> renameContains_;

    std::vector<std::regex> blacklistRegex_;
    std::vector<std::pair<std::regex, std::string>> renameRegex_;

    TypeRules() = default;

    static auto parseMethod(const std::string& m) -> Method {
        if (m == "contains") return Method::Contains;
        if (m == "regex")    return Method::Regex;
        return Method::Exact;
    }
    static auto parseAction(const std::string& a) -> Action {
        return (a == "rename") ? Action::Rename : Action::Blacklist;
    }

    //[[nodiscard]] auto findContainsRename(const std::string_view& typeName) const -> std::optional<const std::string*> {
    //    for (const auto& [pattern, replacement] : renameContains_)
    //        if (typeName.find(pattern) != std::string_view::npos) return &replacement;
    //    return std::nullopt;
    //}
    //[[nodiscard]] auto findRegexRename(const std::string_view& typeName) const -> std::optional<const std::string*> {
    //    for (const auto& [pattern, replacement] : renameRegex_)
    //        if (std::regex_match(typeName.begin(), typeName.end(), pattern)) return &replacement;
    //    return std::nullopt;
    //}
};
