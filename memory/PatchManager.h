#pragma once

#include "IModule.h"

#include "PatchDefinition.h"

class PluginContext;
class ILogger;
class AsyncGate;

class PatchManager : public IModule {
    AIM_INJECTABLE(PatchManager)
    
public:
    explicit PatchManager() = default;
    ~PatchManager();
    void init();

    void add(const PatchDefinition& patch);
    bool applyPatchStep(PatchStep& step, uintptr_t& cursor, bool dryRun = false);
    void readMemory(uintptr_t address, void* buffer, size_t size);
    void writeMemory(uintptr_t address, const void* data, size_t size);
    void captureBytes(void* cursor, PatchStep& step);
    bool apply(PatchDefinition& patch, bool dryRun = false);
    void restore(const PatchDefinition& patch);

    void saveRegisters();
    void restoreRegisters();
    
    void applyAll();
    void restoreAll();
    void dryRun(PatchDefinition& patch);
    void dryRunAll();

    void debugPrintPatches() const;
    void debugPrintPatchesVerbose() const;

private:
    std::vector<PatchDefinition> patches_;
};