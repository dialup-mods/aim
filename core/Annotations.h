#ifdef DIALUP_BUILD
#define DIALUP_API __declspec(dllexport)
#else
#define DIALUP_API __declspec(dllimport)
#endif

// For any function that might be called from game code or other unsafe contexts
#define DIALUP_SAFE_CALL(func_body) \
__try { \
    try { \
        func_body \
    } catch (const std::exception& e) { \
        if (auto log = resolve<ILogger>()) { \
        log->error("[C++] %s", e.what()); \
    } \
        return false; \
    } catch (...) { \
        if (auto log = resolve<ILogger>()) { \
        log->error("[C++] Unknown exception"); \
    } \
        return false; \
    } \
    } __except (EXCEPTION_EXECUTE_HANDLER) { \
        if (auto log = resolve<ILogger>()) { \
        log->error("[SEH] 0x%08X", GetExceptionCode()); \
    } \
    return false; \
}