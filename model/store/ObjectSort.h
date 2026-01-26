#pragma once
#include <vector>

#include "Logger.h"
#include "ObjectStore.h"
#include "UClass.h"
#include "UScriptStruct.h"

#include <queue>

struct StructSortResult {
    std::vector<UScriptStructEntry*> sortedStructs; // all structs, including the forward declares
    std::vector<UScriptStructEntry*> forwardDecl; // circular deps, must be forward declared
};

namespace object_sort {

inline auto topoSortStructs(std::vector<UScriptStructEntry*> structs_) -> StructSortResult {
    std::unordered_map<std::string, int> indegree;
    std::unordered_map<std::string, std::vector<std::string>> dependents;

    // Initialize
    for (const auto* node : structs_) {
        indegree[node->getFullName()] = 0;
    }

    // Build both indegree and dependents map, keeps edges o(1) instead of o(n)
    for (auto* node : structs_) {
        //Logger::instance().log("Processing node: {}", static_cast<void*>(node));

        UObject* uObj = node->getObject();
        if (!uObj || uObj->ObjectFlags & (RF_PendingKill | RF_Transient)) {
            //Logger::instance().log("Node {} has invalid UObject, skipping", node->getName());
            continue;
        }

        const char* const delim = ", ";

        auto strings = node->deps();
        std::ostringstream imploded;
        std::copy(strings.begin(), strings.end(), std::ostream_iterator<std::string>(imploded, delim));
        //Logger::instance().log("deps: {}", imploded.str());

        for (auto depName : node->deps()) {
            //Logger::instance().log("  Checking dep: {}", depName);

            if (ObjectStore::instance().getStructByName(depName)) {
                //Logger::instance().log("  Found dep, getting node name...");
                //std::string nodeName = node->getFullName(); // Isolate this call
                //Logger::instance().log("  Node name: {}", nodeName);

                //Logger::instance().log("Adding dependency: {} to {}", depName, nodeName);

                indegree[node->getFullName()]++;
                dependents[depName].push_back(node->getFullName());
            }
        }
    }

    // Start with nodes that have no dependencies
    std::queue<std::string> q;
    for (auto& [nodeStr, deg] : indegree) {
        if (deg == 0) q.push(nodeStr);
    }

    std::vector<UScriptStructEntry*> sorted;
    while (!q.empty()) {
        auto nodeStr = q.front();
        //Logger::instance().log("Processing node: {}, indegree was: {}", nodeStr, indegree[nodeStr]);

        q.pop();
        sorted.push_back(ObjectStore::instance().getStructByName(nodeStr));

        for (auto dependent : dependents[nodeStr]) {
            if (--indegree[dependent] == 0) {
                //Logger::instance().log("  Decrementing dependent: {}", dependent);
                q.push(dependent);
            }
        }
    }

    std::vector<UScriptStructEntry*> forwardDecl;
    if (sorted.size() != structs_.size()) {
        //Logger::instance().log("Structs that got stuck in topo sort:");
        for (auto& [nodeStr, deg] : indegree) {
            if (deg > 0) {
                //Logger::instance().log("  {} (indegree: {})", nodeStr, deg);
                //Logger::instance().log(" {}", nodeStr);
                forwardDecl.emplace_back(ObjectStore::instance().getStructByName(nodeStr));
                // need to also output the full def
                sorted.emplace_back(ObjectStore::instance().getStructByName(nodeStr));
            }
        }
    }
    //Logger::instance().log("sorted structs size: {}", sorted.size());

    return { .sortedStructs=sorted, .forwardDecl=forwardDecl };
}

inline auto topoSortClasses(std::vector<ClassEntry*> classes_)
    -> std::vector<ClassEntry*> {

    // Build lookup for O(1) parent resolution
    std::unordered_map<std::string, ClassEntry*> classMap;
    for (auto* c : classes_)
        classMap[c->getFullName()] = c;

    std::unordered_set<std::string> visited;
    std::vector<ClassEntry*> ordered;
    ordered.reserve(classes_.size());

    std::function<void(ClassEntry*)> visit = [&](ClassEntry* c) {
        if (!c || visited.contains(c->getFullName()))
            return;

        visited.insert(c->getFullName());

        // Resolve and visit parent first if it’s in the same package
        auto superName = c->getSuperFieldFullName();
        if (!superName.empty()) {
            if (auto it = classMap.find(superName); it != classMap.end()) {
                visit(it->second);
            } else {
                // parent not in this package — treat as external root
                //Logger::log("Parent {} not found for {}", superName, c->getFullName());
            }
        }

        ordered.push_back(c);
    };

    for (auto* c : classes_)
        visit(c);

    return ordered;
}

}