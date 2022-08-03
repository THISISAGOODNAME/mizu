// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Core::Memory {

/**
 * Page size used by the ARM architecture. This is the smallest granularity with which memory can
 * be mapped.
 */
constexpr std::size_t PAGE_BITS = 12;
constexpr u64 PAGE_SIZE = 1ULL << PAGE_BITS;
constexpr u64 PAGE_MASK = PAGE_SIZE - 1;

} // namespace Core::Memory