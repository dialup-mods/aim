#pragma once

/// Layout traits mixin using CRTP
/// Provides compile-time layout information for Unreal class hierarchies
/// @tparam ThisType The derived class type
/// @tparam BaseType The immediate base class type

template <typename ThisType, typename BaseType>
struct LayoutTraits {
    static constexpr auto getClassSize() -> ptrdiff_t { return sizeof(ThisType); }
    static constexpr auto offsetFrom() -> ptrdiff_t {
        return sizeof(ThisType) - sizeof(BaseType);
    }
};