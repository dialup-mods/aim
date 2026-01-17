#include "PatternScanner.h"

enum class RLFunctionType {
    CallFunction,
    ProcessEvent,
    ProcessInternal,
};

template<typename FnType>
struct RLFunctionPattern {
    const BYTE* originalPattern;
    size_t originalPatternSize;
    const BYTE* hookedPattern;
    size_t hookedPatternSize;
};

template<typename FnType>
class RLFunction {
  public:
    RLFunctionType type;
    RLFunctionPattern<FnType> pattern;
    FnType rlFn = nullptr;
    FnType ogFn = nullptr;

    RLFunction(RLFunctionType t, const RLFunctionPattern<FnType>& p)
      : type(t)
      , pattern(p) {
        resolve();
    }

    void resolve() {
        void* found = scanForPattern(pattern.originalPattern, pattern.originalPatternSize);

        if (!found) {
            found = scanForPattern(pattern.hookedPattern, pattern.hookedPatternSize);
        }

        if (found) {
            rlFn = reinterpret_cast<FnType>(found);
            ogFn = rlFn;
            logger->debug("Resolved " + functionTypeName(type) + " at 0x" + getPointerString());
        } else {
            logger->error("Failed to resolve " + functionTypeName(type));
        }
    }

    std::string getPointerString() const {
        std::ostringstream oss;
        oss << std::hex << reinterpret_cast<uintptr_t>(rlFn);
        return oss.str();
    }

    bool isValid() const { return rlFn != nullptr; }

  private:
    static void* scanForPattern(const BYTE* pattern, size_t patternSize) {
        // Your existing pattern scan logic (adapted)
        return patternscanner::scan(pattern, patternSize);
    }

    static std::string functionTypeName(RLFunctionType t) {
        switch (t) {
            case RLFunctionType::CallFunction: return "CallFunction";
            case RLFunctionType::ProcessEvent: return "ProcessEvent";
            case RLFunctionType::ProcessInternal: return "ProcessInternal";
            default: return "Unknown";
        }
    }
};
