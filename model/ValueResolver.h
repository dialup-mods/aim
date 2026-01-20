#pragma once
#include <string>
#include <vector>
#include <cstdint>
class StructLikeEntry;
class UObject;
class UEnum;

struct MyResolvedValue {
    struct ResolvedInterface {
        UObject* object{nullptr};
        std::string objectName;
        std::string className;

        // Raw pointer preserved only for identity/debugging
        uintptr_t interfacePtr = 0;
    };

    struct ResolvedDelegate {
        UObject* object{nullptr};
        std::string functionName;
        std::string objectName;
        std::string className;
        std::vector<uint8_t> unknownData;
    };

    struct ResolvedInlineStruct {
        int32_t entryId;
        int32_t instanceId;
    };

    enum class StorageType {
        Int8,
        Int16,
        Int32,
        Int64,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Float,
        Double,
        Pointer,
        InlineStruct,
        Unknown
    };

    enum class Kind {
        Array,
        Bool,
        Double,
        Float,
        Int32,
        Int64,
        String,

        Class,
        Enum,
        Map,
        Name,
        ObjectRef,
        Struct,

        Delegate,
        Interface,
        MulticastDelegate,

        Null,
        Unknown
    };

    // Object
    //     The owning UObject that implements some interface
    // Interface
    //     A pointer to the interface vtable / subobject
    //     Only meaningful in-process
    //     Not stable, not portable, not replay-safe
    // “This UObject implements interface X, and here’s the fast-call pointer”

    Kind mKind = Kind::Unknown;
    StorageType mStorage = StorageType::Unknown;

    void* data = nullptr;
    const StructLikeEntry* schema{nullptr};

    // Optional enrichments
    UObject* object;
    ResolvedDelegate* mDelegate;
    ResolvedInterface* mInterface;
    ResolvedInlineStruct* mName;
    UEnum* uEnum;
    bool mInvalid;
    std::string objectName;
    std::string objectClass;
    std::string primitiveStr;
    std::string className;
    std::string fullName;
    std::string superName;

    bool isNull() const { return mKind == Kind::Null; }
};