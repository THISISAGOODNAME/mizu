// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <bit>
#include <climits>
#include <cstddef>

#include "common/common_types.h"

namespace Common {

/// Gets the size of a specified type T in bits.
template <typename T>
[[nodiscard]] constexpr std::size_t BitSize() {
    return sizeof(T) * CHAR_BIT;
}

#ifdef VIDEO_CORE_COMPAT
#ifdef _MSC_VER
inline u32 CountTrailingZeroes32(u32 value) {
    unsigned long trailing_zero = 0;

    if (_BitScanForward(&trailing_zero, value) != 0) {
        return trailing_zero;
    }

    return 32;
}

inline u32 CountTrailingZeroes64(u64 value) {
    unsigned long trailing_zero = 0;

    if (_BitScanForward64(&trailing_zero, value) != 0) {
        return trailing_zero;
    }

    return 64;
}
#else
inline u32 CountTrailingZeroes32(u32 value) {
    if (value == 0) {
        return 32;
    }

    return static_cast<u32>(__builtin_ctz(value));
}

inline u32 CountTrailingZeroes64(u64 value) {
    if (value == 0) {
        return 64;
    }

    return static_cast<u32>(__builtin_ctzll(value));
}
#endif
#endif

[[nodiscard]] constexpr u32 MostSignificantBit32(const u32 value) {
    return 31U - static_cast<u32>(std::countl_zero(value));
}

[[nodiscard]] constexpr u32 MostSignificantBit64(const u64 value) {
    return 63U - static_cast<u32>(std::countl_zero(value));
}

[[nodiscard]] constexpr u32 Log2Floor32(const u32 value) {
    return MostSignificantBit32(value);
}

[[nodiscard]] constexpr u32 Log2Floor64(const u64 value) {
    return MostSignificantBit64(value);
}

[[nodiscard]] constexpr u32 Log2Ceil32(const u32 value) {
    const u32 log2_f = Log2Floor32(value);
    return log2_f + static_cast<u32>((value ^ (1U << log2_f)) != 0U);
}

[[nodiscard]] constexpr u32 Log2Ceil64(const u64 value) {
    const u64 log2_f = Log2Floor64(value);
    return static_cast<u32>(log2_f + static_cast<u64>((value ^ (1ULL << log2_f)) != 0ULL));
}

} // namespace Common
