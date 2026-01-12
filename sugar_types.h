#ifndef LIBSUGARX_TYPES_H
#define LIBSUGARX_TYPES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace libsugarx
{
    template<std::size_t N>
    using data_buffer = std::array<std::byte, N>;

    template<std::size_t N>
    using int32_buffer = std::array<std::int32_t, N>;

    using data_span = std::span<std::byte>;
    using const_data_span = std::span<const std::byte>;

    using int32_span = std::span<std::int32_t>;
    using const_int32_span = std::span<const std::int32_t>;

    template<typename T>
    concept is_enum_only = std::is_enum_v<T>;

    template<is_enum_only E>
    constexpr E operator|(E a, E b)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(a) | static_cast<U>(b));
    }

    template<is_enum_only E>
    constexpr E operator&(E a, E b)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(a) & static_cast<U>(b));
    }

    template<is_enum_only E>
    constexpr E operator^(E a, E b)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(a) ^ static_cast<U>(b));
    }

    template<is_enum_only E>
    constexpr E operator~(E a)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(~static_cast<U>(a));
    }
};

#endif // LIBSUGARX_TYPES_H