# AIM Architecture

AIM provides type-safe memory access and function hooking for game modding.

## Public API

See `include/` directory for full API documentation.

## Implementation

Core implementation is closed-source for security reasons. 
The public headers demonstrate the architecture and design patterns used.

**Key features:**
- RAII-based resource management
- Type-safe function hooking
- Automatic cleanup on detach
- Cross-DLL safe operation
